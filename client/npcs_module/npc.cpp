#include "npc.h"
#include "utils.h"

#include "npcs_module.h"

npcs_module::npc::npc(uint16_t id, uint16_t model_id, const CVector &position) {
  my_id = id;
  if (!is_ped_model(model_id)) {
    model_id = MODEL_MALE01;
  }
  request_model(model_id);
  ped = std::make_unique<CCivilianPed>(PED_TYPE_CIVMALE, model_id);
  if (ped != nullptr) { // can it fail ped creation?
    ped->SetCharCreatedBy(2); // PED_MISSION
    ped->SetPosn(position);
    ped->SetOrientation(0.f, 0.f, 0.f);
    ped->m_nMoneyCount = 0;
    { // Setup ped flags
      ped->m_nPedFlags.bDoesntDropWeaponsWhenDead = true;
      ped->m_nPedFlags.bAllowMedicsToReviveMe = false;
      ped->m_nPedFlags.bNoCriticalHits = true;
      ped->m_nPedFlags.bDrownsInWater = false;
      ped->m_nPedFlags.bDontDragMeOutCar = true;
    }
    ped->m_fHealth = 100.f;
    ped->m_fMaxHealth = 100.f;
    ped->m_pIntelligence->SetPedDecisionMakerType(-2);

    // gonna send after X time since npc creation
    last_sync_send = std::chrono::steady_clock::now() + std::chrono::milliseconds(250);
    last_sync_send_check = last_sync_send;

    CWorld::Add(ped.get());

    clear_active_task();
  }
}

npcs_module::npc::~npc() {
  if (!is_ped_valid() && ped != nullptr) {
    static_cast<void>(ped.release());
  }
}

void npcs_module::npc::set_stun_enabled(bool enabled) {
  stun_enabled = enabled;
}

void npcs_module::npc::set_position(const CVector &position) {
  if (!is_ped_valid())
    return;

  ped->SetPosn(position);
}

void npcs_module::npc::set_heading(float heading) {
  if (!is_ped_valid())
    return;

  if (heading < 0.f) heading += 360.f;
  else if (heading > 360.f) heading -= 360.f;
  heading = std::clamp(heading, 0.f, 360.f);
  heading *= 0.017453292f;

  ped->m_fCurrentRotation = heading;
  ped->m_fAimingRotation = heading;
  ped->SetHeading(heading);
  ped->UpdateRwMatrix();
}

void npcs_module::npc::set_weapon_accuracy(uint8_t accuracy) {
  if (!is_ped_valid())
    return;

  ped->m_nWeaponAccuracy = std::clamp(accuracy, static_cast<uint8_t>(0u), static_cast<uint8_t>(100u));
}

void npcs_module::npc::set_weapon_shooting_rate(uint8_t shooting_rate) {
  if (!is_ped_valid())
    return;

  ped->m_nWeaponShootingRate = std::clamp(shooting_rate, static_cast<uint8_t>(0u), static_cast<uint8_t>(100u));
}

void npcs_module::npc::set_weapon_skill(uint8_t skill) {
  if (!is_ped_valid())
    return;

  ped->m_nWeaponSkill = std::clamp(skill, static_cast<uint8_t>(0u), static_cast<uint8_t>(2u)); // poor = 0; std = 1; pro = 2
}

void npcs_module::npc::set_current_weapon(uint8_t weapon_id, uint32_t ammo, uint16_t ammo_in_clip) {
  if (!is_ped_valid())
    return;

  ped->ClearWeapons();

  if (ammo <= 0) {
    return;
  }

  auto weapon_type = static_cast<eWeaponType>(weapon_id);
  auto weapon_info = CWeaponInfo::GetWeaponInfo(weapon_type, 1);
  auto weapon_model_id = weapon_info->m_nModelId1;

  if (weapon_model_id == -1 || !is_weapon_model(weapon_model_id)) {
    return;
  }

  request_model(weapon_model_id);
  ped->GiveWeapon(weapon_type, ammo, true);
  ped->SetCurrentWeapon(weapon_type);

  if (ammo_in_clip != 0xFFFF) {
    ped->m_aWeapons[weapon_info->m_nSlot].m_nAmmoInClip = ammo_in_clip;
  }

  ped->SetWeaponSkill(weapon_type, 2); // eWeaponSkill::PRO
}

