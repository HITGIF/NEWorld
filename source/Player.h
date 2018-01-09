/*
* NEWorld: A free game with similar rules to Minecraft.
* Copyright (C) 2017-2018 NEWorld Team
*
* This file is part of NEWorld.
* NEWorld is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* NEWorld is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with NEWorld.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once
#include "Definitions.h"

const double g = 9.8;
const double EDrop = 0.1;
const double speedCast = 1 / 20.0;

struct PlayerPacket;

namespace Hitbox {
    struct AABB;
}

class Frustum;

class Player {
public:
    static void init(double x, double y, double z);
    static void spawn();

    static void updatePosition();

    static bool save(const std::string& worldn);
    static bool load(const std::string& worldn);

    static bool addItem(item itemname, short amount = 1);
    static bool putBlock(int x, int y, int z, Block blockname);

    //修改游戏模式
    static void changeGameMode(int gamemode);

    static PlayerPacket convertToPlayerPacket();
    static Hitbox::AABB playerbox;
    static std::vector<Hitbox::AABB> Hitboxes;
    static double xa, ya, za, xd, yd, zd;
    static double health, healthmax, healSpeed, dropDamage;
    static onlineid onlineID;
    static std::string name;
    static Frustum ViewFrustum;

    enum GameMode { Survival, Creative };

    static int gamemode;
    static bool Glide;
    static bool Flying;
    static bool CrossWall;
    static double glidingMinimumSpeed;

    static bool OnGround;
    static bool Running;
    static bool NearWall;
    static bool inWater;
    static bool glidingNow;

    static double speed;
    static int AirJumps;
    static int cxt, cyt, czt, cxtl, cytl, cztl;
    static double lookupdown, heading, xpos, ypos, zpos, xposold, yposold, zposold, jump;
    static double xlookspeed, ylookspeed;
    static int intxpos, intypos, intzpos, intxposold, intyposold, intzposold;

    static float height;
    static float heightExt;

    static item BlockInHand;
    static uint8_t indexInHand;
    static item inventory[4][10];
    static short inventoryAmount[4][10];

    static double glidingEnergy, glidingSpeed;
};
