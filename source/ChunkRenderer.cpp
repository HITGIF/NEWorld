// 
// NEWorld: ChunkRenderer.cpp
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

#include "ChunkRenderer.h"
#include "Renderer.h"
#include "World.h"
#include "Definitions.h"
#include "Blocks.h"
#include "Textures.h"

using namespace ChunkRenderer;
using namespace World;
using namespace Renderer;

namespace {
    VertexArray va(262144, VertexFormat(3, 3, 0, 3, 1));

    const int delta[6][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};

    //合并面的一整个面 | One face in merge face
    struct QuadPrimitive {
        int x, y, z, length, direction;
        /*
        * 如果顶点颜色不同（平滑光照启用时），这个方形就不能和别的方形拼合起来。
        * 这个变量同时意味着四个顶点颜色是否不同。
        * If the vertexes have different colors (smoothed lighting), the primitive cannot connect with others.
        * This variable also means whether the vertexes have different colors.
        */
        bool once;
        //顶点颜色 | Vertex colors
        int col0, col1, col2, col3;
        //纹理ID | Texture ID
        TextureID tex;

        QuadPrimitive() : x(0), y(0), z(0), length(0), direction(0), once(false),
                          col0(0), col1(0), col2(0), col3(0), tex(Textures::NULLBLOCK) { }
    };

    //深度模型的面 | Face in depth model
    struct QuadPrimitiveDepth {
        int x, y, z, length, direction;

        QuadPrimitiveDepth() : x(0), y(0), z(0), length(0), direction(0) { }
    };

	class ChunkRenderData {
	public:
		Block* mBlocks;
		Brightness* mBrightness;

		ChunkRenderData(int cx, int cy, int cz) {
			mBlocks = new Block[18 * 18 * 18];
			mBrightness = new Brightness[18 * 18 * 18];

			World::Chunk* chunkptr[3][3][3];

			for (int x = -1; x <= 1; x++)
				for (int y = -1; y <= 1; y++)
					for (int z = -1; z <= 1; z++)
						chunkptr[x + 1][y + 1][z + 1] = World::getChunkPtr(x + cx, y + cy, z + cz);

			for (int x = 0; x < 18; x++) {
				int rcx = 1, bx = x - 1;
				if (x == 0)
					rcx = 0, bx += 16;
				else if (x == 18 - 1)
					rcx = 2, bx -= 16;

				for (int y = 0; y < 18; y++) {
					int rcy = 1, by = y - 1;
					if (y == 0)
						rcy = 0, by += 16;
					else if (y == 18 - 1)
						rcy = 2, by -= 16;

					for (int z = 0; z < 18; z++) {
						int rcz = 1, bz = z - 1;
						if (z == 0)
							rcz = 0, bz += 16;
						else if (z == 18 - 1)
							rcz = 2, bz -= 16;

						if (chunkptr[rcx][rcy][rcz] == nullptr) {
							mBlocks[x * 18 * 18 + y * 18 + z] = Blocks::ROCK;
							mBrightness[x * 18 * 18 + y * 18 + z] = World::skylight;
						} else if (chunkptr[rcx][rcy][rcz] == World::emptyChunkPtr) {
							mBlocks[x * 18 * 18 + y * 18 + z] = Blocks::AIR;
							mBrightness[x * 18 * 18 + y * 18 + z] = (rcy - 1 + cy < 0) ? World::BrightnessMin : World::skylight;
						} else {
							mBlocks[x * 18 * 18 + y * 18 + z] = chunkptr[rcx][rcy][rcz]->getBlock(bx, by, bz);
							mBrightness[x * 18 * 18 + y * 18 + z] = chunkptr[rcx][rcy][rcz]->getBrightness(bx, by, bz);
						}
					}
				}
			}
		}

		~ChunkRenderData() {
			delete[] mBlocks;
			delete[] mBrightness;
		}

