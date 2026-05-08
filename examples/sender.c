// nozzle-sokol sender example: renders a color-gradient texture and shares it via nozzle
#define SOKOL_IMPL
#define SOKOL_NO_ENTRY
#include <sokol/sokol_app.h>
#include <sokol/sokol_gfx.h>
#include <sokol/sokol_log.h>
#include <sokol/sokol_glue.h>

#define NOZZLE_SOKOL_IMPL
#include <nozzle-sokol.h>
#include <nozzle/nozzle_c.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct {
    sg_pass_action pass_action;
    NozzleSender *sender;
    sg_image img;
    int frame_count;
    uint32_t width;
    uint32_t height;
} state;

static void init(void) {
    sg_desc sg_desc{};
    sg_desc.environment = sglue_environment();
    sg_desc.logger.func = slog_func;
    sg_setup(&sg_desc);

    NozzleSenderDesc desc{};
    desc.name = "sokol_sender";
    desc.application_name = "nozzle-sokol-sender";
    desc.ring_buffer_size = 3;
    desc.fallback_flags_valid = 1;
    desc.fallback_flags = NOZZLE_FALLBACK_SAFE_DEFAULTS;
    if (nozzle_sender_create(&desc, &state.sender) != NOZZLE_OK) {
        fprintf(stderr, "failed to create nozzle sender\n");
    }

    state.width = 512;
    state.height = 512;

    sg_image_desc img_desc{};
    img_desc.width = state.width;
    img_desc.height = state.height;
    img_desc.pixel_format = SG_PIXELFORMAT_RGBA8;
    img_desc.usage = SG_USAGE_STREAM;
    state.img = sg_make_image(&img_desc);

    state.pass_action.colors[0].load_action = SG_LOADACTION_CLEAR;
    state.pass_action.colors[0].clear_value = (sg_color){0.1f, 0.1f, 0.1f, 1.0f};
}

static void frame(void) {
    uint32_t w = state.width;
    uint32_t h = state.height;
    size_t data_size = w * h * 4;
    uint8_t *pixels = (uint8_t *)malloc(data_size);

    float t = (float)state.frame_count * 0.02f;
    for (uint32_t y = 0; y < h; y++) {
        for (uint32_t x = 0; x < w; x++) {
            uint32_t idx = (y * w + x) * 4;
            pixels[idx + 0] = (uint8_t)((x * 255) / w);
            pixels[idx + 1] = (uint8_t)((y * 255) / h);
            pixels[idx + 2] = (uint8_t)(128 + 127 * sinf(t + (float)x / w * 6.28f));
            pixels[idx + 3] = 255;
        }
    }

    sg_image_data img_data{};
    img_data.subimage[0][0].ptr = pixels;
    img_data.subimage[0][0].size = data_size;
    sg_update_image(state.img, &img_data);
    free(pixels);

    if (state.sender) {
        nozzle_sokol_image_publish(state.sender, state.img);
    }

    sg_begin_pass(&(sg_pass){.action = state.pass_action, .swapchain = sglue_swapchain()});
    sg_end_pass();
    sg_commit();

    state.frame_count++;
}

static void cleanup(void) {
    if (state.sender) {
        nozzle_sender_destroy(state.sender);
    }
    sg_shutdown();
}

sapp_desc sokol_main(int argc, char *argv[]) {
    (void)argc; (void)argv;
    sapp_desc desc{};
    desc.init_cb = init;
    desc.frame_cb = frame;
    desc.cleanup_cb = cleanup;
    desc.width = 512;
    desc.height = 512;
    desc.window_title = "nozzle-sokol sender";
    desc.logger.func = slog_func;
    return desc;
}
