#pragma once

#include "npcs_module/npcs_module.h"

namespace npcs_module::hooks {
using CTimer_Update_t = void (*)();

using SAMP_DamageHandler_t = BOOL (__stdcall *)(uintptr_t damage_response_calculator, CPed *ped);
using SAMP_CWeapon_FireInstantHit_t = bool (__thiscall *)(CWeapon *weapon,
                                                          CEntity *firingEntity,
                                                          CVector *posn,
                                                          CVector *effectPosn,
                                                          CEntity *targetEntity,
                                                          CVector *target,
                                                          CVector *posnForDriveBy,
                                                          bool a8,
                                                          bool additionalEffects);

using CTaskSimpleClimb_TestForClimb_t = CEntity *(*)(CPed *ped,
                                                     CVector *outClimbPos,
                                                     float *outClimbHeading,
                                                     uint8_t *outSurfaceType,
                                                     bool bLaunch);

using CPed_KillPedWithCar_t = void (__thiscall *)(CPed *ped,
                                                  CVehicle *vehicle,
                                                  float fDamageIntensity,
                                                  bool bPlayDeadAnimation);

using CNetGame_ShutdownForRestart_t = void (__thiscall *)(void*);

using CEventDamage_ComputeDamageAnim_t = void (__thiscall *)(CEventDamage *event, CPed *ped, char a3);

inline kthook::kthook_simple<CTimer_Update_t> CTimer_Update_hook;

inline kthook::kthook_simple<SAMP_DamageHandler_t> SAMP_DamageHandler_hook;
inline kthook::kthook_simple<SAMP_CWeapon_FireInstantHit_t> SAMP_CWeapon_FireInstantHit_hook;

inline kthook::kthook_simple<CTaskSimpleClimb_TestForClimb_t> CTaskSimpleClimb_TestForClimb_hook;

inline kthook::kthook_simple<CPed_KillPedWithCar_t> CPed_KillPedWithCar_hook;

inline kthook::kthook_simple<CEventDamage_ComputeDamageAnim_t> CEventDamage_ComputeDamageAnim_hook;

inline kthook::kthook_simple<CNetGame_ShutdownForRestart_t> CNetGame_ShutdownForRestart_hook;

inline kthook::kthook_naked packet_handler_hook;

void install();

void mainloop(const decltype(CTimer_Update_hook) &hook);

BOOL SAMP_DamageHandler(const decltype(SAMP_DamageHandler_hook) &hook, uintptr_t damage_response_calculator, CPed *ped);
bool SAMP_CWeapon_FireInstantHit(const decltype(SAMP_CWeapon_FireInstantHit_hook) &hook,
                                 CWeapon *weapon,
                                 CEntity *firingEntity,
                                 CVector *posn,
                                 CVector *effectPosn,
                                 CEntity *targetEntity,
                                 CVector *target,
                                 CVector *posnForDriveBy,
                                 bool a8,
                                 bool additionalEffects);

CEntity *CTaskSimpleClimb_TestForClimb(const decltype(CTaskSimpleClimb_TestForClimb_hook) &hook,
                                       CPed *ped,
                                       CVector *outClimbPos,
                                       float *outClimbHeading,
                                       uint8_t *outSurfaceType,
                                       bool bLaunch);

void CPed_KillPedWithCar(const decltype(CPed_KillPedWithCar_hook) &hook,
                         CPed *ped,
                         CVehicle *vehicle,
                         float fDamageIntensity,
                         bool bPlayDeadAnimation);

void CEventDamage_ComputeDamageAnim(const decltype(CEventDamage_ComputeDamageAnim_hook) &hook,
                                    CEventDamage *event, CPed *ped, char a3);

void CNetGame_ShutdownForRestart(const decltype(CNetGame_ShutdownForRestart_hook) &hook,
                                    void *netgame);
}