		Block getBlock(int x, int y, int z) {
			assert(x >= -1 && x <= 16 && y >= -1 && y <= 16 && z >= -1 && z <= 16);
			return mBlocks[(x + 1) * 18 * 18 + (y + 1) * 18 + (z + 1)];
		}

		Brightness getBrightness(int x, int y, int z) {
			assert(x >= -1 && x <= 16 && y >= -1 && y <= 16 && z >= -1 && z <= 16);
			return mBrightness[(x + 1) * 18 * 18 + (y + 1) * 18 + (z + 1)];
		}
	};
}

/*
合并面的顶点顺序（以0到3标出）：

The vertex order of merge face render
Numbered from 0 to 3:

(k++)
...
|    |
+----+--
|    |
|    |    |
3----2----+-
|curr|    |   ...
|face|    |   (j++)
0----1----+--

--qiaozhanrong
*/

void renderPrimitive(const QuadPrimitive& p) {
    auto col0 = static_cast<float>(p.col0) * 0.25f / BrightnessMax;
    auto col1 = static_cast<float>(p.col1) * 0.25f / BrightnessMax;
    auto col2 = static_cast<float>(p.col2) * 0.25f / BrightnessMax;
    auto col3 = static_cast<float>(p.col3) * 0.25f / BrightnessMax;
    const float x = p.x, y = p.y, z = p.z, length = p.length;
    const float texBase = (p.tex + 0.5) / 64.0;
    switch (p.direction) {
    case 0:
        col0 *= 0.7f, col1 *= 0.7f, col2 *= 0.7f, col3 *= 0.7f;
        va.addPrimitive(4, {
            0.0, 0.0, texBase, col0, col0, col0, x + 0.5f, y - 0.5f, z - 0.5f, 0.0f,
            0.0, 1.0, texBase, col1, col1, col1, x + 0.5f, y + 0.5f, z - 0.5f, 0.0f,
            length + 1.0f, 1.0, texBase, col2, col2, col2, x + 0.5f, y + 0.5f, z + length + 0.5f, 0.0f,
            length + 1.0f, 0.0, texBase, col3, col3, col3, x + 0.5f, y - 0.5f, z + length + 0.5f, 0.0f
        });
        break;
    case 1:
        col0 *= 0.7f, col1 *= 0.7f, col2 *= 0.7f, col3 *= 0.7f;
        va.addPrimitive(4, {
            0.0, 1.0, texBase, col0, col0, col0, x - 0.5f, y + 0.5f, z - 0.5f, 1.0f,
            0.0, 0.0, texBase, col1, col1, col1, x - 0.5f, y - 0.5f, z - 0.5f, 1.0f,
            length + 1.0f, 0.0, texBase, col2, col2, col2, x - 0.5f, y - 0.5f, z + length + 0.5f, 1.0f,
            length + 1.0f, 1.0, texBase, col3, col3, col3, x - 0.5f, y + 0.5f, z + length + 0.5f, 1.0f
        });
        break;
    case 2:
        va.addPrimitive(4, {
            0.0, 0.0, texBase, col0, col0, col0, x + 0.5f, y + 0.5f, z - 0.5f, 2.0f,
            0.0, 1.0, texBase, col1, col1, col1, x - 0.5f, y + 0.5f, z - 0.5f, 2.0f,
            length + 1.0f, 1.0, texBase, col2, col2, col2, x - 0.5f, y + 0.5f, z + length + 0.5f, 2.0f,
            length + 1.0f, 0.0, texBase, col3, col3, col3, x + 0.5f, y + 0.5f, z + length + 0.5f, 2.0f
        });
        break;
    case 3:
        va.addPrimitive(4, {
            0.0, 0.0, texBase, col0, col0, col0, x - 0.5f, y - 0.5f, z - 0.5f, 3.0f,
            0.0, 1.0, texBase, col1, col1, col1, x + 0.5f, y - 0.5f, z - 0.5f, 3.0f,
            length + 1.0f, 1.0, texBase, col2, col2, col2, x + 0.5f, y - 0.5f, z + length + 0.5f, 3.0f,
            length + 1.0f, 0.0, texBase, col3, col3, col3, x - 0.5f, y - 0.5f, z + length + 0.5f, 3.0f
        });
        break;
    case 4:
        col0 *= 0.5f, col1 *= 0.5f, col2 *= 0.5f, col3 *= 0.5f;
        va.addPrimitive(4, {
            0.0, 1.0, texBase, col0, col0, col0, x - 0.5f, y + 0.5f, z + 0.5f, 4.0f,
            0.0, 0.0, texBase, col1, col1, col1, x - 0.5f, y - 0.5f, z + 0.5f, 4.0f,
            length + 1.0f, 0.0, texBase, col2, col2, col2, x + length + 0.5f, y - 0.5f, z + 0.5f, 4.0f,
            length + 1.0f, 1.0, texBase, col3, col3, col3, x + length + 0.5f, y + 0.5f, z + 0.5f, 4.0f
        });
        break;
    case 5:
        col0 *= 0.5f, col1 *= 0.5f, col2 *= 0.5f, col3 *= 0.5f;
        va.addPrimitive(4, {
            0.0, 0.0, texBase, col0, col0, col0, x - 0.5f, y - 0.5f, z - 0.5f, 5.0f,
            0.0, 1.0, texBase, col1, col1, col1, x - 0.5f, y + 0.5f, z - 0.5f, 5.0f,
            length + 1.0f, 1.0, texBase, col2, col2, col2, x + length + 0.5f, y + 0.5f, z - 0.5f, 5.0f,
            length + 1.0f, 0.0, texBase, col3, col3, col3, x + length + 0.5f, y - 0.5f, z - 0.5f, 5.0f
        });
    default: ;
    }
}

