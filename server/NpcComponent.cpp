#include "NpcComponent.h"

#include <Server/Components/Pawn/Impl/pawn_natives.hpp>
#include <Server/Components/Pawn/Impl/pawn_impl.hpp>

#include <utils.hpp>

#include "NpcNetwork.hpp"

StringView NpcComponent::componentName() const {
  return COMPONENT_NAME;
}

SemanticVersion NpcComponent::componentVersion() const {
  return {COMPONENT_VERSION_MAJOR, COMPONENT_VERSION_MINOR, COMPONENT_VERSION_PATCH, COMPONENT_VERSION_TWEAK};
}

void NpcComponent::onLoad(ICore *c) {
  core = c;
  players = &core->getPlayers();
  streamConfigHelper = StreamConfigHelper(core->getConfig());
  onfootSyncRate = Milliseconds(*core->getConfig().getInt("network.on_foot_sync_rate"));
  setAmxLookups(core);

  core->getEventDispatcher().addEventHandler(this, EventPriority_FairlyLow);

  NpcControlRpc::addEventHandler(*core, &npcControlRPCHandler);
  NpcSyncPacket::addEventHandler(*core, &npcFootSyncHandler);

  players->getPlayerUpdateDispatcher().addEventHandler(this);
  players->getPoolEventDispatcher().addEventHandler(this);

  getNpcDamageDispatcher().addEventHandler(this);
}

void NpcComponent::onInit(IComponentList *components) {
  auto pawnComponent_ = components->queryComponent<IPawnComponent>();
  if (pawnComponent_ != nullptr) {
    pawnComponent = pawnComponent_;
    pawnComponent->getEventDispatcher().addEventHandler(this);
    setAmxFunctions(pawnComponent->getAmxFunctions());
    setAmxLookups(components);
  }
}

void NpcComponent::onAmxLoad(IPawnScript &script) {
  pawn_natives::AmxLoad(script.GetAMX());
}

void NpcComponent::onAmxUnload(IPawnScript &script) {
  /* Nothing to do */
}

void NpcComponent::onFree(IComponent *component) {
  if (component == pawnComponent) {
    pawnComponent = nullptr;
    setAmxFunctions();
    setAmxLookups();
  }
}

void NpcComponent::reset() {
  storage.clear();
}

void NpcComponent::free() {
  if (core != nullptr) {
    core->getEventDispatcher().removeEventHandler(this);

    NpcControlRpc::removeEventHandler(*core, &npcControlRPCHandler);
    NpcSyncPacket::removeEventHandler(*core, &npcFootSyncHandler);
  }
  if (players != nullptr) {
    players->getPlayerUpdateDispatcher().removeEventHandler(this);
    players->getPoolEventDispatcher().removeEventHandler(this);
  }
  if (pawnComponent != nullptr) {
    pawnComponent->getEventDispatcher().removeEventHandler(this);
  }

  getNpcDamageDispatcher().removeEventHandler(this);
}

bool NpcComponent::onPlayerUpdate(IPlayer &player, TimePoint now) {
  if (streamConfigHelper.shouldStream(player.getID(), now)) {
    const auto maxDist = streamConfigHelper.getDistanceSqr();
    for (auto npc : storage) {
      updateNpcStateForPlayer(dynamic_cast<Npc&>(*npc), player, maxDist);
    }
  }
  if (player.getState() != PlayerState_None) {
    lastPlayersUpdateSend[player.getID()] = now;
  }
  return true;
}

void NpcComponent::onPoolEntryDestroyed(IPlayer &player) {
  for (auto npc : storage) {
    npc->streamOutForPlayer(player);

    auto &npc_ = dynamic_cast<Npc&>(*npc);
    if (const auto task = std::get_if<NpcTaskAttackPlayer>(&npc_.currentTask); task != nullptr && task->target == &player) {
      npc->standStill();
    } else if (const auto task = std::get_if<NpcTaskFollowPlayer>(&npc_.currentTask); task != nullptr && task->target == &player) {
      npc->standStill();
    }
  }
}

