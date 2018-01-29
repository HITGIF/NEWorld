// 
// NWShared: VectorBase.cpp
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

#include "VectorBase.h"
#include <cstdlib>
#include <new>

void* vectorAlloc(size_t size, size_t item) {
    const auto ret = std::malloc(size * item);
    if (!ret)
        throw std::bad_alloc();
    return ret;
}

void* vectorRealloc(void* old, size_t size, size_t item) {
    const auto ret = std::realloc(old, size * item);
    if (!ret)
        throw std::bad_alloc();
    return ret;
}

void vectorFree(void* mem) {
    std::free(mem);
}
