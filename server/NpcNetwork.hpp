#pragma once

struct NpcControlRpc : NetworkPacketBase<NpcComponent::kNpcControlRpcId, NetworkPacketType::RPC, OrderingChannel_SyncRPC> {
  enum NpcControlRpcType {
    NpcControlRpcType_StreamIn,
    NpcControlRpcType_StreamOut,

    NpcControlRpcType_GiveDamage,
    NpcControlRpcType_TakeDamage,

    NpcControlRpcType_SetActiveTask,
  };

  NpcControlRpcType Type;

  uint16_t NpcID;

  struct {
    int Skin;
    Vector3 Position;
    float Heading;

    float Health;
    bool StunEnabled;

    uint8_t WeaponID;
    uint8_t WeaponShootingAccuracy;
    uint8_t WeaponShootingRate;
    NpcWeaponSkillType WeaponSkill;

    NpcTasksSet Task;
  } StreamIn;

  struct {
    float Damage;
    uint8_t WeaponID;
    uint8_t Bodypart;
  } GiveTakeDamage;

  bool read(NetworkBitStream& bs) {
    { // Reading a type
      int type_;
      if (!bs.readUINT8(type_))
        return false;

      Type = static_cast<NpcControlRpcType>(type_);
    }

    if (Type != NpcControlRpcType_GiveDamage && Type != NpcControlRpcType_TakeDamage) return false;

    if (!bs.readUINT16(NpcID)) return false;
    if (!bs.readFLOAT(GiveTakeDamage.Damage)) return false;
    if (!bs.readUINT8(GiveTakeDamage.WeaponID)) return false;
    if (!bs.readUINT8(GiveTakeDamage.Bodypart)) return false;

    return true;
  }

  void write(NetworkBitStream& bs) const {
    bs.writeUINT8(int(Type));
    bs.writeUINT16(NpcID);

    if (Type == NpcControlRpcType_StreamIn) {
      bs.writeUINT16(StreamIn.Skin);
      bs.writeVEC3(StreamIn.Position);
      bs.writeFLOAT(StreamIn.Heading);
      bs.writeFLOAT(StreamIn.Health);
      bs.writeUINT8(StreamIn.StunEnabled ? 1 : 0);

      bs.writeUINT8(StreamIn.WeaponID);
      bs.writeUINT8(StreamIn.WeaponShootingAccuracy);
      bs.writeUINT8(StreamIn.WeaponShootingRate);
      bs.writeUINT8(static_cast<uint8_t>(StreamIn.WeaponSkill));

      std::visit([&bs](const auto &task) { task.writeInternal(bs); }, StreamIn.Task);
    } else if (Type == NpcControlRpcType_StreamOut) {
      // Nothing to do
    } else if (Type == NpcControlRpcType_SetActiveTask) {
      std::visit([&bs](const auto &task) { task.writeInternal(bs); }, StreamIn.Task);
    }
  }
};

struct NpcSyncPacket : NetworkPacketBase<NpcComponent::kNpcSyncPacketId, NetworkPacketType::Packet, OrderingChannel_SyncPacket> {
  uint16_t NpcID;

  Vector3 Position;
  float Heading;

  float Health;

  bool read(NetworkBitStream& bs) {
    if (!bs.readUINT16(NpcID)) return false;
    if (!bs.readPosVEC3(Position)) return false;
    if (!bs.readFLOAT(Heading)) return false;
    if (!(Heading >= 0.f && Heading <= 360.f)) return false;

    return true;
  }

  void write(NetworkBitStream& bs) const {
    bs.writeUINT8(PacketID);
    bs.writeUINT16(NpcID);
    bs.writeVEC3(Position);
    bs.writeFLOAT(Heading);
    bs.writeFLOAT(Health);
  }
};
