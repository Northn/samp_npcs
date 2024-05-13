#include "Npc.h"
#include "NpcComponent.h"
#include "NpcNetwork.hpp"

Npc::Npc(int skin, Vector3 position)
    : skin(skin),
      pos(position),
      virtualWorld(0),
      angle(0.f),
      health(100.f),
      currentWeaponId(0),
      weaponShootingAccuracy(70),
      weaponShootingRate(70),
      invulnerable(false),
      stunAnimationEnabled(true),
      shouldBroadcastSyncPacket(false),
      weaponSkill(NpcWeaponSkillType_STD),
      currentTask(NpcTaskStandStill()) {
  /* Nothing to do */
}

void Npc::restream() {
  for (auto player : streamedFor_.entries()) {
    streamOutForClient(*player);
    streamInForClient(*player);
  }
}

void Npc::destream() {
  for (auto player : streamedFor_.entries()) {
    streamOutForClient(*player);
  }
}

void Npc::streamInForClient(IPlayer &player) {
  NpcControlRpc rpc;
  rpc.Type = NpcControlRpc::NpcControlRpcType_StreamIn;
  rpc.NpcID = getID();
  rpc.StreamIn.Skin = skin;
  rpc.StreamIn.Position = pos;
  rpc.StreamIn.Heading = angle;
  rpc.StreamIn.Health = health;
  rpc.StreamIn.StunEnabled = stunAnimationEnabled;
  rpc.StreamIn.WeaponID = currentWeaponId;
  rpc.StreamIn.WeaponShootingAccuracy = weaponShootingAccuracy;
  rpc.StreamIn.WeaponShootingRate = weaponShootingRate;
  rpc.StreamIn.WeaponSkill = weaponSkill;
  rpc.StreamIn.Task = currentTask;
  PacketHelper::send(rpc, player);
}

void Npc::streamOutForClient(IPlayer &player) {
  NpcControlRpc rpc;
  rpc.Type = NpcControlRpc::NpcControlRpcType_StreamOut;
  rpc.NpcID = getID();
  PacketHelper::send(rpc, player);
}

void Npc::broadcastSyncIfRequired(Milliseconds onfootSyncRate) {
  if (!shouldBroadcastSyncPacket) return;
  if (const auto now = Time::now(); now - lastSyncBroadcast > onfootSyncRate) {
    broadcastSync();
    lastSyncBroadcast = now;
    shouldBroadcastSyncPacket = false;
  }
}

void Npc::broadcastSync() {
  NpcSyncPacket data;

  data.NpcID = getID();
  data.Position = pos;
  data.Heading = angle;
  data.Health = health;

  PacketHelper::broadcastToSome(data, streamedFor_.entries());
}

bool Npc::isStreamedInForPlayer(const IPlayer &player) const {
  return streamedFor_.valid(player.getID());
}

void Npc::streamInForPlayer(IPlayer &player) {
  streamedFor_.add(player.getID(), player);
  streamInForClient(player);
}

void Npc::streamOutForPlayer(IPlayer &player) {
  streamedFor_.remove(player.getID(), player);
  verifiedSupportedPlayers_.remove(player.getID(), player);
  streamOutForClient(player);
}

void Npc::setSkin(int skin_) {
  skin = skin_;
  restream();
}

int Npc::getSkin() const {
  return skin;
}

void Npc::setHealth(float health_) {
  auto oldHealth = health;
  health = health_;
  broadcastSync();
  if (oldHealth <= 0.f) {
    // Simply sending a new health to player does not revive npc
    // So it's simpler to just restream npc
    restream();
  }
}

float Npc::getHealth() const {
  return health;
}

void Npc::setInvulnerable(bool invuln) {
  invulnerable = invuln;
}

bool Npc::isInvulnerable() const {
  return invulnerable;
}

void Npc::setStunAnimationEnabled(bool enabled) {
  if (stunAnimationEnabled != enabled) {
    stunAnimationEnabled = enabled;
    restream();
  }
}

bool Npc::isStunAnimationEnabled() const {
  return stunAnimationEnabled;
}

void Npc::setWeapon(uint8_t weaponId) {
  WeaponSlotData weapon;
  weapon.id = weaponId;
  if (weapon.slot() == INVALID_WEAPON_SLOT) return;
  if (weapon.id == PlayerWeapon_Satchel || weapon.id == PlayerWeapon_Bomb) return;
  currentWeaponId = weaponId;
  restream();
}

void Npc::setWeaponShootingAccuracy(uint8_t accuracy) {
  weaponShootingAccuracy = std::clamp(accuracy, static_cast<uint8_t>(0), static_cast<uint8_t>(100u));
  restream();
}

void Npc::setWeaponShootingRate(uint8_t shootingRate) {
  weaponShootingRate = std::clamp(shootingRate, static_cast<uint8_t>(0), static_cast<uint8_t>(100u));
  restream();
}

void Npc::setWeaponSkill(NpcWeaponSkillType skill) {
  weaponSkill = skill;
  restream();
}

uint8_t Npc::getWeapon() const {
  return currentWeaponId;
}

void Npc::clearActiveTasks() {
  standStill();
}

