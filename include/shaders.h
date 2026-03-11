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

#include "context.h"

#include <SDL3/SDL.h>

typedef struct LoadShaderInfo {
    SDL_GPUShaderStage stage;
    SDL_GPUShaderFormat format;
    const char *base_path;
    Uint32 sampler_count;
    Uint32 uniform_buffer_count;
    Uint32 storage_buffer_count;
    Uint32 storage_texture_count;
} LoadShaderInfo;

SDL_GPUShader *load_shader(AppContext *context,
                           const LoadShaderInfo *load_shader_info);
SDL_GPUShaderFormat pick_best_shader_format(SDL_GPUDevice *device);
