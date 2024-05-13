#include "utils.h"

bool npcs_module::utils::is_game_foreground() {
  auto game_hwnd = get_game_hwnd();
  if (game_hwnd == NULL) {
    return false;
  }
  return GetActiveWindow() == game_hwnd;
}

bool npcs_module::utils::is_pause_menu_active() {
  return *reinterpret_cast<bool *>(0xBA67A4);
}

CVector npcs_module::utils::get_ped_velocity(CPed *ped) {
  if (ped == nullptr) {
    return {0.f, 0.f, 0.f};
  }
  if (const auto vehicle = get_ped_vehicle(ped); vehicle != nullptr) {
    return vehicle->m_vecMoveSpeed;
  }
  return ped->m_vecMoveSpeed;
}

CVehicle *npcs_module::utils::get_ped_vehicle(CPed *ped) {
  if (ped == nullptr) {
    ped = FindPlayerPed(-1);
  }
  if (ped->m_nPedFlags.bInVehicle && ped->m_pVehicle != nullptr) {
    return ped->m_pVehicle;
  }
  return nullptr;
}

uintptr_t npcs_module::utils::get_samp_module() {
  static uintptr_t base = 0;
  if (base == 0)
    base = reinterpret_cast<uintptr_t>(GetModuleHandleA("samp.dll"));
  return base;
}

uintptr_t npcs_module::utils::get_samp_netgame() {
  auto samp_module = get_samp_module();
  if (samp_module == 0)
    return 0;
  return *reinterpret_cast<uintptr_t *>(samp_module + 0x26E8DC);
}

RakClientInterface *npcs_module::utils::get_samp_rakclient_intf() {
  auto samp_netgame = get_samp_netgame();
  if (samp_netgame == 0)
    return nullptr;
  return *reinterpret_cast<RakClientInterface **>(samp_netgame + 44);
}

bool npcs_module::utils::samp_is_playing() {
  auto samp_netgame = get_samp_netgame();
  if (samp_netgame == 0)
    return false;
  return *reinterpret_cast<int *>(get_samp_netgame() + 0x3CD) == 0x5;
}

uintptr_t npcs_module::utils::get_samp_player_pool() {
  auto samp_module = get_samp_module();
  auto samp_netgame = get_samp_netgame();
  if (samp_netgame == 0)
    return 0;
  return reinterpret_cast<uintptr_t(__thiscall *)(uintptr_t)>(samp_module + 0x1160)(samp_netgame);
}

uintptr_t npcs_module::utils::get_samp_vehicle_pool() {
  auto samp_module = get_samp_module();
  auto samp_netgame = get_samp_netgame();
  if (samp_netgame == 0)
    return 0;
  return reinterpret_cast<uintptr_t(__thiscall *)(uintptr_t)>(samp_module + 0x1170)(samp_netgame);
}

uintptr_t npcs_module::utils::get_samp_object_pool() {
  auto samp_module = get_samp_module();
  auto samp_netgame = get_samp_netgame();
  if (samp_netgame == 0)
    return 0;
  return reinterpret_cast<uintptr_t(__thiscall *)(uintptr_t)>(samp_module + 0x2DF0)(samp_netgame);
}

uintptr_t npcs_module::utils::get_samp_input_ptr() {
  auto samp_module = get_samp_module();
  if (samp_module == 0)
    return 0;
  return *reinterpret_cast<uintptr_t*>(samp_module + 0x26E8CC);
}

uintptr_t npcs_module::utils::get_samp_dialog_ptr() {
  auto samp_module = get_samp_module();
  if (samp_module == 0)
    return 0;
  return *reinterpret_cast<uintptr_t*>(samp_module + 0x26E898);
}

bool npcs_module::utils::is_samp_player_exists(uint16_t samp_player_id) {
  auto samp_module = get_samp_module();
  auto samp_playerpool = get_samp_player_pool();
  if (samp_playerpool == 0)
    return false;
  const auto my_id = samp_get_my_id();
  if (samp_player_id == my_id)
    return true;
  return reinterpret_cast<BOOL(__thiscall *)(uintptr_t, uint16_t)>(samp_module + 0x10B0)
      (samp_playerpool, samp_player_id) != 0;
}