void Npc::standStill() {
  NpcTaskStandStill task;
  currentTask = task;
  broadcastActiveTask();
}

void Npc::goToPoint(const Vector3 &destination, NpcMoveMode mode) {
  NpcTaskGoToPoint task;
  task.destination = destination;
  task.mode = mode;
  currentTask = task;
  broadcastActiveTask();
}

void Npc::attackPlayer(const IPlayer &player) {
  NpcTaskAttackPlayer task;
  task.target = &player;
  currentTask = task;
  broadcastActiveTask();
}

void Npc::followPlayer(const IPlayer &player) {
  NpcTaskFollowPlayer task;
  task.target = &player;
  currentTask = task;
  broadcastActiveTask();
}

int Npc::getID() const {
  return poolID;
}

Vector3 Npc::getPosition() const {
  return pos;
}

void Npc::setPosition(Vector3 position) {
  pos = position;
  broadcastSync();
}

GTAQuat Npc::getRotation() const {
  return GTAQuat(0.f, 0.f, angle);
}

void Npc::setRotation(GTAQuat rotation) {
  angle = rotation.ToEuler().z;
  restream();
}

int Npc::getVirtualWorld() const {
  return virtualWorld;
}

void Npc::setVirtualWorld(int vw) {
  virtualWorld = vw;
}

bool Npc::updateFromSync(const NpcSyncPacket &syncPacket, IPlayer *sender) {
  if (sender != nullptr) {
    verifiedSupportedPlayers_.add(sender->getID(), *sender);
  }
  const auto dist3D = syncPacket.Position - getPosition();
  const auto dist = glm::dot(dist3D, dist3D);

  if (dist > 3.f) {
    const auto currentPos = getPosition();
    const auto newPos = syncPacket.Position;

    const auto currentPos2D = Vector2(currentPos);
    const auto newPos2D = Vector2(newPos);
    const auto dist2D = newPos2D - currentPos2D;
    const auto dist2D_ = glm::dot(dist2D, dist2D);

    if (currentPos.z < -60.f && newPos.z < -70.f && dist2D_ <= 3.f) {
      // if we felt under the map
      // currentPos.z < -60.f -- if we're already below the ground for server
      // newPos.z < -70.f -- if client is still sending some position below the ground
      // two checkups were done to avoid immediate teleport below the ground

      // Nothing to do
    } else if (currentPos.z < -60.f && newPos.z > -20.f) {
      // 1. current position for server: we're under the map
      // 2. new position sent by client: we're on the map as should be

      // dist_ = distance between current and new position in 2d
      if (dist2D_ > 80.f) {
        // our npc teleported to the ground by the game
        // but teleport happened too far away
        // maybe some cheat? some bug? not sure, anyway that's something illegal
        broadcastSync(); // resend the actual sync data
        return false;
      }
    } else if (dist2D_ > 1.8f) {
//      NpcComponent::instance().core->logLn(LogLevel::Message, "Ignored: %0.2f", dist2D_);
      // The overall this kind of anticheat should be done in a better way
      // We should check the distance difference within second-two-three
      // And ignore if distance difference in 2d was too large
      broadcastSync(); // resend the actual sync data
      return false;
    }
  }

  // Allow sync packet only from the closest to npc player
  if (sender != nullptr) {
    const IPlayer *prioritizedPlayer = nullptr;
    if (const auto task = std::get_if<NpcTaskFollowPlayer>(&currentTask); task != nullptr) {
      prioritizedPlayer = task->target;
    }
//    else if (const auto task = std::get_if<NpcTaskAttackPlayer>(&currentTask); task != nullptr) {
//      prioritizedPlayer = task->target;
//    }
    if (prioritizedPlayer != nullptr
      && (NpcComponent::instance().isPlayerAfk(*prioritizedPlayer) || !isStreamedInForPlayer(*prioritizedPlayer) || !verifiedSupportedPlayers_.valid(prioritizedPlayer->getID()))
    ) {
      prioritizedPlayer = nullptr; // well, yeah
    }

    if (prioritizedPlayer != nullptr) {
      if (sender != prioritizedPlayer) {
        return false;
      }
    } else {
      const auto playerDist3D = sender->getPosition() - pos;
      const auto distFromPlayer = glm::dot(playerDist3D, playerDist3D);
      const auto &entries = streamedFor_.entries();
      for (const auto comparable : entries) {
        if (comparable == sender || NpcComponent::instance().isPlayerAfk(*comparable) || !verifiedSupportedPlayers_.valid(comparable->getID())) {
          continue;
        }
        const auto otherDist3D = comparable->getPosition() - pos;
        if (glm::dot(otherDist3D, otherDist3D) < distFromPlayer) {
          return false;
        }
      }
    }
  }

  shouldBroadcastSyncPacket = true;
  pos = syncPacket.Position;
  angle = syncPacket.Heading;

  return true;
}

void Npc::broadcastActiveTask() {
  NpcControlRpc rpc;
  rpc.Type = NpcControlRpc::NpcControlRpcType_SetActiveTask;
  rpc.NpcID = getID();
  rpc.StreamIn.Task = currentTask;
  PacketHelper::broadcastToSome(rpc, streamedFor_.entries());
}
