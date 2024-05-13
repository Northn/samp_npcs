#include "npcs_module.h"
#include "utils.h"

void npcs_module::initialize() {
  register_rpc();
}

void npcs_module::reset() {
  npcs.clear();
}

void npcs_module::destroy() {
  unregister_rpc();
}

void npcs_module::loop() {
  {
    static auto i = 0;
    if (!utils::is_pause_menu_active() && (++i % 5) == 0) {
      last_nonafk_loop = steady_clock_t::now();
      i = 0;
    }
  }

  const auto should_send_sync = !utils::is_pause_menu_active() && steady_clock_t::now() - last_nonafk_loop < std::chrono::milliseconds(1800);
  for (auto &npc : npcs) {
    npc.second.update();
    if (should_send_sync) {
      // we could be afk and send outdated data
      // lets avoid this
      npc.second.send_sync_if_required();
    }
  }
}

void npcs_module::register_rpc() {
  auto rpc_id = kNpcControlRpcId;
  if (auto rakclient = utils::get_samp_rakclient_intf(); rakclient != nullptr) {
    rakclient->RegisterAsRemoteProcedureCall(&rpc_id, process_rpc);
  }
}

void npcs_module::unregister_rpc() {
  auto rpc_id = kNpcControlRpcId;
  if (auto rakclient = utils::get_samp_rakclient_intf(); rakclient != nullptr) {
    rakclient->UnregisterAsRemoteProcedureCall(&rpc_id);
  }
}

void npcs_module::process_rpc(RPCParameters *params) {
  BitStream bs(params->input, BITS_TO_BYTES(params->numberOfBitsOfData), false);

  uint8_t rpc_type_ = 0;
  bs.Read(rpc_type_);

  auto setup_npc_active_task = false;
  auto rpc_type = static_cast<control_rpc_id_t>(rpc_type_);

  uint16_t npc_id = 0xFFFF;
  bs.Read(npc_id);

  auto npc_iter = npcs.find(npc_id);
  if (rpc_type == control_rpc_id_t::kStreamIn) {
    uint16_t skin_id = 0;
    bs.Read(skin_id);

    CVector pos;

    bs.Read(pos.x);
    bs.Read(pos.y);
    bs.Read(pos.z);

    npc_iter = npcs.insert({npc_id, npc_t(npc_id, skin_id, pos)}).first;

    auto &npc = npc_iter->second;

    { // Setting up a heading
      float heading = 0.f;
      bs.Read(heading);

      npc.set_heading(heading);
    }

    { // Setting up a health
      float health = 100.f;
      bs.Read(health);

      npc.set_health(health);
    }

    { // Setting up a stun animation
      uint8_t is_stun_enabled_ = 0;
      bs.Read(is_stun_enabled_);
      auto is_stun_enabled = is_stun_enabled_ != 0;

      npc.set_stun_enabled(is_stun_enabled);
    }

    { // Setting up a weapon
      uint8_t weapon_id = 0;

      bs.Read(weapon_id);
      if (weapon_id != 0) {
        npc.set_current_weapon(weapon_id, 2147483640);
      }
    }

    { // Setting up a weapon accuracy, shooting rate and skill
      uint8_t accuracy = 0;
      uint8_t shooting_rate = 0;
      uint8_t skill = 0;

      bs.Read(accuracy);
      bs.Read(shooting_rate);
      bs.Read(skill);

      npc.set_weapon_accuracy(accuracy);
      npc.set_weapon_shooting_rate(shooting_rate);
      npc.set_weapon_skill(skill);
    }

    // After all data above task data is sent
    setup_npc_active_task = true;
  } else if (rpc_type == control_rpc_id_t::kStreamOut) {
    if (npc_iter != npcs.end()) {
      npcs.erase(npc_iter);
    }
  } else if (rpc_type == control_rpc_id_t::kSetActiveTask) {
    setup_npc_active_task = true;
  }

  if (setup_npc_active_task && npc_iter != npcs.end()) {
    // Setting up task
    // setup_npc_active_task is true if kSetActiveTask or kStreamIn sent

    auto &npc = npc_iter->second;
    uint8_t task_id = 0;

    bs.Read(task_id);

    switch (task_id) {
    case 1: { // attack player
      uint16_t target_player_id = 0xFFFF;
      bs.Read(target_player_id);

      npc.attack_player(target_player_id);
      break;
    }
    case 2: { // go to point
      CVector target;

      uint8_t mode;

      bs.Read(target.x);
      bs.Read(target.y);
      bs.Read(target.z);

      bs.Read(mode);

      npc.go_to_point(target, static_cast<npc::npc_move_mode_t>(mode));
      break;
    }
    case 3: { // follow player
      uint16_t target_player_id = 0xFFFF;
      bs.Read(target_player_id);

      npc.follow_player(target_player_id);
      break;
    }
    default: {
      // Considered as stand still task (0 id)
      npc.stand_still();
      break;
    }
    }
  }
}

