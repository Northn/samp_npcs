#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d9.h>
#include <shlobj.h>
#include <psapi.h>

#include <cstdint>
#include <chrono>
#include <unordered_map>
#include <string>
#include <filesystem>
#include <memory>

#include <RakClient.h>
#include <BitStream.h>

#include <kthook/kthook.hpp>

#include <plugin.h>
#include <CCamera.h>
#include <CEntity.h>
#include <CVehicle.h>

// Required for npcs
#include <CCivilianPed.h>
#include <CTask.h>
#include <CTaskComplex.h>

#include <CTaskComplexKillPedOnFoot.h>
#include <CTaskSimpleStandStill.h>
#include <CTaskComplexWanderStandard.h>
#include <CTaskComplexDie.h>

#include <CStreaming.h>
#include <CWorld.h>
#include <CModelInfo.h>
