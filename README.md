# SAMP NPCs

> In case you wanted to make smart interactive NPCs on your SA-MP server

---

## Overview

This project is designed to enable the creation of smart, interactive NPCs for SA-MP (San Andreas Multiplayer) servers. It provides a many functions for defining NPC behavior, interactions, and movement logic, allowing NPCs to feel responsive and purposeful rather than static, default Actors or builtin SAMP players behaving like NPCs. The goal of this project is to help servers developers create more interesting quests, missions, by introducing NPCs that can react to players, perform tasks, follow paths, and simulate basic decision-making things. Everything's based on a game (San Andreas) builtin peds logic.

## Features

* Immersive NPCs creation rather than simple Actors.
* Many tasks for NPCs, like attacking players, npcs, attacking players vehicles, following players and so on.
* Pathfinding and movement control for realistic navigation
* Player interaction support (detection, reaction, engagement)
* NPC-to-NPC interaction and coordination
* Dynamic behavior switching based on context (fight, stand still, climb obstacles, buildings).
* Exposable SDK for components based on open.mp sdk.

## Getting Started

### Prerequisites

* [plugin-sdk](https://github.com/DK22Pac/plugin-sdk)
* SA-MP 0.3.7-R3

### Building

```bash
# Clone the repository
git clone https://github.com/Northn/samp_npcs --recursive

# Navigate into the project directory
cd samp_npcs

# Generate a project
cmake -B build -DBUILD_CLIENT=YES -A Win32

# Build a project
cmake --build build --config RelWithDebInfo --parallel
```

## Usage

```pawn
static NPC:created_npcs[1024] = {INVALID_NPC, ...};

stock bool:IsValidNpcSlot(slotid) {
    return slotid >= 0 && slotid < sizeof(created_npcs);
}

stock bool:IsValidNpcInSlot(slotid) {
    return IsValidNpcSlot(slotid) && IsValidNpc(created_npcs[slotid]);
}

stock NPC:GetNpcInSlot(slotid) {
    return IsValidNpcInSlot(slotid) ? created_npcs[slotid] : INVALID_NPC;
}

stock FindAvailableNpcSlot() {
    for (new i = 0; i < sizeof(created_npcs); ++i) {
        if (!IsValidNpcInSlot(i)) {
            return i;
        }
    }
    return -1;
}

stock CreateNpcSlot(skinid, Float:x, Float:y, Float:z, world) {
    new available_slot = FindAvailableNpcSlot();
    if (available_slot == -1) return -1;
    created_npcs[available_slot] = CreateNpc(skinid, x, y, z);
    SetNpcVirtualWorld(created_npcs[available_slot], world);
    return available_slot;
}

stock bool:DestroyNpcSlot(slotid) {
    if (!IsValidNpcInSlot(slotid)) return false;

    DestroyNpc(created_npcs[slotid]);
    created_npcs[slotid] = INVALID_NPC;
    return true;
}

#define ASSERT_NPC_EXISTS(%0,%1)                                                            \
    new NPC:%0 = GetNpcInSlot(%1);                                                          \
    if (npc == INVALID_NPC) {                                                               \
        SendClientMessage(playerid, COLOR_GREY, "NPC does not exist.");                     \
        return 1;                                                                           \
    }

CMD:create_npc(playerid, const params[]) {
    new skinid = 0;
    if (sscanf(params, "i", skinid)) {
        SendClientMessage(playerid, COLOR_GREY, "* Create an interactive NPC [ /create_npc Skin ]");
        return 1;
    }

    new Float:x, Float:y, Float:z;
    GetPlayerPos(playerid, x, y, z);

    x += 0.25;
    y += 0.25;
    z += 0.25;
    
    new npc_slot = CreateNpcSlot(skinid, x, y, z, GetPlayerVirtualWorld(playerid));
    if (!IsValidNpcInSlot(npc_slot)) {
        SendClientMessage(playerid, COLOR_GREY, "* NPC buffer is full");
        SendClientMessage(playerid, COLOR_GREY, "* Clear them using: [ /clear_npcs ]");
        return 1;
    }

    SendClientMessage(playerid, COLOR_GREY, "* NPC created! ID: %d", npc_slot);
    return 1;
}

CMD:npc_attack(playerid, const params[]) {
    new npcid, targetid, bool: aggressive;
    if (sscanf(params, "iiI(0)", npcid, targetid, aggressive)) {
        SendClientMessage(playerid, COLOR_GREY, "* Order an NPC to attack a player [ /npc_attack NPC_ID Target_ID Aggression* ]");
        return 1;
    }

    if (!IsPlayerConnected(targetid) || IsPlayerNPC(targetid)) {
        SendClientMessage(playerid, COLOR_GREY, "* Target is not online...");
        return 1;
    }
    
    if (npcid != -1) {
        ASSERT_NPC_EXISTS(npc, npcid)
        TaskNpcAttackPlayer(npc, targetid, aggressive);
    } else {
        for (new i = 0; i < sizeof(created_npcs); ++i) {
            if (!IsValidNpcInSlot(i)) continue;
            new NPC:npc = GetNpcInSlot(i);
            new Float:npc_x, Float:npc_y, Float:npc_z;
            GetNpcPosition(npc, npc_x, npc_y, npc_z);
            if (GetPlayerDistanceFromPoint(playerid, npc_x, npc_y, npc_z) <= 30.0) {
                TaskNpcAttackPlayer(npc, targetid, aggressive);
            }
        }
    }

    SendClientMessage(playerid, COLOR_GREY, "* Done!");
    return 1;
}

CMD:npc_stand(playerid, const params[]) {
    new npcid;
    if (sscanf(params, "i", npcid)) {
        SendClientMessage(playerid, COLOR_GREY, "* Order an NPC to stand still [ /npc_stand NPC_ID ]");
        return 1;
    }
    
    if (npcid != -1) {
        ASSERT_NPC_EXISTS(npc, npcid)
        TaskNpcStandStill(npc);
    } else {
        for (new i = 0; i < sizeof(created_npcs); ++i) {
            if (!IsValidNpcInSlot(i)) continue;
            new NPC:npc = GetNpcInSlot(i);
            new Float:npc_x, Float:npc_y, Float:npc_z;
            GetNpcPosition(npc, npc_x, npc_y, npc_z);
            if (GetPlayerDistanceFromPoint(playerid, npc_x, npc_y, npc_z) <= 30.0) {
                TaskNpcStandStill(npc);
            }
        }
    }
    
    SendClientMessage(playerid, COLOR_GREY, "* Done!");
    return 1;
}

CMD:npc_go_here(playerid, const params[]) {
    new npcid, mode;
    if (sscanf(params, "ii", npcid, mode) || !(mode >= 0 && mode <= 2)) {
        SendClientMessage(playerid, COLOR_GREY, "* Order an NPC to come to you [ /npc_go_here NPC_ID Mode ]");
        SendClientMessage(playerid, COLOR_GREY, "* Mode: 0 - walk, 1 - light run, 2 - sprint");
        return 1;
    }
    
    new Float:x, Float:y, Float:z;
    GetPlayerPos(playerid, x, y, z);

    if (npcid != -1) {
        ASSERT_NPC_EXISTS(npc, npcid)
        TaskNpcGoToPoint(npc, x, y, z, NPC_MOVE_MODE:mode);
    } else {
        for (new i = 0; i < sizeof(created_npcs); ++i) {
            if (!IsValidNpcInSlot(i)) continue;
            new NPC:npc = GetNpcInSlot(i);
            new Float:npc_x, Float:npc_y, Float:npc_z;
            GetNpcPosition(npc, npc_x, npc_y, npc_z);
            if (GetPlayerDistanceFromPoint(playerid, npc_x, npc_y, npc_z) <= 30.0) {
                TaskNpcGoToPoint(npc, x, y, z, NPC_MOVE_MODE:mode);
            }
        }
    }
    
    SendClientMessage(playerid, COLOR_GREY, "* Done!");
    return 1;
}

CMD:npc_go_random(playerid, const params[]) {
    new npcid, mode;
    if (sscanf(params, "ii", npcid, mode) || !(mode >= 0 && mode <= 2)) {
        SendClientMessage(playerid, COLOR_GREY, "* Order an NPC to move randomly around you [ /npc_go_random NPC_ID Mode ]");
        SendClientMessage(playerid, COLOR_GREY, "* Mode: 0 - walk, 1 - light run, 2 - sprint");
        return 1;
    }

    if (npcid != -1) {
        new Float:x, Float:y, Float:z;
        GetPlayerPos(playerid, x, y, z);

        x += 10 + random(15);
        y += 10 + random(15);

        ASSERT_NPC_EXISTS(npc, npcid)
        TaskNpcGoToPoint(npc, x, y, z, NPC_MOVE_MODE:mode);
    } else {
        for (new i = 0; i < sizeof(created_npcs); ++i) {
            if (!IsValidNpcInSlot(i)) continue;

            new Float:x, Float:y, Float:z;
            GetPlayerPos(playerid, x, y, z);

            x += 10 + random(15);
            y += 10 + random(15);

            new NPC:npc = GetNpcInSlot(i);
            new Float:npc_x, Float:npc_y, Float:npc_z;
            GetNpcPosition(npc, npc_x, npc_y, npc_z);
            if (GetPlayerDistanceFromPoint(playerid, npc_x, npc_y, npc_z) <= 30.0) {
                TaskNpcGoToPoint(npc, x, y, z, NPC_MOVE_MODE:mode);
            }
        }
    }
    
    SendClientMessage(playerid, COLOR_GREY, "* Done!");
    return 1;
}

CMD:npc_follow(playerid, const params[]) {
    new npcid, targetid;
    if (sscanf(params, "ii", npcid, targetid)) {
        SendClientMessage(playerid, COLOR_GREY, "* Order an NPC to follow a player [ /npc_follow NPC_ID Target_ID ]");
        return 1;
    }

    if (!IsPlayerConnected(targetid) || IsPlayerNPC(targetid)) {
        SendClientMessage(playerid, COLOR_GREY, "* Target is not online...");
        return 1;
    }
    
    if (npcid != -1) {
        ASSERT_NPC_EXISTS(npc, npcid)
        TaskNpcFollowPlayer(npc, targetid);
    } else {
        for (new i = 0; i < sizeof(created_npcs); ++i) {
            if (!IsValidNpcInSlot(i)) continue;
            new NPC:npc = GetNpcInSlot(i);
            new Float:npc_x, Float:npc_y, Float:npc_z;
            GetNpcPosition(npc, npc_x, npc_y, npc_z);
            if (GetPlayerDistanceFromPoint(playerid, npc_x, npc_y, npc_z) <= 30.0) {
                TaskNpcFollowPlayer(npc, targetid);
            }
        }
    }

    SendClientMessage(playerid, COLOR_GREY, "* Done!");
    return 1;
}

CMD:npc_sethp(playerid, const params[]) {
    new npcid, Float:amount;
    if (sscanf(params, "if", npcid, amount)) {
        SendClientMessage(playerid, COLOR_GREY, "* Set NPC health amount [ /npc_sethp NPC_ID Health ]");
        return 1;
    }
    
    if (npcid != -1) {
        ASSERT_NPC_EXISTS(npc, npcid)
        SetNpcHealth(npc, amount);
    } else {
        for (new i = 0; i < sizeof(created_npcs); ++i) {
            if (!IsValidNpcInSlot(i)) continue;
            new NPC:npc = GetNpcInSlot(i);
            new Float:npc_x, Float:npc_y, Float:npc_z;
            GetNpcPosition(npc, npc_x, npc_y, npc_z);
            if (GetPlayerDistanceFromPoint(playerid, npc_x, npc_y, npc_z) <= 30.0) {
                SetNpcHealth(npc, amount);
            }
        }
    }
    
    SendClientMessage(playerid, COLOR_GREY, "* Done!");
    return 1;
}

CMD:npc_setweapon(playerid, const params[]) {
    new npcid, weaponid;
    if (sscanf(params, "ii", npcid, weaponid)) {
        SendClientMessage(playerid, COLOR_GREY, "* Give a weapon to an NPC [ /npc_setweapon NPC_ID Weapon_ID ]");
        return 1;
    }

    if (npcid != -1) {
        ASSERT_NPC_EXISTS(npc, npcid)
        SetNpcWeapon(npc, WEAPON:weaponid);
    } else {
        for (new i = 0; i < sizeof(created_npcs); ++i) {
            if (!IsValidNpcInSlot(i)) continue;
            new NPC:npc = GetNpcInSlot(i);
            new Float:npc_x, Float:npc_y, Float:npc_z;
            GetNpcPosition(npc, npc_x, npc_y, npc_z);
            if (GetPlayerDistanceFromPoint(playerid, npc_x, npc_y, npc_z) <= 30.0) {
                SetNpcWeapon(npc, WEAPON:weaponid);
            }
        }
    }
    
    SendClientMessage(playerid, COLOR_GREY, "* Done!");
    return 1;
}

alias:npc_delete("npc_remove", "npc_clear", "npc_destroy")
CMD:npc_delete(playerid, const params[]) {
    new npcid;
    if (sscanf(params, "i", npcid)) {
        SendClientMessage(playerid, COLOR_GREY, "* Delete an interactive NPC [ /npc_delete NPC_ID ]");
        return 1;
    }
    
    if (npcid != -1) {
        ASSERT_NPC_EXISTS(npc, npcid)
        DestroyNpcSlot(npcid);
    } else {
        for (new i = 0; i < sizeof(created_npcs); ++i) {
            if (!IsValidNpcInSlot(i)) continue;
            new NPC:npc = GetNpcInSlot(i);
            new Float:npc_x, Float:npc_y, Float:npc_z;
            GetNpcPosition(npc, npc_x, npc_y, npc_z);
            if (GetPlayerDistanceFromPoint(playerid, npc_x, npc_y, npc_z) <= 30.0) {
                DestroyNpcSlot(i);
            }
        }
    }
    
    SendClientMessage(playerid, COLOR_GREY, "* Done!");
    return 1;
}

CMD:clear_npcs(playerid, const params[]) {
    new amount = 0;
    for (new i = 0; i < sizeof(created_npcs); ++i) {
        if (DestroyNpcSlot(i)) {
            ++amount;
        }
    }
    
    SendClientMessage(playerid, COLOR_GREY, "* Done! Removed: %d", amount);
    return 1;
}

#undef ASSERT_NPC_EXISTS

public bool:OnPlayerGiveDamageNpc(NPC:npc, damagerid, Float:amount, weaponid, bodypart)
{
    return true;
}

public bool:OnNpcGiveDamageNpc(NPC:npc, NPC:damager, Float:amount, weaponid, bodypart)
{
    return true;
}

public OnPlayerTakeDamageNpc(NPC:npc, issuerid, Float:amount, weaponid, bodypart)
{
    return 1;
}

public OnNpcDeath(NPC:npc, killerid, reason)
{
    return 1;
}
```

## Contributing

Contributions are welcomed. Feel free to fork the repository, commit changes and open a Pull Request!

## Links
BlastHack thread: https://www.blast.hk/threads/247132/    
open.mp: https://github.com/openmultiplayer/open.mp/    
plugin-sdk: https://github.com/DK22Pac/plugin-sdk
