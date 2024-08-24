#pragma once

#include <Server/Components/Pawn/pawn.hpp>

#include <Impl/pool_impl.hpp>
#include <network.hpp>
#include <packet.hpp>

#include "NpcTask.hpp"

using namespace Impl;

enum NpcWeaponSkillType {
  NpcWeaponSkillType_Poor = 0,
  NpcWeaponSkillType_STD = 1,
  NpcWeaponSkillType_Pro = 2
};

struct INpc : public IExtensible, public IEntity {
  /// Checks if player has the npc streamed in for themselves
  virtual bool isStreamedInForPlayer(const IPlayer &player) const = 0;

  /// Streams in the npc for a specific player
  virtual void streamInForPlayer(IPlayer &player) = 0;

  /// Streams out the npc for a specific player
  virtual void streamOutForPlayer(IPlayer &player) = 0;

  /// Set the npc skin
  virtual void setSkin(int skin) = 0;

  /// Get the npc skin
  virtual int getSkin() const = 0;

  /// Set the npc health
  virtual void setHealth(float health) = 0;

  /// Get the npc health
  virtual float getHealth() const = 0;

  /// Set whether the npc is invulnerable
  virtual void setInvulnerable(bool invuln) = 0;

  /// Get whether the npc is invulnerable
  virtual bool isInvulnerable() const = 0;

  /// Set whether the npc suffers from damage by anim
  virtual void setStunAnimationEnabled(bool enabled) = 0;

  /// Get whether the npc suffers from damage by anim
  virtual bool isStunAnimationEnabled() const = 0;

  // Set npc active weapon
  virtual void setWeapon(uint8_t weaponId) = 0;

  /// Get the npc currently armed weapon
  virtual uint8_t getWeapon() const = 0;

  /// Set npc weapon shooting accuracy
  /// Available values between 0 (never hits the target) and 100 (always hits the target)
  virtual void setWeaponShootingAccuracy(uint8_t accuracy) = 0;

  /// Set npc weapon shooting rate
  /// Controls how often npc will shoot
  /// Available values between 0 (never shots the target) and 100 (always shots the target when reloaded)
  virtual void setWeaponShootingRate(uint8_t shootingRate) = 0;

  /// Set npc weapon skill
  virtual void setWeaponSkill(NpcWeaponSkillType skill) = 0;

  /// Clears npc active tasks
  /// Currently that's the alias to standStill
  virtual void clearActiveTasks() = 0;

  /// Npc will stand still and won't move anywhere
  virtual void standStill() = 0;

  // Npc will go to required point
  virtual void goToPoint(const Vector3 &destination, NpcMoveMode mode) = 0;

  // Npc will attack specified player
  virtual void attackPlayer(const IPlayer &player, bool aggressive = false) = 0;

  // Npc will follow specified player
  virtual void followPlayer(const IPlayer &player) = 0;
};

class Npc : public INpc,
            public PoolIDProvider,
            public NoCopy {
public:
  Npc(int skin, Vector3 position);

  void restream();
  void destream();
  void streamInForClient(IPlayer &player);
  void streamOutForClient(IPlayer &player);
  void broadcastSyncIfRequired(Milliseconds onfootSyncRate);
  void broadcastSync();
  bool updateFromSync(const struct NpcSyncPacket &syncPacket, IPlayer *sender = nullptr);
  void broadcastActiveTask();

  // Inherited from INpc
  bool isStreamedInForPlayer(const IPlayer &player) const override;
  void streamInForPlayer(IPlayer &player) override;
  void streamOutForPlayer(IPlayer &player) override;
  void setSkin(int skin_) override;
  int getSkin() const override;
  void setHealth(float health_) override;
  float getHealth() const override;
  void setInvulnerable(bool invuln) override;
  bool isInvulnerable() const override;
  void setStunAnimationEnabled(bool enabled) override;
  bool isStunAnimationEnabled() const override;
  void setWeapon(uint8_t weaponId) override;
  void setWeaponShootingAccuracy(uint8_t accuracy) override;
  void setWeaponShootingRate(uint8_t shootingRate) override;
  void setWeaponSkill(NpcWeaponSkillType skill) override;
  uint8_t getWeapon() const override;
  void clearActiveTasks() override;
  void standStill() override;
  void goToPoint(const Vector3 &destination, NpcMoveMode mode) override;
  void attackPlayer(const IPlayer &player, bool aggressive) override;
  void followPlayer(const IPlayer &player) override;

  // Inherited from IEntity -> IIDProvider
  int getID() const override;

  // Inherited from IEntity
  Vector3 getPosition() const override;
  void setPosition(Vector3 position) override;
  GTAQuat getRotation() const override;
  void setRotation(GTAQuat rotation) override;
  int getVirtualWorld() const override;
  void setVirtualWorld(int vw) override;

//private:
  uint16_t skin;
  Vector3 pos;
  int virtualWorld;
  float angle;

  bool invulnerable;
  bool stunAnimationEnabled;
  float health;

  uint8_t currentWeaponId;
  uint8_t weaponShootingRate;
  uint8_t weaponShootingAccuracy;
  NpcWeaponSkillType weaponSkill;

  NpcTasksSet currentTask;

  TimePoint lastSyncBroadcast;
  bool shouldBroadcastSyncPacket;

  UniqueIDArray<IPlayer, PLAYER_POOL_SIZE> streamedFor_;
  UniqueIDArray<IPlayer, PLAYER_POOL_SIZE> verifiedSupportedPlayers_; // players who have sent npc sync once at least
};