void renderPrimitiveDepth(const QuadPrimitiveDepth& p) {
    const auto x = p.x, y = p.y, z = p.z, length = p.length;
    switch (p.direction) {
    case 0:
        va.addPrimitive(4, {
            x + 0.5f, y - 0.5f, z - 0.5f, x + 0.5f, y + 0.5f, z - 0.5f,
            x + 0.5f, y + 0.5f, z + length + 0.5f, x + 0.5f, y - 0.5f, z + length + 0.5f
        });
        break;
    case 1:
        va.addPrimitive(4, {
            x - 0.5f, y + 0.5f, z - 0.5f, x - 0.5f, y - 0.5f, z - 0.5f,
            x - 0.5f, y - 0.5f, z + length + 0.5f, x - 0.5f, y + 0.5f, z + length + 0.5f
        });
        break;
    case 2:
        va.addPrimitive(4, {
            x + 0.5f, y + 0.5f, z - 0.5f, x - 0.5f, y + 0.5f, z - 0.5f,
            x - 0.5f, y + 0.5f, z + length + 0.5f, x + 0.5f, y + 0.5f, z + length + 0.5f
        });
        break;
    case 3:
        va.addPrimitive(4, {
            x - 0.5f, y - 0.5f, z - 0.5f, x + 0.5f, y - 0.5f, z - 0.5f,
            x + 0.5f, y - 0.5f, z + length + 0.5f, x - 0.5f, y - 0.5f, z + length + 0.5f
        });
        break;
    case 4:
        va.addPrimitive(4, {
            x - 0.5f, y + 0.5f, z + 0.5f, x - 0.5f, y - 0.5f, z + 0.5f,
            x + length + 0.5f, y - 0.5f, z + 0.5f, x + length + 0.5f, y + 0.5f, z + 0.5f
        });
        break;
    case 5:
        va.addPrimitive(4, {
            x - 0.5f, y - 0.5f, z - 0.5f, x - 0.5f, y + 0.5f, z - 0.5f,
            x + length + 0.5f, y + 0.5f, z - 0.5f, x + length + 0.5f, y - 0.5f, z - 0.5f
        });
    default: ;
    }
}