void npcs_module::npc::set_health(float health) {
  if (!is_ped_valid())
    return;

  ped->m_fHealth = health;
  ped->m_fMaxHealth = std::max(health, ped->m_fMaxHealth);

  if (health <= 0.f && get_active_task_type() != TASK_COMPLEX_DIE) {
    clear_active_task(true);
    auto task = new CTaskComplexDie(WEAPON_UNARMED,
                                    ANIM_GROUP_DEFAULT,
                                    0xF,
                                    4.0,
                                    0.0,
                                    false,
                                    false,
                                    0,
                                    false);
    set_current_task(task);
  } else if (health > 0.f && get_active_task_type() == TASK_COMPLEX_DIE) {
    // TODO: Reviving is not working
    // TODO: Some changes applied to tasks setter, probably can work now
    stand_still();
  }
}

void npcs_module::npc::attack_player(uint16_t samp_player_id, bool aggressive) {
  if (!is_ped_valid() || is_dead() || utils::samp_get_player_health(samp_player_id) <= 0.f) {
    aggressive_attack = aggressive;
    player_attack_to = samp_player_id; // will probably attack later
    return;
  }

  clear_active_task();

  player_attack_to = samp_player_id; // set one more time
  aggressive_attack = aggressive;
  if (const auto player_ped = utils::get_samp_player_ped_game_ptr(player_attack_to); player_ped != nullptr) {
    auto task = new CTaskComplexKillPedOnFoot(player_ped, -1, 0, 0, 20, 1);
    set_current_task(task);
  }
}

void npcs_module::npc::follow_player(uint16_t samp_player_id) {
  if (!is_ped_valid() || is_dead() || utils::samp_get_player_health(samp_player_id) <= 0.f) {
    player_follow_to = samp_player_id; // will probably follow later
    return;
  }

  clear_active_task();

  player_follow_to = samp_player_id; // set one more time
  if (const auto player_ped = utils::get_samp_player_ped_game_ptr(player_follow_to); player_ped != nullptr) {
    static constexpr auto CTaskComplexFollowPedFootsteps_size = 0x20;
    auto CTaskComplexFollowPedFootsteps_constructor = reinterpret_cast<void (__thiscall *)(CTask *task,
                                                                                           CPed *target)>(0x694E20);

    auto task = reinterpret_cast<CTask *>(CTask::operator new(CTaskComplexFollowPedFootsteps_size));
    if (task != nullptr) {
      CTaskComplexFollowPedFootsteps_constructor(task, player_ped);
    }
    set_current_task(task);
  }
}

void npcs_module::npc::update() {
  if (!is_ped_valid())
    return;

  // TODO: NPC is attacking another player even if he's dead
  //  need to check remote player health
  if (should_attack_target() && !is_attacking_target()) {
    attack_player(get_attack_target(), aggressive_attack);
  } else if (should_follow_target() && !is_following_target()) {
    follow_player(get_follow_target());
  }

  if (is_dead() && get_active_task_type() != TASK_COMPLEX_DIE) {
    clear_active_task(true);
    auto task = new CTaskComplexDie(WEAPON_UNARMED,
                                    ANIM_GROUP_DEFAULT,
                                    0xF,
                                    4.0,
                                    0.0,
                                    false,
                                    false,
                                    0,
                                    false);
    set_current_task(task);
  }
}

