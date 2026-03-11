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

#define SDL_MAIN_USE_CALLBACKS

#include "context.h"
#include "geometry.h"
#include "shader_config.h"
#include "shaders.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdbool.h>

static SDL_AppResult build_pipeline(AppContext *context) {
    // Create Shaders
    SDL_GPUShaderFormat format = pick_best_shader_format(context->device);

    LoadShaderInfo vertex_shader_info = {.stage = SDL_GPU_SHADERSTAGE_VERTEX,
                                         .format = format,
                                         .base_path = "position_color.vert",
                                         .sampler_count = 0,
                                         .uniform_buffer_count = 0,
                                         .storage_buffer_count = 0,
                                         .storage_texture_count = 0};

    SDL_GPUShader *vertex_shader = load_shader(context, &vertex_shader_info);
    if (vertex_shader == NULL) {
        return SDL_APP_FAILURE;
    }

    LoadShaderInfo fragment_shader_info = {.stage =
                                               SDL_GPU_SHADERSTAGE_FRAGMENT,
                                           .format = format,
                                           .base_path = "color.frag",
                                           .sampler_count = 0,
                                           .uniform_buffer_count = 0,
                                           .storage_buffer_count = 0,
                                           .storage_texture_count = 0};

    SDL_GPUShader *fragment_shader =
        load_shader(context, &fragment_shader_info);
    if (fragment_shader == NULL) {
        SDL_ReleaseGPUShader(context->device, vertex_shader);
        return SDL_APP_FAILURE;
    }

    // Describe Render Target
    SDL_GPUTextureFormat target_format =
        SDL_GetGPUSwapchainTextureFormat(context->device, context->window);

    SDL_GPUGraphicsPipelineTargetInfo target_info = {
        .num_color_targets = 1,
        .color_target_descriptions = (SDL_GPUColorTargetDescription[]){
            {.format = target_format,
             .blend_state = {
                 .enable_blend = true,
                 .color_blend_op = SDL_GPU_BLENDOP_ADD,
                 .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
                 .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
                 .dst_color_blendfactor =
                     SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                 .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
                 .dst_alpha_blendfactor =
                     SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA}}}};

    // Primitives Rasterization
    SDL_GPURasterizerState rasterizer_state = {
        .fill_mode = SDL_GPU_FILLMODE_FILL,
        .cull_mode = SDL_GPU_CULLMODE_BACK,
        .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE};

    // Set The Vertex Input Format
    SDL_GPUVertexInputState vertex_input_state = {
        .num_vertex_buffers = 1,
        .vertex_buffer_descriptions =
            (SDL_GPUVertexBufferDescription[]){
                {.slot = 0,
                 .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
                 .instance_step_rate = 0,
                 .pitch = sizeof(Vertex)}},
        .num_vertex_attributes = 2,
        .vertex_attributes = (SDL_GPUVertexAttribute[]){
            {.buffer_slot = 0,
             .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
             .location = 0,
             .offset = 0},
            {.buffer_slot = 0,
             .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
             .location = 1,
             .offset = sizeof(float) * 2}}};

    SDL_GPUGraphicsPipelineCreateInfo pipeline_create_info = {
        .target_info = target_info,
        .rasterizer_state = rasterizer_state,
        .vertex_input_state = vertex_input_state,
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader};

    SDL_GPUGraphicsPipeline *pipeline =
        SDL_CreateGPUGraphicsPipeline(context->device, &pipeline_create_info);
    if (pipeline == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "Unable to create graphics pipeline: %s", SDL_GetError());
    }

    SDL_ReleaseGPUShader(context->device, vertex_shader);
    SDL_ReleaseGPUShader(context->device, fragment_shader);

    context->pipeline = pipeline;
    return pipeline != NULL ? SDL_APP_CONTINUE : SDL_APP_FAILURE;
}