bool npcs_module::utils::is_samp_vehicle_exists(uint16_t samp_vehicle_id) {
  auto samp_module = get_samp_module();
  auto samp_vehiclepool = get_samp_vehicle_pool();
  if (samp_vehiclepool == 0)
    return false;
  return reinterpret_cast<BOOL(__thiscall *)(uintptr_t, uint16_t)>(samp_module + 0x1140)
      (samp_vehiclepool, samp_vehicle_id) != 0;
}

bool npcs_module::utils::is_samp_object_exists(uint16_t samp_object_id) {
  auto samp_module = get_samp_module();
  auto samp_objectpool = get_samp_object_pool();
  if (samp_objectpool == 0)
    return false;
  return reinterpret_cast<BOOL(__thiscall *)(uintptr_t, uint16_t)>(samp_module + 0x12430)
      (samp_objectpool, samp_object_id) != 0;
}

uint16_t npcs_module::utils::samp_get_my_id() {
  if (auto samp_playerpool = get_samp_player_pool(); samp_playerpool != 0) {
    return *reinterpret_cast<uint16_t*>(samp_playerpool + 0x2F14 + 0x8);
  }
  return 0xFFFF;
}

std::string npcs_module::utils::samp_get_player_nickname(uint16_t samp_player_id) {
//  if (!is_samp_player_exists(samp_player_id)) return "";
  if (auto samp_playerpool = get_samp_player_pool(); samp_playerpool != 0) {
    if (samp_get_my_id() == samp_player_id) {
      return *reinterpret_cast<std::string*>(samp_playerpool + 0x2F14 + 0xE);
    }
    auto name = reinterpret_cast<const char *(__thiscall *)(uintptr_t, uint16_t)>(get_samp_module() + 0x16F00)(
        samp_playerpool,
        samp_player_id);
    return name != nullptr ? name : "";
  }
  return "";
}

uint16_t npcs_module::utils::samp_get_player_ping(uint16_t samp_player_id) {
  //  if (!is_samp_player_exists(samp_player_id)) return "";
  samp_update_players_pings_scores();
  if (auto samp_playerpool = get_samp_player_pool(); samp_playerpool != 0) {
    if (samp_get_my_id() == samp_player_id) {
      return static_cast<uint16_t>(*reinterpret_cast<uint32_t*>(samp_playerpool + 0x2F14 + 0x0));
    }
    return reinterpret_cast<int(__thiscall *)(uintptr_t, uint16_t)>(get_samp_module() + 0x6E110)(
        samp_playerpool,
        samp_player_id);
  }
  return 0;
}

float npcs_module::utils::samp_get_player_health(uint16_t samp_player_id) {
  if (!is_samp_player_exists(samp_player_id))
    return 0.f;
  auto samp_playerpool = get_samp_player_pool();
  if (samp_playerpool == 0) {
    return 0.f;
  }
  if (samp_get_my_id() == samp_player_id) {
    return FindPlayerPed(-1)->m_fHealth;
  }
  if (!samp_is_player_streamed_in(samp_player_id)) {
    return 0.f;
  }
  auto samp_module = get_samp_module();
  auto remote_player_ptr = reinterpret_cast<uintptr_t(__thiscall*)(uintptr_t, uint16_t)>(samp_module + 0x10F0)(samp_playerpool, samp_player_id);
  if (remote_player_ptr == 0) {
    return 0.f;
  }
  auto reported_health = *reinterpret_cast<float*>(remote_player_ptr + 0x1B0);
  return reported_health;
}

