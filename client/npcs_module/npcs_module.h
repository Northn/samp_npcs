#pragma once

#include "npc.h"

namespace npcs_module {
constexpr auto kNpcSyncPacketId = NPC_SYNC_PACKET_ID;
constexpr auto kNpcControlRpcId = NPC_CONTROL_RPC_ID;

enum class control_rpc_id_t {
  kStreamIn,  // by server
  kStreamOut, // by server

  kGiveDamage, // by client
  kTakeDamage, // by client

  kSetActiveTask, // by server
};

using steady_clock_t = std::chrono::steady_clock;

using npc_t = npc;

// Created npcs
inline std::unordered_map<uint16_t, npc_t> npcs;

// Last loop() call is used to determine when we should accept packets and if we should at all
inline steady_clock_t::time_point last_nonafk_loop;

#pragma pack(push, 1)
struct npc_sync_send_data_t {
  float pos_x = 0.f;
  float pos_y = 0.f;
  float pos_z = 0.f;

  float heading = 0.f;
};

struct npc_sync_receive_data_t {
  float pos_x = 0.f;
  float pos_y = 0.f;
  float pos_z = 0.f;

  float heading = 0.f;

  float health = 100.f;
};
#pragma pack(pop)

// Initialization and destruction itself
void initialize();
void reset();
void destroy();

// Looper
void loop();

// Network
void register_rpc();
void unregister_rpc();
void process_rpc(RPCParameters *params);
void handle_incoming_packet(uint8_t id, Packet *packet);
void send_control_rpc(const BitStream &bs);
void send_npc_sync_packet(uint16_t npc_id, const npc_sync_send_data_t &data);

// Game events handlers
bool handle_damage(CEntity *damager, CPed *receiver, float amount, uint32_t body_part, uint32_t weapon_type);
bool handle_hit(CEntity *damager, CEntity *receiver);
float get_climb_height_add(CPed *ped);
bool should_apply_damage_anim(CPed *ped);

// Utils
decltype(npcs)::iterator find_npc_by_game_ped(CPed *ped);
}