void npcs_module::npc::update_from_sync(const npc_sync_receive_data_t &data) {
  if (!is_ped_valid())
    return;

  const auto my_pos = ped->GetPosition();
  const CVector sync_pos(data.pos_x, data.pos_y, data.pos_z);
  const auto dist = DistanceBetweenPoints(my_pos, sync_pos);
  const auto dist_2d = DistanceBetweenPoints(CVector2D(my_pos), CVector2D(sync_pos));
  const auto z_diff = std::abs(my_pos.z - sync_pos.z);
  const auto z_movespeed_multiplied = std::abs(ped->m_vecMoveSpeed.z) * 10.f;
  const auto am_i_falling = ped->m_vecMoveSpeed.z < -0.03f;
  const auto am_i_moving = ped->m_vecMoveSpeed.Magnitude() >= 0.03f; // 0.0308 when walking, 0.08 when running and 0.12 when sprinting

  if (dist > (am_i_moving ? 3.25f : 2.7f)) {
    if (am_i_falling && dist_2d <= 0.08f && z_diff <= z_movespeed_multiplied) {
//      std::cout << "N! Diff: " << dist << "; in 2d: " << dist_2d << std::endl;
//      std::cout << "N! Z Diff: " << z_diff << "; " << z_movespeed_multiplied << std::endl;
//      std::cout << "N! Move speed: " << movespeed_multiplied << "; " << (movespeed_multiplied / 10.f) << std::endl;
//      std::cout << "N! Move speed: " << ped->m_vecMoveSpeed.x << '\t' << ped->m_vecMoveSpeed.y << '\t'
//                << ped->m_vecMoveSpeed.z << std::endl;
    } else {
//      std::cout << "U! Updated pos!" << my_id << std::endl;
//      std::cout << "U! Diff: " << dist << "; in 2d: " << dist_2d << std::endl;
//      std::cout << "U! Z Diff: " << z_diff << "; " << z_movespeed_multiplied << std::endl;
//      std::cout << "U! Move speed: " << movespeed_multiplied << "; " << (movespeed_multiplied / 10.f) << std::endl;
//      std::cout << "U! Move speed: " << ped->m_vecMoveSpeed.x << '\t' << ped->m_vecMoveSpeed.y << '\t'
//                << ped->m_vecMoveSpeed.z << std::endl;
      set_position(sync_pos);
    }
//    ped->m_vecMoveSpeed.z = std::max(ped->m_vecMoveSpeed.z, -0.75f);
  }

  set_health(data.health);

//  const auto my_heading = get_heading();
//  if (std::abs(my_heading - data.heading) >= 15.f) {
//    set_heading(data.heading);
//  }
}

bool npcs_module::npc::send_sync_if_required() {
  if (!is_ped_valid()) return false;

  auto am_i_the_closest_to_npc = [this]() {
    const auto npc_pos = ped->GetPosition();
    const auto my_player_id = utils::samp_get_my_id();
    const auto my_distance_to_npc = DistanceBetweenPoints(npc_pos, FindPlayerPed()->GetPosition());
    for (auto i = 0; i < 1000; ++i) {
      if (i == my_player_id) continue;
      if (auto other_player_ped = utils::get_samp_player_ped_game_ptr(i);
          other_player_ped != nullptr && !utils::samp_is_player_afk(i) && DistanceBetweenPoints(other_player_ped->GetPosition(), npc_pos) < my_distance_to_npc) {
        return false;
      }
    }
    return true;
  };

  const auto now = std::chrono::steady_clock::now();

  auto sent_sync = false;

  if (now - last_sync_send_check > get_sync_send_rate()) {
    const auto should_send_this_frame = !sent_sync_once || am_i_the_closest_to_npc();
    if (should_send_this_frame) {
      send_sync();
      sent_sync = true;
    }
    last_sync_send_check = now;
  }

  return sent_sync;
}

void npcs_module::npc::stand_still() {
  if (!is_ped_valid())
    return;

  clear_active_task(true);
  auto task = new CTaskSimpleStandStill(0, true, false, 8.0);
  set_current_task(task);

  ped->SetIdle();
}

void npcs_module::npc::wander() {
  if (!is_ped_valid())
    return;

  clear_active_task();
  auto task = new CTaskComplexWanderStandard(PEDMOVE_WALK, 3, true);
  set_current_task(task);
}

void npcs_module::npc::go_to_point(const CVector &point, npc_move_mode_t mode) {
  if (!is_ped_valid() || is_dead())
    return;

  clear_active_task();

  auto move_mode = PEDMOVE_WALK;
  if (mode == npc_move_mode_t::kSprint) {
    move_mode = PEDMOVE_SPRINT;
  } else if (mode == npc_move_mode_t::kRun) {
    move_mode = PEDMOVE_RUN;
  }

  static constexpr auto CTaskComplexGoToPointAndStandStill_size = 0x28;
  auto CTaskComplexGoToPointAndStandStill_constructor = reinterpret_cast<void (__thiscall *)(CTask *task,
                                                                                             eMoveState moveState,
                                                                                             const CVector *posn,
                                                                                             float radius,
                                                                                             float moveStateRadius,
                                                                                             bool a6,
                                                                                             bool a7)>(0x668120);

  auto task = reinterpret_cast<CTask *>(CTask::operator new(CTaskComplexGoToPointAndStandStill_size));
  if (task != nullptr) {
    CTaskComplexGoToPointAndStandStill_constructor(task, move_mode, &point, 1.5f, 2.f, false, false);
  }
  set_current_task(task);
}