float npcs_module::utils::samp_get_player_armor(uint16_t samp_player_id) {
  if (!is_samp_player_exists(samp_player_id))
    return 0.f;
  auto samp_playerpool = get_samp_player_pool();
  if (samp_playerpool == 0) {
    return 0.f;
  }
  if (samp_get_my_id() == samp_player_id) {
    return FindPlayerPed(-1)->m_fArmour;
  }
  if (!samp_is_player_streamed_in(samp_player_id)) {
    return 0.f;
  }
  auto samp_module = get_samp_module();
  auto remote_player_ptr = reinterpret_cast<uintptr_t(__thiscall*)(uintptr_t, uint16_t)>(samp_module + 0x10F0)(samp_playerpool, samp_player_id);
  if (remote_player_ptr == 0) {
    return 0.f;
  }
  auto reported_armor = *reinterpret_cast<float*>(remote_player_ptr + 0x1AC);
  return reported_armor;
}

bool npcs_module::utils::samp_is_player_streamed_in(uint16_t samp_player_id) {
  return get_samp_player_ped_game_ptr(samp_player_id) != nullptr;
}

bool npcs_module::utils::samp_is_player_afk(uint16_t samp_player_id) {
  if (!is_samp_player_exists(samp_player_id))
    return false;
  auto samp_playerpool = get_samp_player_pool();
  if (samp_playerpool == 0) {
    return false;
  }
  if (samp_get_my_id() == samp_player_id) {
    return !is_game_foreground() || is_pause_menu_active();
  }
  if (!samp_is_player_streamed_in(samp_player_id)) {
    return false;
  }
  auto samp_module = get_samp_module();
  auto remote_player_ptr = reinterpret_cast<uintptr_t(__thiscall*)(uintptr_t, uint16_t)>(samp_module + 0x10F0)(samp_playerpool, samp_player_id);
  if (remote_player_ptr == 0) {
    return false;
  }
  using ms = std::chrono::milliseconds;
  auto last_update = *reinterpret_cast<uint32_t*>(remote_player_ptr + 0x1C5);
  auto last_update_diff = ms(GetTickCount() - last_update);
  return last_update_diff > ms(1800);
}

void npcs_module::utils::samp_update_players_pings_scores() {
  auto samp_module = get_samp_module();
  auto samp_netgame = get_samp_netgame();
  if (samp_netgame == 0) return;
  reinterpret_cast<void(__thiscall *)(uintptr_t)>(samp_module + 0x8BA0)(samp_netgame);
}

std::chrono::milliseconds npcs_module::utils::samp_get_onfoot_sync_rate(bool local_player_only) {
  using ms = std::chrono::milliseconds;

  auto samp_playerpool = get_samp_player_pool();
  if (samp_playerpool == 0) {
    return ms(1000);
  }
  auto localplayer = *reinterpret_cast<void**>(samp_playerpool + 0x2F14 + 0x26);
  if (localplayer == nullptr) {
    return ms(1000);
  }
  auto samp_module = get_samp_module();
  auto sendrate_ms = *reinterpret_cast<int*>(samp_module + 0xFE0A8);
  if (local_player_only) {
    sendrate_ms = reinterpret_cast<int(__thiscall*)(void*)>(samp_module + 0x039B0)(localplayer);
  }
  return ms(sendrate_ms);
}

CPed *npcs_module::utils::get_samp_player_ped_game_ptr(uint16_t samp_player_id) {
  if (!is_samp_player_exists(samp_player_id))
    return nullptr;
  auto samp_playerpool = get_samp_player_pool();
  if (samp_playerpool == 0) {
    return nullptr;
  }
  if (samp_get_my_id() == samp_player_id) {
    return FindPlayerPed(-1);
  }
  auto samp_module = get_samp_module();
  auto remote_player_ptr = reinterpret_cast<uintptr_t(__thiscall*)(uintptr_t, uint16_t)>(samp_module + 0x10F0)(samp_playerpool, samp_player_id);
  if (remote_player_ptr == 0) {
    return nullptr;
  }
  auto samp_player_cped = *reinterpret_cast<uintptr_t*>(remote_player_ptr);
  if (samp_player_cped == 0) {
    return nullptr;
  }
  return *reinterpret_cast<CPed **>(samp_player_cped + 0x2A4);
}

CVehicle *npcs_module::utils::get_samp_vehicle_game_ptr(uint16_t samp_vehicle_id) {
  if (!is_samp_vehicle_exists(samp_vehicle_id))
    return nullptr;
  return *reinterpret_cast<CVehicle **>(get_samp_vehicle_pool() + 0x4FB4 + (samp_vehicle_id * sizeof(void *)));
}