void NpcComponent::onTick(Microseconds elapsed, TimePoint now) {
  for (auto npc : storage) {
    auto &npc_ = dynamic_cast<Npc&>(*npc);
    npc_.broadcastSyncIfRequired(onfootSyncRate);
  }
}

bool NpcComponent::onPlayerGiveDamageNpc(INpc &npc, IPlayer &from, float amount, unsigned int weapon, BodyPart part) {
  static constexpr auto publicName = "OnPlayerGiveDamageNpc";

  if (pawnComponent == nullptr) return true;

  for (auto sideScript : pawnComponent->sideScripts()) {
    auto ret = sideScript->Call(publicName, DefaultReturnValue_True, npc.getID(), from.getID(), amount, weapon, int(part));
    if (!ret) {
      return false;
    }
  }

  if (auto mainScript = pawnComponent->mainScript(); mainScript != nullptr) {
    auto ret = mainScript->Call(publicName, DefaultReturnValue_True, npc.getID(), from.getID(), amount, weapon, int(part));
    if (!ret) {
      return false;
    }
  }

  return true;
}

void NpcComponent::onPlayerTakeDamageNpc(INpc &npc, IPlayer &to, float amount, unsigned int weapon, BodyPart part) {
  static constexpr auto publicName = "OnPlayerTakeDamageNpc";

  if (pawnComponent == nullptr) return;

  for (auto sideScript : pawnComponent->sideScripts()) {
    auto ret = sideScript->Call(publicName, DefaultReturnValue_False, npc.getID(), to.getID(), amount, weapon, int(part));
    if (ret) break;
  }
  if (auto mainScript = pawnComponent->mainScript(); mainScript != nullptr) {
    mainScript->Call(publicName, DefaultReturnValue_False, npc.getID(), to.getID(), amount, weapon, int(part));
  }
}

void NpcComponent::onNpcDeath(INpc &npc, IPlayer &killer, int reason) {
  static constexpr auto publicName = "OnNpcDeath";

  if (pawnComponent == nullptr) return;

  for (auto sideScript : pawnComponent->sideScripts()) {
    auto ret = sideScript->Call(publicName, DefaultReturnValue_False, npc.getID(), killer.getID(), reason);
    if (ret) break;
  }
  if (auto mainScript = pawnComponent->mainScript(); mainScript != nullptr) {
    mainScript->Call(publicName, DefaultReturnValue_False, npc.getID(), killer.getID(), reason);
  }
}

void NpcComponent::updateNpcStateForPlayer(Npc &npc, IPlayer &player, float maxDist) {
  const auto world = npc.getVirtualWorld();
  const auto pos = npc.getPosition();

  const auto playerState = player.getState();
  const auto playerWorld = player.getVirtualWorld();
  const auto dist3D = pos - player.getPosition();
  const auto dist = glm::dot(dist3D, dist3D);

  const auto shouldBeStreamedIn = playerState != PlayerState_None && world == playerWorld && dist < maxDist;

  const auto isStreamedIn = npc.isStreamedInForPlayer(player);
  if (!isStreamedIn && shouldBeStreamedIn) {
    npc.streamInForPlayer(player);
  } else if (isStreamedIn && !shouldBeStreamedIn) {
    npc.streamOutForPlayer(player);
  }
}

bool NpcComponent::isPlayerAfk(const IPlayer &player) const {
  const auto lastPlayerUpdateSend = lastPlayersUpdateSend[player.getID()];
  return Time::now() - lastPlayerUpdateSend > Milliseconds(1800);
}

INpc *NpcComponent::create(int skin, Vector3 position) {
  return storage.emplace(skin, position);
}

void NpcComponent::release(int index) {
  if (auto npc = storage.get(index); npc != nullptr) {
    npc->destream();
    storage.release(index, false);
  }
}

void NpcComponent::lock(int index) {
  storage.lock(index);
}

bool NpcComponent::unlock(int index) {
  return storage.unlock(index);
}

IEventDispatcher<PoolEventHandler<INpc>> &NpcComponent::getPoolEventDispatcher() {
  return storage.getEventDispatcher();
}