bool npcs_module::npc::is_stun_enabled() const {
  return stun_enabled;
}

bool npcs_module::npc::is_aggressive_attack() const {
  return aggressive_attack;
}

CPed *npcs_module::npc::get_ped() const {
  return is_ped_valid() ? ped.get() : nullptr;
}

std::chrono::milliseconds npcs_module::npc::get_sync_send_rate() const {
  using ms = std::chrono::milliseconds;
  if (!is_ped_valid()) {
    return ms(1000);
  }

  auto default_sendrate = utils::samp_get_onfoot_sync_rate();

  const auto dist = DistanceBetweenPoints(last_send_sync_pos, ped->GetPosition());

  if (dist <= 0.02f) {
    default_sendrate = ms(600);
  } else if (dist <= 0.07f) {
    default_sendrate *= 5;
  } else if (dist <= 0.12f) {
    default_sendrate *= 4;
  } else {
    default_sendrate *= 3;
  }

  return default_sendrate + ms(npcs.size());
}

bool npcs_module::npc::is_dead() const {
  if (!is_ped_valid()) return false;
  return ped->m_fHealth <= 0.f || get_active_task_type() == TASK_COMPLEX_DIE;
}

float npcs_module::npc::get_heading() const {
  if (!is_ped_valid())
    return 0.f;

  auto heading = ped->GetHeading() / 0.017453292f;
  if (heading < 0.f) heading += 360.f;
  else if (heading > 360.f ) heading -= 360.f;
  heading = std::clamp(heading, 0.f, 360.f);

  return heading;
}

void npcs_module::npc::send_sync() {
  if (!is_ped_valid()) {
    return;
  }

  const auto my_pos = ped->GetPosition();
  const auto my_heading = get_heading();

  last_sync_send = std::chrono::steady_clock::now();
  last_send_sync_pos = my_pos;
  sent_sync_once = true;

  npc_sync_send_data_t data;
  { // Filling up a sync data
    data.pos_x = my_pos.x;
    data.pos_y = my_pos.y;
    data.pos_z = my_pos.z;

    data.heading = my_heading;
  }

  send_npc_sync_packet(my_id, data);
}

bool npcs_module::npc::is_following_target() const {
  if (!is_ped_valid())
    return false;

  if (get_active_task_type() != TASK_COMPLEX_FOLLOW_PED_FOOTSTEPS)
    return false;

  const auto target_ped = *reinterpret_cast<CPed**>(reinterpret_cast<uintptr_t>(get_active_task()) + 0xC);
  return target_ped != nullptr && target_ped == utils::get_samp_player_ped_game_ptr(get_follow_target());
}

bool npcs_module::npc::should_follow_target() const {
  return get_follow_target() != kInvalidTargetId && !is_dead() && utils::samp_get_player_health(get_attack_target()) > 0.f;;
}

uint16_t npcs_module::npc::get_follow_target() const {
  return player_follow_to;
}

bool npcs_module::npc::is_attacking_target() const {
  if (!is_ped_valid())
    return false;

  if (get_active_task_type() != TASK_COMPLEX_KILL_PED_ON_FOOT)
    return false;

  const auto task_kill_ped_onfoot = reinterpret_cast<CTaskComplexKillPedOnFoot *>(get_active_task());
  const auto target = task_kill_ped_onfoot->m_pTarget;
  return target != nullptr && target == utils::get_samp_player_ped_game_ptr(get_attack_target());
}

bool npcs_module::npc::should_attack_target() const {
  return get_attack_target() != kInvalidTargetId && !is_dead() && utils::samp_get_player_health(get_attack_target()) > 0.f;
}

uint16_t npcs_module::npc::get_attack_target() const {
  return player_attack_to;
}

eTaskType npcs_module::npc::get_active_task_type() const {
  auto task = get_active_task();
  if (task == nullptr) {
    return TASK_NONE;
  }

  return task->GetId();
}

CTask *npcs_module::npc::get_active_task() const {
  if (!is_ped_valid())
    return nullptr;
  return ped->m_pIntelligence->m_TaskMgr.m_aPrimaryTasks[kTaskIdPlaceTo];
}

