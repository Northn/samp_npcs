#pragma once

#include <component.hpp>
#include <Server/Components/Pawn/pawn.hpp>

#include <Impl/pool_impl.hpp>

#include "Npc.h"

using namespace Impl;

/// Npc death and damage event handlers
struct NpcDamageEventHandler {
  virtual bool onPlayerGiveDamageNpc(INpc& npc, IPlayer& from, float amount, unsigned weapon, BodyPart part) { return true; }
  virtual void onPlayerTakeDamageNpc(INpc& npc, IPlayer& to, float amount, unsigned weapon, BodyPart part) { }
  virtual void onNpcDeath(INpc& npc, IPlayer& killer, int reason) { }
};

class NpcComponent final : public PawnEventHandler,
                           public PlayerUpdateEventHandler,
                           public PoolEventHandler<IPlayer>,
                           public IPoolComponent<INpc>,
                           public CoreEventHandler,
                           public NpcDamageEventHandler,
                           public NoCopy {
public:
  static constexpr auto kNpcPoolSize = 8192;
  static constexpr auto kNpcSyncPacketId = NPC_SYNC_PACKET_ID;
  static constexpr auto kNpcControlRpcId = NPC_CONTROL_RPC_ID;

  PROVIDE_UID(0x37098B1B46B4198E);

  // Inherited from IComponent
  StringView componentName() const override;
  SemanticVersion componentVersion() const override;
  void onLoad(ICore *c) override;
  void onInit(IComponentList *components) override;
  void onFree(IComponent *component) override;
  void reset() override;
  void free() override;

  // Inherited from PawnEventHandler
  void onAmxLoad(IPawnScript &script) override;
  void onAmxUnload(IPawnScript &script) override;

  // Inherited from PlayerUpdateEventHandler
  bool onPlayerUpdate(IPlayer &player, TimePoint now) override;

  // Inherited from PoolEventHandler<IPlayer>
  void onPoolEntryDestroyed(IPlayer &player) override;

  void onTick(Microseconds elapsed, TimePoint now) override;

  // Inherited from NpcDamageEventHandler
  bool onPlayerGiveDamageNpc(INpc& npc, IPlayer& from, float amount, unsigned weapon, BodyPart part) override;
  void onPlayerTakeDamageNpc(INpc& npc, IPlayer& to, float amount, unsigned weapon, BodyPart part) override;
  void onNpcDeath(INpc& npc, IPlayer& killer, int reason) override;

  static void updateNpcStateForPlayer(Npc &npc, IPlayer &player, float maxDist);
  bool isPlayerAfk(const IPlayer &player) const;

  INpc *create(int skin, Vector3 position);

  // Inherited from IPoolComponent<INpc> -> IPool<INpc>
  void release(int index) override;
  void lock(int index) override;
  bool unlock(int index) override;
  IEventDispatcher<PoolEventHandler<INpc>> &getPoolEventDispatcher() override;

  // Event dispatcher providers
  IEventDispatcher<NpcDamageEventHandler>& getNpcDamageDispatcher();
protected:
  const FlatPtrHashSet<INpc> &entries() override;
public:

  // Inherited from IPoolComponent<INpc> -> IPool<INpc> -> IReadOnlyPool<INpc>
  Pair<size_t, size_t> bounds() const override;
  INpc *get(int index) override;

  struct NpcFootSyncHandler : public SingleNetworkInEventHandler {
    bool onReceive(IPlayer &peer, NetworkBitStream &bs) override;
  } npcFootSyncHandler;

  struct NpcControlRPCHandler : public SingleNetworkInEventHandler {
    bool onReceive(IPlayer &peer, NetworkBitStream &bs) override;
  } npcControlRPCHandler;

  static NpcComponent &instance();

private:
  ICore *core = nullptr;

  IPawnComponent *pawnComponent = nullptr;
  IPlayerPool *players = nullptr;

  DefaultEventDispatcher<NpcDamageEventHandler> npcDamageDispatcher;

  StaticArray<TimePoint, PLAYER_POOL_SIZE> lastPlayersUpdateSend;

  Milliseconds onfootSyncRate;
  StreamConfigHelper streamConfigHelper;
  MarkedPoolStorage<Npc, INpc, 1, kNpcPoolSize> storage;
};
