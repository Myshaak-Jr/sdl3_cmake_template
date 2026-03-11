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

#include "shaders.h"

#include "shader_config.h"

static const char *shader_ext_for_format(SDL_GPUShaderFormat format) {
    switch (format) {
        case SDL_GPU_SHADERFORMAT_SPIRV:
            return "spv";
        case SDL_GPU_SHADERFORMAT_DXIL:
            return "dxil";
        case SDL_GPU_SHADERFORMAT_MSL:
            return "msl";
        default:
            return NULL;
    }
}

static const char *shader_directory_for_format(SDL_GPUShaderFormat format) {
    switch (format) {
        case SDL_GPU_SHADERFORMAT_SPIRV:
            return SHADERS_DIR_SPIRV;
        case SDL_GPU_SHADERFORMAT_DXIL:
            return SHADERS_DIR_DXIL;
        case SDL_GPU_SHADERFORMAT_MSL:
            return SHADERS_DIR_MSL;
        default:
            return NULL;
    }
}

SDL_GPUShader *load_shader(AppContext *context,
                           const LoadShaderInfo *load_shader_info) {
    const char *format_ext = shader_ext_for_format(load_shader_info->format);
    const char *format_dir =
        shader_directory_for_format(load_shader_info->format);

    if (format_ext == NULL || format_dir == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Invalid shader format");
        return NULL;
    }

    char path[512];
    SDL_snprintf(path, sizeof(path), "shaders/%s/%s.%s", format_dir,
                 load_shader_info->base_path, format_ext);

    SDL_Log("Loading shader file %s...", path);

    Uint64 code_size;
    void *code;
    if (!SDL_GetStorageFileSize(context->title_storage, path, &code_size)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "Unable to determine size of file '%s': %s", path,
                     SDL_GetError());
        return NULL;
    }

    code = SDL_malloc(code_size);
    if (!SDL_ReadStorageFile(context->title_storage, path, code, code_size)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "Unable to read file '%s': %s", path, SDL_GetError());
        SDL_free(code);
        return NULL;
    }

    const char *entry_point = "main";

    SDL_GPUShaderCreateInfo shader_info = {
        .code = code,
        .code_size = code_size,
        .entrypoint = entry_point,
        .format = load_shader_info->format,
        .stage = load_shader_info->stage,
        .num_samplers = load_shader_info->sampler_count,
        .num_uniform_buffers = load_shader_info->uniform_buffer_count,
        .num_storage_buffers = load_shader_info->storage_buffer_count,
        .num_storage_textures = load_shader_info->storage_texture_count};

    SDL_GPUShader *shader = SDL_CreateGPUShader(context->device, &shader_info);
    if (shader == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "Unable to create shader: %s", SDL_GetError());
        SDL_free(code);
        return shader;
    }

    SDL_Log("Loaded.");
    SDL_free(code);
    return shader;
}

SDL_GPUShaderFormat pick_best_shader_format(SDL_GPUDevice *device) {
    // SDL_GetGPUShaderFormats returns a bitmask of SDL_GPUShaderFormat values
    // supported by the device.
    const Uint32 supported = SDL_GetGPUShaderFormats(device);

    // Prefer SPIR-V (common on Vulkan), then DXIL/DXBC, then Metal formats.
    if (supported & SDL_GPU_SHADERFORMAT_SPIRV)
        return SDL_GPU_SHADERFORMAT_SPIRV;
    if (supported & SDL_GPU_SHADERFORMAT_DXIL)
        return SDL_GPU_SHADERFORMAT_DXIL;
    if (supported & SDL_GPU_SHADERFORMAT_DXBC)
        return SDL_GPU_SHADERFORMAT_DXBC;
    if (supported & SDL_GPU_SHADERFORMAT_METALLIB)
        return SDL_GPU_SHADERFORMAT_METALLIB;
    if (supported & SDL_GPU_SHADERFORMAT_MSL)
        return SDL_GPU_SHADERFORMAT_MSL;

    return (SDL_GPUShaderFormat)0;
}