void npcs_module::npc::clear_active_task(bool immediately) {
  if (!is_ped_valid())
    return;

  auto destroy_task = [&](int type, bool immediately = false) {
    auto result = true;

    auto task = ped->m_pIntelligence->m_TaskMgr.m_aPrimaryTasks[type];
    if (task != nullptr) {
      result = task->MakeAbortable(ped.get(), immediately ? ABORT_PRIORITY_IMMEDIATE : ABORT_PRIORITY_URGENT, nullptr);
      if (result) {
        ped->m_pIntelligence->m_TaskMgr.SetTask(nullptr, type, false);
      }
    }

    return result;
  };

  auto destroy_secondary_task = [&](int type, bool immediately = false) {
    auto result = true;

    auto task = ped->m_pIntelligence->m_TaskMgr.m_aSecondaryTasks[type];
    if (task != nullptr) {
      result = task->MakeAbortable(ped.get(), immediately ? ABORT_PRIORITY_IMMEDIATE : ABORT_PRIORITY_URGENT, nullptr);
      if (result) {
        ped->m_pIntelligence->m_TaskMgr.SetTaskSecondary(nullptr, type);
      }
    }

    return result;
  };

  { // Reset tasks variables
    player_attack_to = kInvalidTargetId;
    aggressive_attack = false;
    player_follow_to = kInvalidTargetId;
  }

  for (auto i = 0; i < (5 - 1); ++i) {
    destroy_task(i, immediately);
  }

  { // Cleanup secondary attack & duck tasks
    destroy_secondary_task(TASK_SECONDARY_ATTACK, immediately);
    destroy_secondary_task(TASK_SECONDARY_DUCK, immediately);
  }

  // Cleanup animation
  auto ped_pos = ped->GetPosition();
  ped_pos.z += 0.001f;
  ped->Teleport(ped_pos, false);

  ped->SetIdle();
}

void npcs_module::npc::set_current_task(CTask *task) {
  if (!is_ped_valid())
    return;

  if (auto existingTask = ped->m_pIntelligence->m_TaskMgr.m_aPrimaryTasks[kTaskIdPlaceTo]; existingTask != nullptr) {
    existingTask->MakeAbortable(ped.get(), ABORT_PRIORITY_IMMEDIATE, nullptr);
  }

  ped->m_pIntelligence->m_TaskMgr.SetTask(task, kTaskIdPlaceTo, false);
}

CWeapon *npcs_module::npc::get_current_weapon() {
  if (!is_ped_valid())
    return nullptr;

  auto &weapon = ped->m_aWeapons[ped->m_nActiveWeaponSlot];
  if (weapon.m_eWeaponType == WEAPON_UNARMED)
    return nullptr;

  return &weapon;
}

bool npcs_module::npc::is_ped_valid() const {
  return ped != nullptr && IsPedPointerValid(ped.get());
}

bool npcs_module::npc::is_weapon_model(int model_id) {
  return is_model_available(model_id) && CModelInfo::GetModelInfo(model_id)->GetModelType() == MODEL_INFO_WEAPON;
}

bool npcs_module::npc::is_ped_model(int model_id) {
  return is_model_available(model_id) && CModelInfo::GetModelInfo(model_id)->GetModelType() == MODEL_INFO_PED;
}

bool npcs_module::npc::is_model_available(int model_id) {
  return CModelInfo::GetModelInfo(model_id) != nullptr;
}

bool npcs_module::npc::is_model_loaded(int model_id) {
  return is_model_available(model_id) && CStreaming::ms_aInfoForModel[model_id].m_nLoadState == LOADSTATE_LOADED;
}

bool npcs_module::npc::request_model(int model_id) {
  if (!is_model_available(model_id))
    return false;
  if (!is_model_loaded(model_id)) {
//    auto old_flags = CStreaming::ms_aInfoForModel[model_id].m_nFlags;

    CStreaming::RequestModel(model_id, GAME_REQUIRED);
    CStreaming::LoadAllRequestedModels(false);
    while (!is_model_loaded(model_id));

//    if (!(old_flags & GAME_REQUIRED)) {
//      CStreaming::SetModelIsDeletable(model_id);
//      CStreaming::SetModelTxdIsDeletable(model_id);
//    }
  }
  return true;
}

void npcs_module::npc::release_model(int model_id) {
  if (!is_model_loaded(model_id) || model_id == MODEL_MALE01)
    return;
//  CStreaming::RemoveModel(model_id);
  CStreaming::SetModelIsDeletable(model_id);
  CStreaming::SetModelTxdIsDeletable(model_id);
}
