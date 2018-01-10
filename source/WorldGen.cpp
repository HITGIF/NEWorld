// 
// NEWorld: WorldGen.cpp
// NEWorld: A Free Game with Similar Rules to Minecraft.
// Copyright (C) 2015-2018 NEWorld Team
// 
// NEWorld is free software: you can redistribute it and/or modify it 
// under the terms of the GNU Lesser General Public License as published
// by the Free Software Foundation, either version 3 of the License, or 
// (at your option) any later version.
// 
// NEWorld is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
// or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General 
// Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with NEWorld.  If not, see <http://www.gnu.org/licenses/>.
// 

#include "WorldGen.h"
#include "Definitions.h"

//Perlin Noise 2D
namespace WorldGen {
    double perm[256];
    int seed;
    double NoiseScaleX = 64;
    double NoiseScaleZ = 64;
    int WaterLevel = 30;

    void perlinNoiseInit(int mapseed) {
        fastSrand(mapseed);
        for (double& i : perm) { i = rnd() * 256.0; }
        seed = mapseed;
    }

    double SmoothedNoise(int x, int y) {
        double corners, sides, center;
        corners = (Noise(x - 1, y - 1) + Noise(x + 1, y - 1) + Noise(x - 1, y + 1) + Noise(x + 1, y + 1)) / 8.0;
        sides = (Noise(x - 1, y) + Noise(x + 1, y) + Noise(x, y - 1) + Noise(x, y + 1)) / 4.0;
        center = Noise(x, y);
        return corners + sides + center;
    }

    double InterpolatedNoise(double x, double y) {
        int int_X, int_Y;
        double fractional_X, fractional_Y, v1, v2, v3, v4, i1, i2;
        int_X = (int)floor(x); //不要问我为毛用floor，c++默认居然TM的是向零取整的
        fractional_X = x - int_X;
        int_Y = (int)floor(y);
        fractional_Y = y - int_Y;
        v1 = Noise(int_X, int_Y);
        v2 = Noise(int_X + 1, int_Y);
        v3 = Noise(int_X, int_Y + 1);
        v4 = Noise(int_X + 1, int_Y + 1);
        i1 = Interpolate(v1, v2, fractional_X);
        i2 = Interpolate(v3, v4, fractional_X);
        return Interpolate(i1, i2, fractional_Y);
    }

    double PerlinNoise2D(double x, double y) {
        double total = 0, frequency = 1, amplitude = 1;
        for (int i = 0; i <= 4; i++) {
            total += InterpolatedNoise(x * frequency, y * frequency) * amplitude;
            frequency *= 2;
            amplitude /= 2.0;
        }
        return total;
    }
}
