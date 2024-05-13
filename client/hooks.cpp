#include "hooks.h"
#include "npcs_module/utils.h"

void npcs_module::hooks::install() {
  CTimer_Update_hook.set_cb(npcs_module::hooks::mainloop);
  CTimer_Update_hook.set_dest(0x53E968);
  CTimer_Update_hook.install();

  { // Install SAMP hooks
    static bool once = false;
    if (auto samp_base = utils::get_samp_module(); !once && samp_base != 0) {
      once = true;

      packet_handler_hook.set_dest(samp_base + 0xAF4B);
      packet_handler_hook.set_cb([](const auto &hook) {
        auto packet_id = static_cast<uint8_t>(hook.get_context().eax);
        auto packet = reinterpret_cast<Packet *>(hook.get_context().edi);
        npcs_module::handle_incoming_packet(packet_id, packet);
      });
      packet_handler_hook.install();

      SAMP_DamageHandler_hook.set_dest(samp_base + 0xA362C);
      SAMP_DamageHandler_hook.set_cb(SAMP_DamageHandler);
      SAMP_DamageHandler_hook.install();

      SAMP_CWeapon_FireInstantHit_hook.set_dest(samp_base + 0xA50D0);
      SAMP_CWeapon_FireInstantHit_hook.set_cb(SAMP_CWeapon_FireInstantHit);
      SAMP_CWeapon_FireInstantHit_hook.install();

      CNetGame_ShutdownForRestart_hook.set_dest(samp_base + 0xA1E0);
      CNetGame_ShutdownForRestart_hook.set_cb(CNetGame_ShutdownForRestart);
      CNetGame_ShutdownForRestart_hook.install();
    }
  }

  { // Patch peds onfoot game ai to let them climb on obstacles
    utils::nop_and_naked_hook(0x4BF4CB, 0x4BF4D3, [](const auto &hook) {
      hook.get_return_address() = 0x4BF533;
    }); // Disables CTaskComplexWalkRoundBuildingAttempt return if ped is not following player and not killing anyone
    utils::nop_and_naked_hook(0x4BF5FD, 0x4BF64F, [](const auto &hook) {
      hook.get_return_address() = 0x4BF6AC;
    }); // Disables CTaskComplexWalkRoundBuildingAttempt return if CPedGeometryAnalyser::CanPedTargetPed is failed

    utils::nop_and_naked_hook(0x4BF666, 0x4BF689, [](const auto &hook) {
      // set targetEntity to nullptr
      // useless, it's actually already nullptr, because
      // enterCarAsPassengerTask_1 is located by esi register
      // and, at this point, it's already nulled
      // hook.get_context().esi = 0x0;

      hook.get_return_address() = 0x4BF6AC;
    }); // Disables CTaskComplexWalkRoundBuildingAttempt return if ped is not waiting for another ped to enter

    utils::nop_range(0x4BF6C8, 0x4BF6D0); // Do not call & check CPedGroups::IsInPlayersGroup in CEventHandler::ComputeBuildingCollisionResponse
    utils::nop_range(0x5F3493, 0x5F34AE); // Do not call & check CPedGroups::IsInPlayersGroup in CPedGeometryAnalyser::CanPedJumpObstacle and do not substract 0.15 from origin if true
    utils::nop_range(0x67DA58, 0x67DA7C); // Allow high jump in CTaskComplexJump::CreateSubTask regardless of CPedGroups::IsInPlayersGroup return value

    { // Never call IKChainManager_c::LookAt and the rest of code related to it
      utils::nop_range(0x4BF76A, 0x4BF76D); // do not pick value from [esi+36h], useless & at this point it can be nullptr
      utils::nop_and_naked_hook(0x4BF771, 0x4BF7C6, [](const auto &hook) {
        // Never call IKChainManager_c::LookAt and the rest of code related to it
        // Jump to another address if target ped (by esi register) is zero
        // Another address (0x4BF4E3) is literally the same function, but called by position instead of entity
        // Keep working if it's not zero (will continue at 4BF7C6)
        if (hook.get_context().esi == 0x0) {
          hook.get_return_address() = 0x4BF4E3;
          return;
        }
      });
    }

    // Hook CTaskSimpleClimb::TestForClimb to add ped height for npcs
    // This will let peds climb on high obstacles even without jumping from far away
    CTaskSimpleClimb_TestForClimb_hook.set_dest(0x6803A0);
    CTaskSimpleClimb_TestForClimb_hook.set_cb(CTaskSimpleClimb_TestForClimb);
    CTaskSimpleClimb_TestForClimb_hook.install();

//    { // Disable IK (Inverse Kinematics) in some tasks
//      auto cb = [](const auto address) {
//        memset(address, 0x90, 32);
//
//        address[0] = 0xC2;
//        address[1] = 0x04;
//        address[2] = 0x00;
//        // retn 4
//      };
//
//      // CTaskSimpleGoTo::SetUpIK
//      utils::unprotect_and_run(0x667AD0, 0x667C96, cb);
//
//      // CTaskSimpleAchieveHeading::SetUpIK
//      utils::unprotect_and_run(0x667F20, 0x66805F, cb);
//
//      // CTaskComplexAvoidOtherPedWhileWandering::SetUpIK
//      utils::unprotect_and_run(0x66A850, 0x66AA17, cb);
//    }
  }

  // Do not kill npcs by car
  CPed_KillPedWithCar_hook.set_dest(0x5F0360);
  CPed_KillPedWithCar_hook.set_cb(CPed_KillPedWithCar);
  CPed_KillPedWithCar_hook.install();

  // Control npc stun animation
  CEventDamage_ComputeDamageAnim_hook.set_dest(0x4B3FC0);
  CEventDamage_ComputeDamageAnim_hook.set_cb(CEventDamage_ComputeDamageAnim);
  CEventDamage_ComputeDamageAnim_hook.install();

  // Do not let npcs to block attack when aiming weapon/shooting them if npc antistun is enabled
  utils::nop_and_naked_hook(0x62A29C, 0x62A2A4, [](const auto &hook) {
    // TODO: broken kthook address
    //  CRASH ALERT: if this someonce begins crashing you should probably remove +8
    auto true_esp = hook.get_context().esp + 0x8;

    auto ped = reinterpret_cast<CPed*>(hook.get_context().edi);
    auto new_melee_command = 0x2; // MCOMMAND_BLOCK
    if (const auto npc = npcs_module::find_npc_by_game_ped(ped); npc != npcs.cend() && !npc->second.is_stun_enabled()) {
      new_melee_command = 0xB; // MCOMMAND_ATTACK_1
    }
    *reinterpret_cast<uint32_t*>(true_esp + 0x10) = new_melee_command;
  });

  { // Do not follow node route in CTaskComplexSeekEntity if npc is too far from ped
    // This task is used in CTaskComplexKillPedOnFoot too to reach ped

    { // CTaskComplexSeekEntity::<CEntitySeekPosCalculatorStandard>::CreateFirstSubTask
      utils::nop_range(0x46F7A3, 0x46F7A5);
      utils::unprotect_and_run(0x46F7BB, 2, [](const auto ptr) {
        ptr[0] = 0xEB; // jnz short loc_46F7CF -> jmp short loc_46F7CF
      });
    }
    { // CTaskComplexSeekEntity<CEntitySeekPosCalculatorStandard>::CreateNextSubTask
      utils::nop_range(0x46F188, 0x46F18A);
    }
  }
}