void npcs_module::handle_incoming_packet(uint8_t id, Packet *packet) {
  if (id != kNpcSyncPacketId) {
    return;
  }

//  if (steady_clock_t::now() - last_nonafk_loop > std::chrono::milliseconds(800)) {
//    // Do not accept outdated packets (for example, when player is paused)
//    return;
//  }

  BitStream bs(packet->data, BITS_TO_BYTES(packet->bitSize), false);
  bs.IgnoreBits(8);

  uint16_t npc_id = 0xFFFF;
  bs.Read(npc_id);

  npc_sync_receive_data_t data;
  if (bs.GetNumberOfUnreadBits() < BYTES_TO_BITS(sizeof(data))) {
    return;
  }

  auto npc_iter = npcs.find(npc_id);
  if (npc_iter == npcs.end()) {
    return;
  }

  auto &npc = npc_iter->second;

  bs.Read(data);

  npc.update_from_sync(data);
}

void npcs_module::send_control_rpc(const BitStream &bs) {
  auto rakclient = utils::get_samp_rakclient_intf();
  if (rakclient == nullptr) return;

  BitStream send_bs;
  send_bs.SetData(bs.GetData());
  send_bs.SetWriteOffset(bs.GetWriteOffset());

  auto rpc_id = kNpcControlRpcId;
  rakclient->RPC(&rpc_id, &send_bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, false);
}

void npcs_module::send_npc_sync_packet(uint16_t npc_id, const npc_sync_send_data_t &data) {
  auto rakclient = utils::get_samp_rakclient_intf();
  if (rakclient == nullptr) return;

  if (npc_id == 0 || npc_id == 0xFFFF) return; // how is this possible?

  BitStream send_bs;
  send_bs.Write<uint8_t>(kNpcSyncPacketId);
  send_bs.Write<uint16_t>(npc_id);
  send_bs.Write(reinterpret_cast<const char*>(&data), sizeof(data)); // avoid copying data

  rakclient->Send(&send_bs, HIGH_PRIORITY, UNRELIABLE_SEQUENCED, 1);
}

bool npcs_module::handle_damage(CEntity *damager,
                                CPed *receiver,
                                float amount,
                                uint32_t body_part,
                                uint32_t weapon_type) {
  if (!utils::samp_is_playing())
    return false;
  if (damager == nullptr || receiver == nullptr)
    return false;
  if (amount <= 0.f)
    return false;

  auto damager_ped = reinterpret_cast<CPed *>(damager);
  if (!IsPedPointerValid(damager_ped))
    return false;

  auto my_ped = FindPlayerPed();

  if (my_ped == receiver) {
    if (auto npc_iter = find_npc_by_game_ped(damager_ped); npc_iter != npcs.end()) {
      auto npc_id = npc_iter->first;

      BitStream bs;
      bs.Write(static_cast<uint8_t>(control_rpc_id_t::kGiveDamage));
      bs.Write<uint16_t>(npc_id);
      bs.Write<float>(amount);
      bs.Write<uint8_t>(weapon_type);
      bs.Write<uint8_t>(body_part);
      send_control_rpc(bs);

      return false; // false = let game to handle this shot
    }
  } else if (my_ped == damager_ped) {
    if (auto npc_iter = find_npc_by_game_ped(receiver); npc_iter != npcs.end()) {
      auto npc_id = npc_iter->first;

      BitStream bs;
      bs.Write(static_cast<uint8_t>(control_rpc_id_t::kTakeDamage));
      bs.Write<uint16_t>(npc_id);
      bs.Write<float>(amount);
      bs.Write<uint8_t>(weapon_type);
      bs.Write<uint8_t>(body_part);
      send_control_rpc(bs);

      return true; // npc health should be sent by server
    }
  } else if (auto npc_iter = find_npc_by_game_ped(receiver); npc_iter != npcs.end()) {
    return true; // npc health should be changed only by server
    // fixes of npc being killed accidentally by another npc (lol)
  }
  return false;
}

bool npcs_module::handle_hit(CEntity *damager, CEntity *receiver) {
  if (!utils::samp_is_playing())
    return false;
  if (damager == nullptr || receiver == nullptr)
    return false;
  if (const auto my_ped = FindPlayerPed(); my_ped == nullptr || (receiver != my_ped && utils::get_ped_vehicle(my_ped) != receiver))
    return false;

  auto damager_ped = reinterpret_cast<CPed *>(damager);
  if (!IsPedPointerValid(damager_ped))
    return false;

  return find_npc_by_game_ped(damager_ped) != npcs.end();
}

float npcs_module::get_climb_height_add(CPed *ped) {
  if (ped == FindPlayerPed()) {
    return 0.f;
  }
  return find_npc_by_game_ped(ped) != npcs.end() ? 0.66f : 0.f; // 0.87f is max height when ped is jumping
}

decltype(npcs_module::npcs)::iterator npcs_module::find_npc_by_game_ped(CPed *ped) {
  if (ped == nullptr) {
    return npcs.end();
  }
  for (auto npc = npcs.begin(); npc != npcs.end(); ++npc) {
    if (npc->second.get_ped() == ped) {
      return npc;
    }
  }
  return npcs.end();
}

bool npcs_module::should_apply_damage_anim(CPed *ped) {
  if (ped == nullptr || ped == FindPlayerPed()) {
    return true;
  }

  auto npc_iter = find_npc_by_game_ped(ped);
  if (npc_iter != npcs.end()) {
    return npc_iter->second.is_stun_enabled();
  }
  return true;
}