static SDL_AppResult build_vertex_buffer(AppContext *context) {
    const Uint32 buffer_size = (Uint32)(sizeof(Vertex) * num_vertices);

    SDL_GPUBufferCreateInfo buffer_create_info = {
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX, .size = buffer_size};
    SDL_GPUBuffer *vertex_buffer =
        SDL_CreateGPUBuffer(context->device, &buffer_create_info);
    if (!vertex_buffer) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "Create vertex buffer failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, .size = buffer_size};
    SDL_GPUTransferBuffer *transfer_buffer = SDL_CreateGPUTransferBuffer(
        context->device, &transfer_buffer_create_info);
    if (!transfer_buffer) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "Create transfer buffer failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    Vertex *vertex_ptr =
        SDL_MapGPUTransferBuffer(context->device, transfer_buffer, false);
    SDL_memcpy(vertex_ptr, vertices, sizeof(Vertex) * num_vertices);
    SDL_UnmapGPUTransferBuffer(context->device, transfer_buffer);
    vertex_ptr = NULL;

    SDL_GPUCommandBuffer *cmd_buf =
        SDL_AcquireGPUCommandBuffer(context->device);
    SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(cmd_buf);
    SDL_GPUTransferBufferLocation source_buffer = {
        .transfer_buffer = transfer_buffer, .offset = 0};
    SDL_GPUBufferRegion target_buffer = {
        .buffer = vertex_buffer, .offset = 0, .size = buffer_size};

    SDL_UploadToGPUBuffer(copy_pass, &source_buffer, &target_buffer, false);
    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(cmd_buf);

    SDL_ReleaseGPUTransferBuffer(context->device, transfer_buffer);

    context->vertex_buffer = vertex_buffer;
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppInit(void **app_state, int argc, char *argv[]) {
    SDL_SetAppMetadata("GPU by Example - Getting Started", "0.1.0",
                       "net.jonathanfischer.GpuByExample1");

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // ==== GPU Device ====

    SDL_GPUShaderFormat shader_formats = 0;

#ifdef SHADERS_BUILD_SPIRV
    shader_formats |= SDL_GPU_SHADERFORMAT_SPIRV;
#endif
#ifdef SHADERS_BUILD_DXIL
    shader_formats |= SDL_GPU_SHADERFORMAT_DXIL;
#endif
#ifdef SHADERS_BUILD_MSL
    shader_formats |= SDL_GPU_SHADERFORMAT_MSL;
#endif

    SDL_GPUDevice *device = SDL_CreateGPUDevice(shader_formats, false, NULL);
    if (device == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "Couldn't not create GPU device: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_Log("Using %s GPU implementation.", SDL_GetGPUDeviceDriver(device));

    // ==== Window ====

    SDL_WindowFlags window_flags =
        SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE;

    SDL_Window *window = SDL_CreateWindow("GPU by Example - Getting Started",
                                          800, 600, window_flags);

    if (window == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window: %s",
                     SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_ClaimWindowForGPUDevice(device, window)) {
        SDL_Log("SDL_ClaimWindowForGPUDevice failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // ==== Storage ====

    SDL_Storage *storage = SDL_OpenTitleStorage(NULL, 0);
    if (storage == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "Unable to open title storage: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    while (!SDL_StorageReady(storage)) {
        SDL_Delay(1);
    }

    // ==== Context ====

    AppContext *context = SDL_calloc(1, sizeof(AppContext));
    *app_state = context;

    context->window = window;
    context->device = device;
    context->title_storage = storage;

    // ==== Create GPU Resources ====

    if (build_pipeline(context) != SDL_APP_CONTINUE) {
        return SDL_APP_FAILURE;
    }

    if (build_vertex_buffer(context) != SDL_APP_CONTINUE) {
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *app_state) {
    AppContext *context = (AppContext *)app_state;

    SDL_GPUCommandBuffer *cmd_buf =
        SDL_AcquireGPUCommandBuffer(context->device);
    if (cmd_buf == NULL) {
        SDL_Log("SDL_AcquireGPUCommandBuffer failed %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_GPUTexture *swapchain_texture;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(
            cmd_buf, context->window, &swapchain_texture, NULL, NULL)) {
        SDL_Log("SDL_WaitAndAcquireGPUSwapchainTexture failed: %s",
                SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (swapchain_texture != NULL) {
        SDL_GPUColorTargetInfo target_info = {
            .texture = swapchain_texture,
            .cycle = true,
            .load_op = SDL_GPU_LOADOP_CLEAR,
            .store_op = SDL_GPU_STOREOP_STORE,
            .clear_color = {0.16f, 0.47f, 0.34f, 1.0f}};

        SDL_GPURenderPass *render_pass;
        render_pass = SDL_BeginGPURenderPass(cmd_buf, &target_info, 1, NULL);

        // Render Pipeline
        SDL_BindGPUGraphicsPipeline(render_pass, context->pipeline);

        SDL_GPUBufferBinding vertex_buffer_binding = {
            .buffer = context->vertex_buffer, .offset = 0};
        SDL_BindGPUVertexBuffers(render_pass, 0, &vertex_buffer_binding, 1);

        SDL_DrawGPUPrimitives(render_pass, 3, 1, 0, 0);

        SDL_EndGPURenderPass(render_pass);
    }

    SDL_SubmitGPUCommandBuffer(cmd_buf);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *app_state, SDL_Event *event) {
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }

    if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_ESCAPE) {
        return SDL_APP_SUCCESS;
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *app_state, SDL_AppResult result) {
    AppContext *context = (AppContext *)app_state;

    if (context != NULL) {
        if (context->title_storage != NULL) {
            SDL_CloseStorage(context->title_storage);
        }

        if (context->device != NULL) {
            if (context->vertex_buffer != NULL) {
                SDL_ReleaseGPUBuffer(context->device, context->vertex_buffer);
            }

            if (context->pipeline != NULL) {
                SDL_ReleaseGPUGraphicsPipeline(context->device,
                                               context->pipeline);
            }

            if (context->window) {
                SDL_ReleaseWindowFromGPUDevice(context->device,
                                               context->window);
                SDL_DestroyWindow(context->window);
            }

            SDL_DestroyGPUDevice(context->device);
        }

        SDL_free(context);
    }

    SDL_Quit();
}