void npcs_module::hooks::mainloop(const decltype(CTimer_Update_hook) &hook) {
  { // Module initialization
    static bool once = false;
    if (auto samp_netgame = utils::get_samp_netgame(); !once && samp_netgame != 0) {
      once = true;
      npcs_module::initialize();
    }
  }

//  { // Runtime hooks installation
//    static bool once = false;
//    if (!once) {
//      auto hwnd = utils::get_game_hwnd();
//      auto d3d_device = utils::get_d3d_device();
//      if (hwnd != nullptr && d3d_device != nullptr) {
//        auto d3d_device_vtbl = *reinterpret_cast<uintptr_t *>(d3d_device);
//
//        once = true;
//      }
//    }
//  }
  npcs_module::loop();
  hook.call_trampoline();
}

BOOL npcs_module::hooks::SAMP_DamageHandler(const decltype(SAMP_DamageHandler_hook) &hook,
                                            uintptr_t damage_response_calculator, // actually an CPedDamageResponseCalculator
                                            CPed *ped) {
  auto result = hook.call_trampoline(damage_response_calculator, ped);
  if (!result) { // if samp did not handle it
    // not sure why multiplication is there
    // actually it's 0.33000001
    const auto &samp_damage_multiplier = *reinterpret_cast<float *>(utils::get_samp_module() + 0xE5900);

    const auto damager = *reinterpret_cast<CEntity **>(damage_response_calculator + 0x0);
    const auto damage_amount = *reinterpret_cast<float *>(damage_response_calculator + 0x4) * samp_damage_multiplier;
    const auto body_part = *reinterpret_cast<uint32_t *>(damage_response_calculator + 0x8);
    const auto weapon_type = *reinterpret_cast<uint32_t *>(damage_response_calculator + 0xC);

    result = npcs_module::handle_damage(damager, ped, damage_amount, body_part, weapon_type) ? TRUE : FALSE;
  }
  return result;
}