//合并面大法好！！！
void ChunkRenderer::mergeFaceRender(World::Chunk* c) {
	//话说我注释一会中文一会英文是不是有点奇怪。。。
	// -- qiaozhanrong

	int cx = c->cx, cy = c->cy, cz = c->cz;
	ChunkRenderData rd(cx, cy, cz);
	int x = 0, y = 0, z = 0, cur_l_mx, br;
	int col0 = 0, col1 = 0, col2 = 0, col3 = 0;
	QuadPrimitive cur;
	Block bl, neighbour;
	int face = 0;
	TextureID tex;
	bool valid = false;

	for (int steps = 0; steps < 3; steps++) {
		cur = QuadPrimitive();
		cur_l_mx = bl = neighbour = 0;
		//Linear merge
		va.setFormat(VertexFormat(3, 3, 0, 3, 1));
		for (int d = 0; d < 6; d++) {
			cur.direction = d;
			if (d == 2) face = 1;
			else if (d == 3) face = 3;
			else face = 2;
			//Render current face
			for (int i = 0; i < 16; i++) for (int j = 0; j < 16; j++) {
				for (int k = 0; k < 16; k++) {
					//Get position & brightness
					if (d == 0) { //x+
						x = i, y = j, z = k;
						br = rd.getBrightness(x + 1, y, z);
						if (SmoothLighting) {
							col0 = br + rd.getBrightness(x + 1, y - 1, z) + rd.getBrightness(x + 1, y, z - 1) + rd.getBrightness(x + 1, y - 1, z - 1);
							col1 = br + rd.getBrightness(x + 1, y + 1, z) + rd.getBrightness(x + 1, y, z - 1) + rd.getBrightness(x + 1, y + 1, z - 1);
							col2 = br + rd.getBrightness(x + 1, y + 1, z) + rd.getBrightness(x + 1, y, z + 1) + rd.getBrightness(x + 1, y + 1, z + 1);
							col3 = br + rd.getBrightness(x + 1, y - 1, z) + rd.getBrightness(x + 1, y, z + 1) + rd.getBrightness(x + 1, y - 1, z + 1);
						}
						else col0 = col1 = col2 = col3 = br * 4;
					}
					else if (d == 1) { //x-
						x = i, y = j, z = k;
						br = rd.getBrightness(x - 1, y, z);
						if (SmoothLighting) {
							col0 = br + rd.getBrightness(x - 1, y + 1, z) + rd.getBrightness(x - 1, y, z - 1) + rd.getBrightness(x - 1, y + 1, z - 1);
							col1 = br + rd.getBrightness(x - 1, y - 1, z) + rd.getBrightness(x - 1, y, z - 1) + rd.getBrightness(x - 1, y - 1, z - 1);
							col2 = br + rd.getBrightness(x - 1, y - 1, z) + rd.getBrightness(x - 1, y, z + 1) + rd.getBrightness(x - 1, y - 1, z + 1);
							col3 = br + rd.getBrightness(x - 1, y + 1, z) + rd.getBrightness(x - 1, y, z + 1) + rd.getBrightness(x - 1, y + 1, z + 1);
						}
						else col0 = col1 = col2 = col3 = br * 4;
					}
					else if (d == 2) { //y+
						x = j, y = i, z = k;
						br = rd.getBrightness(x, y + 1, z);
						if (SmoothLighting) {
							col0 = br + rd.getBrightness(x + 1, y + 1, z) + rd.getBrightness(x, y + 1, z - 1) + rd.getBrightness(x + 1, y + 1, z - 1);
							col1 = br + rd.getBrightness(x - 1, y + 1, z) + rd.getBrightness(x, y + 1, z - 1) + rd.getBrightness(x - 1, y + 1, z - 1);
							col2 = br + rd.getBrightness(x - 1, y + 1, z) + rd.getBrightness(x, y + 1, z + 1) + rd.getBrightness(x - 1, y + 1, z + 1);
							col3 = br + rd.getBrightness(x + 1, y + 1, z) + rd.getBrightness(x, y + 1, z + 1) + rd.getBrightness(x + 1, y + 1, z + 1);
						}
						else col0 = col1 = col2 = col3 = br * 4;
					}
					else if (d == 3) { //y-
						x = j, y = i, z = k;
						br = rd.getBrightness(x, y - 1, z);
						if (SmoothLighting) {
							col0 = br + rd.getBrightness(x - 1, y - 1, z) + rd.getBrightness(x, y - 1, z - 1) + rd.getBrightness(x - 1, y - 1, z - 1);
							col1 = br + rd.getBrightness(x + 1, y - 1, z) + rd.getBrightness(x, y - 1, z - 1) + rd.getBrightness(x + 1, y - 1, z - 1);
							col2 = br + rd.getBrightness(x + 1, y - 1, z) + rd.getBrightness(x, y - 1, z + 1) + rd.getBrightness(x + 1, y - 1, z + 1);
							col3 = br + rd.getBrightness(x - 1, y - 1, z) + rd.getBrightness(x, y - 1, z + 1) + rd.getBrightness(x - 1, y - 1, z + 1);
						}
						else col0 = col1 = col2 = col3 = br * 4;
					}
					else if (d == 4) { //z+
						x = k, y = j, z = i;
						br = rd.getBrightness(x, y, z + 1);
						if (SmoothLighting) {
							col0 = br + rd.getBrightness(x - 1, y, z + 1) + rd.getBrightness(x, y + 1, z + 1) + rd.getBrightness(x - 1, y + 1, z + 1);
							col1 = br + rd.getBrightness(x - 1, y, z + 1) + rd.getBrightness(x, y - 1, z + 1) + rd.getBrightness(x - 1, y - 1, z + 1);
							col2 = br + rd.getBrightness(x + 1, y, z + 1) + rd.getBrightness(x, y - 1, z + 1) + rd.getBrightness(x + 1, y - 1, z + 1);
							col3 = br + rd.getBrightness(x + 1, y, z + 1) + rd.getBrightness(x, y + 1, z + 1) + rd.getBrightness(x + 1, y + 1, z + 1);
						}
						else col0 = col1 = col2 = col3 = br * 4;
					}
					else if (d == 5) { //z-
						x = k, y = j, z = i;
						br = rd.getBrightness(x, y, z - 1);
						if (SmoothLighting) {
							col0 = br + rd.getBrightness(x - 1, y, z - 1) + rd.getBrightness(x, y - 1, z - 1) + rd.getBrightness(x - 1, y - 1, z - 1);
							col1 = br + rd.getBrightness(x - 1, y, z - 1) + rd.getBrightness(x, y + 1, z - 1) + rd.getBrightness(x - 1, y + 1, z - 1);
							col2 = br + rd.getBrightness(x + 1, y, z - 1) + rd.getBrightness(x, y + 1, z - 1) + rd.getBrightness(x + 1, y + 1, z - 1);
							col3 = br + rd.getBrightness(x + 1, y, z - 1) + rd.getBrightness(x, y - 1, z - 1) + rd.getBrightness(x + 1, y - 1, z - 1);
						}
						else col0 = col1 = col2 = col3 = br * 4;
					}
					//Get block ID
					bl = rd.getBlock(x, y, z);
					tex = Textures::getTextureIndex(bl, face);
					neighbour = rd.getBlock(x + delta[d][0], y + delta[d][1], z + delta[d][2]);
					if (NiceGrass && bl == Blocks::GRASS) {
						if (d == 0 && rd.getBlock(x + 1, y - 1, z) == Blocks::GRASS) tex = Textures::getTextureIndex(bl, 1);
						else if (d == 1 && rd.getBlock(x - 1, y - 1, z) == Blocks::GRASS) tex = Textures::getTextureIndex(bl, 1);
						else if (d == 4 && rd.getBlock(x, y - 1, z + 1) == Blocks::GRASS) tex = Textures::getTextureIndex(bl, 1);
						else if (d == 5 && rd.getBlock(x, y - 1, z - 1) == Blocks::GRASS) tex = Textures::getTextureIndex(bl, 1);
					}
					//Render
					const Blocks::SingleBlock& info = getBlockInfo(bl);
					if (bl == Blocks::AIR || bl == neighbour && bl != Blocks::LEAF || getBlockInfo(neighbour).isOpaque() ||
						steps == 0 && info.isTranslucent() ||
						steps == 1 && (!info.isTranslucent() || !info.isSolid()) ||
						steps == 2 && (!info.isTranslucent() || info.isSolid())) {
						//Not valid block
						if (valid) {
							if (getBlockInfo(neighbour).isOpaque() && !cur.once) {
								if (cur_l_mx < cur.length) cur_l_mx = cur.length;
								cur_l_mx++;
							}
							else {
								renderPrimitive(cur);
								valid = false;
							}
						}
						continue;
					}
					if (valid) {
						if (col0 != col1 || col1 != col2 || col2 != col3 || cur.once || tex != cur.tex || col0 != cur.col0) {
							renderPrimitive(cur);
							cur.x = x; cur.y = y; cur.z = z; cur.length = cur_l_mx = 0;
							cur.tex = tex; cur.col0 = col0; cur.col1 = col1; cur.col2 = col2; cur.col3 = col3;
							if (col0 != col1 || col1 != col2 || col2 != col3) cur.once = true; else cur.once = false;
						}
						else {
							if (cur_l_mx > cur.length) cur.length = cur_l_mx;
							cur.length++;
						}
					}
					else {
						valid = true;
						cur.x = x; cur.y = y; cur.z = z; cur.length = cur_l_mx = 0;
						cur.tex = tex; cur.col0 = col0; cur.col1 = col1; cur.col2 = col2; cur.col3 = col3;
						if (col0 != col1 || col1 != col2 || col2 != col3) cur.once = true; else cur.once = false;
					}
				}
				if (valid) {
					renderPrimitive(cur);
					valid = false;
				}
			}
		}
		c->vbo[steps].update(va);
	}
}