IEventDispatcher<NpcDamageEventHandler> &NpcComponent::getNpcDamageDispatcher() {
  return npcDamageDispatcher;
}

const FlatPtrHashSet<INpc> &NpcComponent::entries() {
  return storage._entries();
}

Pair<size_t, size_t> NpcComponent::bounds() const {
  return std::make_pair(storage.Lower, storage.Upper);
}

INpc *NpcComponent::get(int index) {
  return storage.get(index);
}

bool NpcComponent::NpcFootSyncHandler::onReceive(IPlayer &peer, NetworkBitStream &bs) {
  if (peer.getState() == PlayerState_None || NpcComponent::instance().isPlayerAfk(peer)) {
    return false;
  }

  NpcSyncPacket data;
  if (!data.read(bs)) return false;

  auto npc_ = NpcComponent::instance().get(data.NpcID);
  if (npc_ == nullptr) return false;

  if (!npc_->isStreamedInForPlayer(peer)) return false;

  auto &npc = dynamic_cast<Npc&>(*npc_);

  return npc.updateFromSync(data, &peer);
}

bool NpcComponent::NpcControlRPCHandler::onReceive(IPlayer &peer, NetworkBitStream &bs) {
  if (peer.getState() == PlayerState_None || NpcComponent::instance().isPlayerAfk(peer)) {
    return false;
  }

  NpcControlRpc rpc;
  if (!rpc.read(bs)) return false;

  auto npc_ = NpcComponent::instance().get(rpc.NpcID);
  if (npc_ == nullptr) return false;

  if (!npc_->isStreamedInForPlayer(peer)) return false;

  auto &npc = dynamic_cast<Npc&>(*npc_);
  if (rpc.Type == NpcControlRpc::NpcControlRpcType_TakeDamage) {
    if (npc.invulnerable) return false;
    if (rpc.GiveTakeDamage.Damage < 0.f) return false;
    if (rpc.GiveTakeDamage.Bodypart < BodyPart_Torso || rpc.GiveTakeDamage.Bodypart > BodyPart_Head) return false;
    if (!IsWeaponForTakenDamageValid(rpc.GiveTakeDamage.WeaponID)) return false;
    if (npc.health <= 0.f) return false;

    if (!NpcComponent::instance().npcDamageDispatcher.stopAtFalse([&](auto *handler) {
      return handler->onPlayerGiveDamageNpc(*npc_, peer, rpc.GiveTakeDamage.Damage, rpc.GiveTakeDamage.WeaponID, BodyPart(rpc.GiveTakeDamage.Bodypart));
    })) return false;

    npc.health = std::max(0.f, npc.health - rpc.GiveTakeDamage.Damage);

    if (npc.health <= 0.f) {
      NpcComponent::instance().npcDamageDispatcher.dispatch(
          &NpcDamageEventHandler::onNpcDeath,
          *npc_, peer, rpc.GiveTakeDamage.WeaponID
      );
    }

    return true;
  } else if (rpc.Type == NpcControlRpc::NpcControlRpcType_GiveDamage) {
    if (rpc.GiveTakeDamage.Damage < 0.f) return false;
    if (rpc.GiveTakeDamage.Bodypart < BodyPart_Torso || rpc.GiveTakeDamage.Bodypart > BodyPart_Head) return false;
    // If we're going to add support of placing npcs into vehicles,
    // Then we should look for more damage reasons
    // See there: https://github.com/openmultiplayer/open.mp/issues/600#issuecomment-1377236916
    if (rpc.GiveTakeDamage.WeaponID != npc.getWeapon()) return false;

    NpcComponent::instance().npcDamageDispatcher.dispatch(
        &NpcDamageEventHandler::onPlayerTakeDamageNpc,
        *npc_, peer, rpc.GiveTakeDamage.Damage, rpc.GiveTakeDamage.WeaponID, BodyPart(rpc.GiveTakeDamage.Bodypart)
    );

    return true;
  }

  return false;
}

NpcComponent &NpcComponent::instance() {
  static NpcComponent component;
  return component;
}

COMPONENT_ENTRY_POINT() { return &NpcComponent::instance(); }