bool npcs_module::hooks::SAMP_CWeapon_FireInstantHit(const decltype(SAMP_CWeapon_FireInstantHit_hook) &hook,
                                                     CWeapon *weapon,
                                                     CEntity *firingEntity,
                                                     CVector *posn,
                                                     CVector *effectPosn,
                                                     CEntity *targetEntity,
                                                     CVector *target,
                                                     CVector *posnForDriveBy,
                                                     bool a8,
                                                     bool additionalEffects) {
  // we have to hook samp FireInstantHit, otherwise SAMP_DamageHandler would not get called if npc hits someone
  // TODO: SAMP_CWeapon_FireSniper, should be hooked?
  // TODO: Driveby npc = crash
  // TODO: What to do if npc is falling from a high point?
  if (npcs_module::handle_hit(firingEntity, targetEntity)) { // if handled
    // call original
    return weapon->FireInstantHit(firingEntity,
                                  posn,
                                  effectPosn,
                                  targetEntity,
                                  target,
                                  posnForDriveBy,
                                  a8,
                                  additionalEffects);
  }
  return hook.call_trampoline(weapon,
                              firingEntity,
                              posn,
                              effectPosn,
                              targetEntity,
                              target,
                              posnForDriveBy,
                              a8,
                              additionalEffects);
}

CEntity *npcs_module::hooks::CTaskSimpleClimb_TestForClimb(const decltype(CTaskSimpleClimb_TestForClimb_hook) &hook,
                                                           CPed *ped,
                                                           CVector *outClimbPos,
                                                           float *outClimbHeading,
                                                           uint8_t *outSurfaceType,
                                                           bool bLaunch) {
  auto entity = hook.call_trampoline(ped, outClimbPos, outClimbHeading, outSurfaceType, bLaunch);
  if (entity == nullptr) {
    if (auto height_add = get_climb_height_add(ped); height_add != 0.f) {
      const auto orig_pos = ped->GetPosition();
      const auto new_pos = orig_pos + CVector(0.f, 0.f, height_add);
      ped->SetPosn(new_pos);
      entity = hook.call_trampoline(ped, outClimbPos, outClimbHeading, outSurfaceType, bLaunch);
      ped->SetPosn(orig_pos);
    }
  }
  return entity;
}

void npcs_module::hooks::CPed_KillPedWithCar(const kthook::kthook_simple<CPed_KillPedWithCar_t> &hook,
                                             CPed *ped,
                                             CVehicle *vehicle,
                                             float fDamageIntensity,
                                             bool bPlayDeadAnimation) {
  if (find_npc_by_game_ped(ped) != npcs.cend()) {
    // Do not kill npcs by car
    return;
  }
  hook.call_trampoline(ped, vehicle, fDamageIntensity, bPlayDeadAnimation);
}

void npcs_module::hooks::CEventDamage_ComputeDamageAnim(const decltype(CEventDamage_ComputeDamageAnim_hook) &hook,
                                                        CEventDamage *event,
                                                        CPed *ped,
                                                        char a3) {
  if (!should_apply_damage_anim(ped)) return;
  hook.call_trampoline(event, ped, a3);
}

void npcs_module::hooks::CNetGame_ShutdownForRestart(const kthook::kthook_simple<CNetGame_ShutdownForRestart_t> &hook,
                                                     void *netgame) {
  reset();
  hook.call_trampoline(netgame);
}
