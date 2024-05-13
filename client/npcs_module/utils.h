#pragma once

namespace npcs_module::utils {
bool is_game_foreground();
bool is_pause_menu_active();
CVector get_ped_velocity(CPed *ped);
CVehicle *get_ped_vehicle(CPed *ped);

uintptr_t get_samp_module();
uintptr_t get_samp_netgame();
RakClientInterface *get_samp_rakclient_intf();

bool samp_is_playing();

uintptr_t get_samp_player_pool();
uintptr_t get_samp_vehicle_pool();
uintptr_t get_samp_object_pool();
uintptr_t get_samp_input_ptr();
uintptr_t get_samp_dialog_ptr();

bool is_samp_player_exists(uint16_t samp_player_id);
bool is_samp_vehicle_exists(uint16_t samp_vehicle_id);
bool is_samp_object_exists(uint16_t samp_object_id);

uint16_t samp_get_my_id();
std::string samp_get_player_nickname(uint16_t samp_player_id);
uint16_t samp_get_player_ping(uint16_t samp_player_id);
float samp_get_player_health(uint16_t samp_player_id);
float samp_get_player_armor(uint16_t samp_player_id);
bool samp_is_player_streamed_in(uint16_t samp_player_id);
bool samp_is_player_afk(uint16_t samp_player_id);
void samp_update_players_pings_scores();

std::chrono::milliseconds samp_get_onfoot_sync_rate(bool local_player_only = false);

CPed *get_samp_player_ped_game_ptr(uint16_t samp_player_id);
CVehicle *get_samp_vehicle_game_ptr(uint16_t samp_vehicle_id);
CEntity *get_samp_object_game_ptr(uint16_t samp_object_id);

bool is_samp_chat_opened();
bool is_any_samp_dialog_opened();
bool is_sampfuncs_console_opened();

bool can_use_hotkeys();

HWND get_game_hwnd();
LPDIRECT3DDEVICE9 get_d3d_device();
std::string cp1251_to_utf8(const std::string &text);
void show_cursor(bool show);
std::filesystem::path get_appdata_path();
std::filesystem::path get_config_file_path();

// Hook helpers
void unprotect_and_run(uintptr_t address, size_t size, const std::function<void(uint8_t *)> &callback);
void nop_range(uintptr_t start, uintptr_t end);
#ifdef KTHOOK_32
void nop_and_naked_hook(uintptr_t start,
                        uintptr_t end,
                        const std::function<void(const kthook::kthook_naked &)> &detour);
#endif
}