CEntity *npcs_module::utils::get_samp_object_game_ptr(uint16_t samp_object_id) {
  if (!is_samp_object_exists(samp_object_id))
    return nullptr;
  auto samp_module = get_samp_module();
  auto samp_objectpool = get_samp_object_pool();
  auto samp_object = reinterpret_cast<uintptr_t(__thiscall *)(uintptr_t, uint16_t)>(samp_module + 0x2DC0)(
      samp_objectpool,
      samp_object_id);
  if (samp_object == 0)
    return nullptr;
  return *reinterpret_cast<CEntity **>(samp_object + 0x4 + 0x3C);
}

bool npcs_module::utils::is_samp_chat_opened() {
  auto samp_chat_input_ptr = get_samp_input_ptr();
  if (samp_chat_input_ptr == 0)
    return false;
  return *reinterpret_cast<BOOL*>(samp_chat_input_ptr + 0x14E0) != 0;
}

bool npcs_module::utils::is_any_samp_dialog_opened() {
  auto samp_dialog_ptr = get_samp_dialog_ptr();
  if (samp_dialog_ptr == 0)
    return false;
  return *reinterpret_cast<BOOL*>(samp_dialog_ptr + 0x28) != 0;
}

bool npcs_module::utils::is_sampfuncs_console_opened() {
  auto sf = GetModuleHandleA("SAMPFUNCS.asi");
  if (sf == nullptr)
    return false;
  auto proc = GetProcAddress(sf, "?isConsoleOpened@SAMPFUNCS@@QAE_NXZ");
  if (proc == nullptr)
    return false;
  return reinterpret_cast<bool(__thiscall*)(void*)>(proc)(nullptr);
}

bool npcs_module::utils::can_use_hotkeys() {
  return !is_samp_chat_opened() && !is_any_samp_dialog_opened() && !is_sampfuncs_console_opened();
}

std::string npcs_module::utils::cp1251_to_utf8(const std::string &text) {
  int result_w = MultiByteToWideChar(1251, 0, text.c_str(),
                                     static_cast<int>(text.size()), NULL, 0);
  if (result_w == 0)
    return "";

  std::wstring wres(result_w, '\0');
  if (!MultiByteToWideChar(1251, 0, text.c_str(), static_cast<int>(text.size()),
                           wres.data(), result_w))
    return "";

  int result_c =
      WideCharToMultiByte(CP_UTF8, 0, wres.data(),
                          static_cast<int>(wres.size()), NULL, 0, NULL, NULL);
  if (result_c == 0)
    return "";

  std::string res(result_c, '\0');
  if (!WideCharToMultiByte(CP_UTF8, 0, wres.data(),
                           static_cast<int>(wres.size()), res.data(), result_c,
                           0, 0))
    return "";

  return res;
}

