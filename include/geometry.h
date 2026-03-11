/*
sdl3_cmake_template
Copyright (C) 2026 Matej Smetana

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <stdlib.h>

typedef struct Vertex {
    float x, y;
    float r, g, b, a;
} Vertex;

static const size_t num_vertices = 3;

const Vertex vertices[] = {
    {0.0, 0.5, 1.0, 0.0, 0.0, 1.0},
    {-0.5, -0.5, 0.0, 1.0, 0.0, 1.0},
    {0.5, -0.5, 0.0, 0.0, 1.0, 1.0},
};