void ChunkRenderer::renderDepthModel(Chunk* c) {
    constexpr VertexFormat fmt(0, 0, 0, 3);
    int cx = c->cx, cy = c->cy, cz = c->cz;
    int x = 0, y = 0, z = 0;
    QuadPrimitiveDepth cur;
    Block bl, neighbour;
    bool valid = false;
    int cur_l_mx = bl = neighbour = 0;
    //Linear merge for depth model
    va.setFormat(fmt);
    for (int d = 0; d < 6; d++) {
        cur.direction = d;
        for (int i = 0; i < 16; i++)
            for (int j = 0; j < 16; j++) {
                for (int k = 0; k < 16; k++) {
                    //Get position
                    if (d < 2) x = i, y = j, z = k;
                    else if (d < 4) x = i, y = j, z = k;
                    else x = k, y = i, z = j;
                    //Get block ID
                    bl = c->getBlock(x, y, z);
                    //Get neighbour ID
                    int xx = x + delta[d][0], yy = y + delta[d][1], zz = z + delta[d][2];
                    int gx = cx * 16 + xx, gy = cy * 16 + yy, gz = cz * 16 + zz;
                    if (xx < 0 || xx >= 16 || yy < 0 || yy >= 16 || zz < 0 || zz >= 16)
                        neighbour = getBlock(gx, gy, gz);
                    else neighbour = c->getBlock(xx, yy, zz);
                    //Render
                    if (bl == Blocks::AIR || bl == Blocks::GLASS || (bl == neighbour && bl != Blocks::LEAF) ||
                        getBlockInfo(neighbour).isOpaque() || getBlockInfo(bl).isTranslucent()) {
                        //Not valid block
                        if (valid) {
                            if (getBlockInfo(neighbour).isOpaque()) {
                                if (cur_l_mx < cur.length) cur_l_mx = cur.length;
                                cur_l_mx++;
                            }
                            else {
                                renderPrimitiveDepth(cur);
                                valid = false;
                            }
                        }
                        continue;
                    }
                    if (valid) {
                        if (cur_l_mx > cur.length) cur.length = cur_l_mx;
                        cur.length++;
                    }
                    else {
                        valid = true;
                        cur.x = x;
                        cur.y = y;
                        cur.z = z;
                        cur.length = cur_l_mx = 0;
                    }
                }
                if (valid) {
                    renderPrimitiveDepth(cur);
                    valid = false;
                }
            }
    }
    c->vbo[3].update(va);
}