void npcs_module::utils::show_cursor(bool show) {
  using RwD3D9GetCurrentD3DDevice_t = LPDIRECT3DDEVICE9(__cdecl *)();
  using CPad_ClearMouseHistory_t = void (__cdecl *)();
  using CPad_UpdatePads_t = void (__cdecl *)();

  auto rwCurrentD3dDevice = reinterpret_cast<RwD3D9GetCurrentD3DDevice_t>(0x7F9D50U)();

  if (rwCurrentD3dDevice == nullptr) {
    return;
  }

  // CPad::NewMouseControllerState.X
  *reinterpret_cast<float *>(0xB73418 + 0xC) = 0.f;
  // CPad::NewMouseControllerState.Y
  *reinterpret_cast<float *>(0xB73418 + 0x10) = 0.f;

  DWORD updateMouseProtection = PAGE_EXECUTE_READWRITE, rsMouseSetPosProtFirst = PAGE_EXECUTE_READWRITE,
      rsMouseSetPosProtSecond = PAGE_EXECUTE_READWRITE;

  auto update_protection = [&]() {
    ::VirtualProtect(reinterpret_cast<void *>(0x53F3C6U), 5U,
                     updateMouseProtection, &updateMouseProtection);
    ::VirtualProtect(reinterpret_cast<void *>(0x53E9F1U), 5U,
                     rsMouseSetPosProtFirst, &rsMouseSetPosProtFirst);
    ::VirtualProtect(reinterpret_cast<void *>(0x748A1BU), 5U,
                     rsMouseSetPosProtSecond, &rsMouseSetPosProtSecond);
  };

  update_protection();

  if (show) {
    // NOP: CPad::UpdateMouse
    *reinterpret_cast<uint8_t *>(0x53F3C6U) = 0xE9U;
    *reinterpret_cast<uint32_t *>(0x53F3C6U + 1U) = 0x15BU;

    // NOP: RsMouseSetPos
    memset(reinterpret_cast<void *>(0x53E9F1U), 0x90, 5U);
    memset(reinterpret_cast<void *>(0x748A1BU), 0x90, 5U);

    reinterpret_cast<CPad_ClearMouseHistory_t>(0x541BD0U)();
    reinterpret_cast<CPad_UpdatePads_t>(0x541DD0U)();

    rwCurrentD3dDevice->ShowCursor(TRUE);
  } else {
    // Original: CPad::UpdateMouse
    memcpy(reinterpret_cast<void *>(0x53F3C6U), "\xE8\x95\x6C\x20\x00", 5U);

    // Original: RsMouseSetPos
    memcpy(reinterpret_cast<void *>(0x53E9F1U), "\xE8\xAA\xAA\x0D\x00", 5U);
    memcpy(reinterpret_cast<void *>(0x748A1BU), "\xE8\x80\x0A\xED\xFF", 5U);

    reinterpret_cast<CPad_ClearMouseHistory_t>(0x541BD0U)();
    reinterpret_cast<CPad_UpdatePads_t>(0x541DD0U)();

    rwCurrentD3dDevice->ShowCursor(FALSE);
  }

  update_protection();
}

std::filesystem::path npcs_module::utils::get_appdata_path() {
  PWSTR path = NULL;
  if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &path)) && path != NULL) {
    return path;
  }
  return L"";
}

std::filesystem::path npcs_module::utils::get_config_file_path() {
  if (const auto appdata_path = get_appdata_path(); !appdata_path.empty()) {
    return appdata_path / L"Pears Project" / L"Game" / L"npcs_settings.json";
  }
  return L"";
}

HWND npcs_module::utils::get_game_hwnd() {
  auto game_hwnd = *reinterpret_cast<HWND **>(0xC17054);
  if (game_hwnd == nullptr) {
    return NULL;
  }
  return *game_hwnd;
}

LPDIRECT3DDEVICE9 npcs_module::utils::get_d3d_device() {
  return *reinterpret_cast<IDirect3DDevice9 **>(0xC97C28);
}

void npcs_module::utils::unprotect_and_run(uintptr_t address, size_t size, const std::function<void(uint8_t *)> &callback) {
  DWORD old_protection = PAGE_EXECUTE_READWRITE;
  VirtualProtect(reinterpret_cast<LPVOID>(address), size, old_protection, &old_protection);
  callback(reinterpret_cast<uint8_t *>(address));
  VirtualProtect(reinterpret_cast<LPVOID>(address), size, old_protection, &old_protection);
}

void npcs_module::utils::nop_range(uintptr_t start, uintptr_t end) {
  const auto size = end - start;
  unprotect_and_run(start, size, [size](const auto start) {
    memset(reinterpret_cast<void *>(start), 0x90, size);
  });
}

#ifdef KTHOOK_32
void npcs_module::utils::nop_and_naked_hook(uintptr_t start,
                        uintptr_t end,
                        const std::function<void(const kthook::kthook_naked &)> &detour) {
  static std::vector<std::unique_ptr<kthook::kthook_naked>> naked_hooks;

  nop_range(start, end);

  auto &hook = naked_hooks.emplace_back(std::make_unique<kthook::kthook_naked>());
  hook->set_cb(detour);
  hook->set_dest(start);
  hook->install();
}
#endif
