#pragma once

namespace npcs_module {
class npc {
  // Task position to put all our tasks to
  static constexpr auto kTaskIdPlaceTo = TASK_PRIMARY_PRIMARY;

  static constexpr auto kInvalidTargetId = 0xFFFF;

  std::unique_ptr<CCivilianPed> ped = nullptr;
  std::chrono::steady_clock::time_point last_sync_send;
  std::chrono::steady_clock::time_point last_sync_send_check;
  CVector last_send_sync_pos;
  bool sent_sync_once = false;

  uint16_t my_id = 0; // 0 is an invalid npc id
  uint16_t player_attack_to = kInvalidTargetId;
  bool aggressive_attack = false;
  uint16_t player_follow_to = kInvalidTargetId;

  bool stun_enabled = true;
public:
  enum class npc_move_mode_t {
    kWalk,
    kRun,
    kSprint
  };

  npc(uint16_t id, uint16_t model_id, const CVector &position);
  ~npc();

  npc(npc &) = delete;
  npc(npc &&) = default;

  npc &operator=(const npc &) = delete;
  npc &operator=(npc &&) = default;

  void set_stun_enabled(bool enabled);
  void set_position(const CVector &position);
  void set_heading(float heading);
  void set_weapon_accuracy(uint8_t accuracy);
  void set_weapon_shooting_rate(uint8_t shooting_rate);
  void set_weapon_skill(uint8_t skill);
  void set_current_weapon(uint8_t weapon_id, uint32_t ammo, uint16_t ammo_in_clip = 0xFFFF);
  void set_health(float health);

  void update();
  void update_from_sync(const struct npc_sync_receive_data_t &data);
  bool send_sync_if_required();

  // Tasks
  void attack_player(uint16_t samp_player_id, bool aggressive = false);
  void follow_player(uint16_t samp_player_id);
  void stand_still();
  void wander();
  void go_to_point(const CVector &point, npc_move_mode_t mode = npc_move_mode_t::kRun);

  bool is_stun_enabled() const;
  bool is_aggressive_attack() const;
  CPed *get_ped() const;
private:
  std::chrono::milliseconds get_sync_send_rate() const;
  bool is_dead() const;
  float get_heading() const;

  void send_sync();

  // Follow task helpers
  bool is_following_target() const;
  bool should_follow_target() const;
  uint16_t get_follow_target() const;

  // Attack task helpers
  bool is_attacking_target() const;
  bool should_attack_target() const;
  uint16_t get_attack_target() const;

  eTaskType get_active_task_type() const;
  CTask *get_active_task() const;
  void clear_active_task(bool immediately = false);
  void set_current_task(CTask *task);

  CWeapon *get_current_weapon();

  bool is_ped_valid() const;

  static bool is_weapon_model(int model_id);
  static bool is_ped_model(int model_id);
  static bool is_model_available(int model_id);
  static bool is_model_loaded(int model_id);
  static bool request_model(int model_id);
  static void release_model(int model_id);
};
}