constexpr auto off=0.5f,right = off, left = -off, top = off, bottom = -off, front = off, back = -off;

constexpr float cubeVert[8][3] = {
    {right, top, front}, //0
    {right, top, back}, //1
    {right, bottom, front}, //2
    {right, bottom, back}, //3
    {left, top, front}, //4
    {left, top, back}, //5
    {left, bottom, front}, //6
    {left, bottom, back} //7
};

//lt rt rb lb
constexpr int cubeFace[6][4] = {
    {4, 0, 2, 6}, //front
    {5, 1, 3, 7}, //back
    {0, 1, 3, 2}, //right
    {4, 5, 7, 6}, //left
    {0, 1, 5, 4}, //top
    {2, 3, 7, 6} //bottom
};

float sampleBrt(const Vec3f center) {
    auto brt = 0.0f;
    for (auto i : cubeVert) {
        const Vec3f offset{i[0], i[1], i[2]};
        const auto pos = center + offset;
        brt += getBrightness(pos.x, pos.y, pos.z);
    }
    return brt / 8.0f;
}

constexpr float faceBrightness[6] = {
    1.0f, 0.5f, 0.7f, 0.7f, 1.0f, 0.2f
};

Vec4f calcVertex(const Block blk, const float nbrt, const int face, const Vec3f pos, const Vec3f gPos,
                 const int id) {
    const auto vert = cubeFace[face][id];
    const Vec3f offset{cubeVert[vert][0], cubeVert[vert][1], cubeVert[vert][2]};
    const auto vpos = pos + offset;
    auto brt = SmoothLighting ? sampleBrt(gPos + offset) : nbrt;
    brt /= BrightnessMax;
    return {vpos, brt * faceBrightness[face]};
}

