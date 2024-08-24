#include <Server/Components/Pawn/impl/pawn_natives.hpp>

#include "NpcComponent.h"

const int INVALID_NPC_ID = 0xFFFF;

SCRIPT_API_FAILRET(CreateNpc, INVALID_NPC_ID, int(const int skin, Vector3 pos)) {
  auto stream = NpcComponent::instance().create(skin, pos);
  if (stream != nullptr) {
    return stream->getID();
  }
  return FailRet;
}

SCRIPT_API(DestroyNpc, bool(INpc &npc)) {
  NpcComponent::instance().release(npc.getID());
  return true;
}

SCRIPT_API(IsValidNpc, bool(INpc *npc)) {
  return npc != nullptr;
}

///////////////

SCRIPT_API(IsNpcStreamedIn, bool(INpc &npc, IPlayer &player)) {
  return npc.isStreamedInForPlayer(player);
}

///////////////

SCRIPT_API(GetNpcVirtualWorld, int(INpc &npc)) {
  return npc.getVirtualWorld();
}

SCRIPT_API(SetNpcVirtualWorld, bool(INpc &npc, int world)) {
  npc.setVirtualWorld(world);
  return true;
}

///////////////

SCRIPT_API(GetNpcPosition, bool(INpc &npc, Vector3 &out)) {
  out = npc.getPosition();
  return true;
}

SCRIPT_API(SetNpcPosition, bool(INpc &npc, Vector3 pos)) {
  npc.setPosition(pos);
  return true;
}

///////////////

SCRIPT_API(GetNpcFacingAngle, bool(INpc &npc, float &angle)) {
  angle = npc.getRotation().ToEuler().z;
  return true;
}

SCRIPT_API(SetNpcFacingAngle, bool(INpc &npc, float angle)) {
  auto rotation = npc.getRotation().ToEuler();
  rotation.z = angle;
  npc.setRotation(rotation);
  return true;
}

///////////////

SCRIPT_API(IsNpcInvulnerable, bool(INpc &npc)) {
  return npc.isInvulnerable();
}

SCRIPT_API(SetNpcInvulnerable, bool(INpc &npc, bool invulnerable)) {
  npc.setInvulnerable(invulnerable);
  return true;
}

///////////////

SCRIPT_API(IsNpcStunAnimationEnabled, bool(INpc &npc)) {
  return npc.isStunAnimationEnabled();
}

SCRIPT_API(SetNpcStunAnimationEnabled, bool(INpc &npc, bool enabled)) {
  npc.setStunAnimationEnabled(enabled);
  return true;
}

///////////////

SCRIPT_API(GetNpcHealth, bool(INpc &npc, float& health)) {
  health = npc.getHealth();
  return true;
}

SCRIPT_API(SetNpcHealth, bool(INpc &npc, float health)) {
  npc.setHealth(health);
  return true;
}

///////////////

SCRIPT_API(GetNpcSkin, int(INpc &npc)) {
  return npc.getSkin();
}

SCRIPT_API(SetNpcSkin, bool(INpc &npc, int skinId)) {
  npc.setSkin(skinId);
  return true;
}

///////////////

SCRIPT_API(GetNpcWeapon, int(INpc &npc)) {
  return npc.getWeapon();
}

SCRIPT_API(SetNpcWeapon, bool(INpc &npc, uint8_t id)) {
  npc.setWeapon(id);
  return true;
}

SCRIPT_API(SetNpcWeaponShootingAccuracy, bool(INpc &npc, uint8_t accuracy)) {
  npc.setWeaponShootingAccuracy(accuracy);
  return true;
}

SCRIPT_API(SetNpcWeaponShootingRate, bool(INpc &npc, uint8_t shootingrate)) {
  npc.setWeaponShootingRate(shootingrate);
  return true;
}

SCRIPT_API(SetNpcWeaponSkill, bool(INpc &npc, uint8_t skill)) {
  if (skill < NpcWeaponSkillType_Poor || skill > NpcWeaponSkillType_Pro) {
    return false;
  }
  npc.setWeaponSkill(NpcWeaponSkillType(skill));
  return true;
}

///////////////

SCRIPT_API(TaskNpcStandStill, bool(INpc &npc)) {
  npc.standStill();
  return true;
}

SCRIPT_API(TaskNpcAttackPlayer, bool(INpc &npc, IPlayer &target, bool aggressive)) {
  npc.attackPlayer(target, aggressive);
  return true;
}

SCRIPT_API(TaskNpcGoToPoint, bool(INpc &npc, Vector3 destination, int mode)) {
  if (mode < NpcMoveMode_Walk || mode > NpcMoveMode_Sprint) {
    return false;
  }
  npc.goToPoint(destination, NpcMoveMode(mode));
  return true;
}

SCRIPT_API(TaskNpcFollowPlayer, bool(INpc &npc, IPlayer &target)) {
  npc.followPlayer(target);
  return true;
}

///////////////

// Npcs param lookup
namespace pawn_natives {
#define CUSTOM_POOL_PARAM(type)                             \
    template <>                                             \
	struct ParamLookup<type>                                \
	{                                                       \
		static type* Val(cell ref) noexcept                 \
		{                                                   \
            return NpcComponent::instance().get(ref);       \
		}                                                   \
	};                                                      \
                                                            \
	template <>                                             \
	class ParamCast<type*>                                  \
	{                                                       \
	public:                                                 \
		ParamCast(AMX* amx, cell* params, int idx) noexcept \
		{                                                   \
			value_ = ParamLookup<type>::Val(params[idx]);   \
		}                                                   \
                                                            \
		~ParamCast()                                        \
		{                                                   \
		}                                                   \
                                                            \
		ParamCast(ParamCast<type*> const&) = delete;        \
		ParamCast(ParamCast<type*>&&) = delete;             \
                                                            \
		operator type*()                                    \
		{                                                   \
			return value_;                                  \
		}                                                   \
                                                            \
		bool Error() const                                  \
		{                                                   \
			return false;                                   \
		}                                                   \
                                                            \
		static constexpr int Size = 1;                      \
                                                            \
	private:                                                \
		type* value_;                                       \
	};                                                      \
                                                            \
	template <>                                             \
	class ParamCast<type&>                                  \
	{                                                       \
	public:                                                 \
		ParamCast(AMX* amx, cell* params, int idx)          \
		{                                                   \
			value_ = ParamLookup<type>::Val(params[idx]);   \
			if (value_ == nullptr)                          \
			{                                               \
				error_ = true;                              \
			}                                               \
		}                                                   \
                                                            \
		~ParamCast()                                        \
		{                                                   \
		}                                                   \
                                                            \
		ParamCast(ParamCast<type&> const&) = delete;        \
		ParamCast(ParamCast<type&>&&) = delete;             \
                                                            \
		operator type&()                                    \
		{                                                   \
			return *value_;                                 \
		}                                                   \
                                                            \
		bool Error() const                                  \
		{                                                   \
			return error_;                                  \
		}                                                   \
                                                            \
		static constexpr int Size = 1;                      \
                                                            \
	private:                                                \
		type* value_;                                       \
		bool error_ = false;                                \
	};                                                      \
                                                            \
	template <>                                             \
	class ParamCast<const type&>                            \
	{                                                       \
	public:                                                 \
		ParamCast(AMX*, cell*, int) = delete;               \
		ParamCast() = delete;                               \
	};

CUSTOM_POOL_PARAM(INpc);
};