constexpr int faceOffset[6][3] = {
    {0, 0, 1}, //front
    {0, 0, -1}, //back
    {1, 0, 0}, //right
    {-1, 0, 0}, //left
    {0, 1, 0}, //top
    {0, -1, 0} //bottom
};

bool testFace(const Block blk, const Block nearBlk) {
    const auto b = getBlockInfo(blk), nb = getBlockInfo(nearBlk);
    if(b.isSolid() && nb.isSolid()) return !(b.isOpaque() && nb.isOpaque());
    if (!b.isSolid() && !nb.isSolid()) return blk != nearBlk;
    return b.isSolid();
}

void renderFace(const Block blk, const Block nearBlk, const int texType, const int face,
                const Vec3f pos, const Vec3f gPos) {
    constexpr auto size = 1.0f / 8.0f;
    if (testFace(blk,nearBlk)) {
        const auto tcx = Textures::getTexcoordX(blk, texType),
                   tcy = Textures::getTexcoordY(blk, texType);
        const auto brt = getBrightness(gPos.x + faceOffset[face][0], gPos.y + faceOffset[face][1],
                                       gPos.z + faceOffset[face][2]);
        Vec4f vert[4]{
            calcVertex(blk, brt, face, pos, gPos, 0),
            calcVertex(blk, brt, face, pos, gPos, 1),
            calcVertex(blk, brt, face, pos, gPos, 2),
            calcVertex(blk, brt, face, pos, gPos, 3)
        };

        const float faceFlag = face;

        va.addPrimitive(4, {
            tcx + size, tcy + size, vert[0].t, vert[0].t, vert[0].t, vert[0].x, vert[0].y, vert[0].z, faceFlag,
            tcx, tcy + size, vert[1].t, vert[1].t, vert[1].t, vert[1].x, vert[1].y, vert[1].z, faceFlag,
            tcx, tcy, vert[2].t, vert[2].t, vert[2].t, vert[2].x, vert[2].y, vert[2].z, faceFlag,
            tcx + size, tcy, vert[3].t, vert[3].t, vert[3].t, vert[3].x, vert[3].y, vert[3].z, faceFlag
        });
    }
}

int getTexType(const Block blk, const int nx, const int ny, const int nz) {
    return (NiceGrass && blk == Blocks::GRASS && getBlock(nx, ny, nz) == Blocks::GRASS) ? 1 : 2;
}

void renderBlock(const int x, const int y, const int z, const Chunk* const chunkptr) {
    const auto cx = chunkptr->cx, cy = chunkptr->cy, cz = chunkptr->cz;
    const auto gx = cx * 16 + x, gy = cy * 16 + y, gz = cz * 16 + z;

    //front back right left top bottom
    Block blk[7] = {
        chunkptr->getBlock(x, y, z),
        z < 15 ? chunkptr->getBlock(x, y, z + 1) : getBlock(gx, gy, gz + 1, Blocks::ROCK),
        z > 0 ? chunkptr->getBlock(x, y, z - 1) : getBlock(gx, gy, gz - 1, Blocks::ROCK),
        x < 15 ? chunkptr->getBlock(x + 1, y, z) : getBlock(gx + 1, gy, gz, Blocks::ROCK),
        x > 0 ? chunkptr->getBlock(x - 1, y, z) : getBlock(gx - 1, gy, gz, Blocks::ROCK),
        y < 15 ? chunkptr->getBlock(x, y + 1, z) : getBlock(gx, gy + 1, gz, Blocks::ROCK),
        y > 0 ? chunkptr->getBlock(x, y - 1, z) : getBlock(gx, gy - 1, gz, Blocks::ROCK)
    };

    const Vec3f pos(x, y, z), gPos(gx, gy, gz);

    renderFace(blk[0], blk[1], getTexType(blk[0], gx, gy - 1, gz + 1), 0, pos, gPos);
    renderFace(blk[0], blk[2], getTexType(blk[0], gx, gy - 1, gz - 1), 1, pos, gPos);
    renderFace(blk[0], blk[3], getTexType(blk[0], gx + 1, gy - 1, gz), 2, pos, gPos);
    renderFace(blk[0], blk[4], getTexType(blk[0], gx - 1, gy - 1, gz), 3, pos, gPos);
    renderFace(blk[0], blk[5], 1, 4, pos, gPos);
    renderFace(blk[0], blk[6], 3, 5, pos, gPos);
}

void ChunkRenderer::renderChunk(Chunk* c) {
    int x, y, z;
    constexpr auto fmt = VertexFormat(2, 3, 0, 3, 1);
    va.setFormat(fmt);
    for (x = 0; x < 16; x++) {
        for (y = 0; y < 16; y++) {
            for (z = 0; z < 16; z++) {
                const Block curr = c->getBlock(x, y, z);
                if (curr == Blocks::AIR) continue;
                if (!getBlockInfo(curr).isTranslucent()) renderBlock(x, y, z, c);
            }
        }
    }
    c->vbo[0].update(va);
    for (x = 0; x < 16; x++) {
        for (y = 0; y < 16; y++) {
            for (z = 0; z < 16; z++) {
                Block curr = c->getBlock(x, y, z);
                if (curr == Blocks::AIR) continue;
                if (getBlockInfo(curr).isTranslucent() && getBlockInfo(curr).isSolid()) renderBlock(x, y, z, c);
            }
        }
    }
    c->vbo[1].update(va);
    for (x = 0; x < 16; x++) {
        for (y = 0; y < 16; y++) {
            for (z = 0; z < 16; z++) {
                Block curr = c->getBlock(x, y, z);
                if (curr == Blocks::AIR) continue;
                if (getBlockInfo(curr).isTranslucent() && !getBlockInfo(curr).isSolid()) renderBlock(x, y, z, c);
            }
        }
    }
    c->vbo[2].update(va);
}
