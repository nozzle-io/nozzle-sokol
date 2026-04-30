// nozzle-sokol receiver example: receives a shared texture via nozzle and displays it
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

static struct {
    sg_pass_action pass_action;
    sg_pipeline pip;
    sg_bindings bind;
    NozzleReceiver *receiver;
    sg_image shared_img;
    bool has_texture;
    int frame_count;
} state;

static void init(void) {
    sg_desc desc{};
    desc.environment = sglue_environment();
    desc.logger.func = slog_func;
    sg_setup(&desc);

    sg_shader shader = sg_make_shader(&(sg_shader_desc){
        .vs.source =
            "#version 330\n"
            "layout(location=0) in vec2 position;\n"
            "layout(location=1) in vec2 texcoord0;\n"
            "out vec2 uv;"
            "void main() {\n"
            "  gl_Position = vec4(position, 0.0, 1.0);\n"
            "  uv = texcoord0;\n"
            "}\n",
        .fs.source =
            "#version 330\n"
            "uniform sampler2D tex;"
            "in vec2 uv;\n"
            "out vec4 frag_color;\n"
            "void main() {\n"
            "  frag_color = texture(tex, uv);\n"
            "}\n",
        .fs.images[0] = {.used = true, .image_type = SG_IMAGETYPE_2D, .sample_type = SG_IMAGESAMPLETYPE_FLOAT},
    });

    state.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shader,
        .layout = {
            .attrs = {
                [0].format = SG_VERTEXFORMAT_FLOAT2,
                [1].format = SG_VERTEXFORMAT_FLOAT2,
            }
        },
    });

    float vertices[] = {
        -1.0f, -1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 1.0f,
         1.0f,  1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f,  0.0f, 0.0f,
    };
    uint16_t indices[] = {0, 1, 2, 0, 2, 3};

    sg_buffer vbuf = sg_make_buffer(&(sg_buffer_desc){
        .data = SG_RANGE(vertices),
    });
    sg_buffer ibuf = sg_make_buffer(&(sg_buffer_desc){
        .type = SG_BUFFERTYPE_INDEXBUFFER,
        .data = SG_RANGE(indices),
    });

    state.bind.vertex_buffers[0] = vbuf;
    state.bind.index_buffer = ibuf;

    state.pass_action.colors[0].load_action = SG_LOADACTION_CLEAR;
    state.pass_action.colors[0].clear_value = (sg_color){0.0f, 0.0f, 0.0f, 1.0f};

    NozzleReceiverDesc rdesc{};
    rdesc.name = "sokol_sender";
    rdesc.application_name = "nozzle-sokol-receiver";
    if (nozzle_receiver_create(&rdesc, &state.receiver) != NOZZLE_OK) {
        fprintf(stderr, "failed to connect to nozzle sender\n");
    }
}

static void frame(void) {
    if (state.receiver) {
        NozzleAcquireDesc acq{};
        NozzleFrame *frame = nullptr;
        if (nozzle_receiver_acquire_frame(state.receiver, &acq, &frame) == NOZZLE_OK && frame) {
            sg_image new_img{};
            if (nozzle_sokol_frame_to_image(frame, &new_img)) {
                if (state.has_texture) {
                    sg_destroy_image(state.shared_img);
                }
                state.shared_img = new_img;
                state.bind.images[0] = new_img;
                state.has_texture = true;
            }
            nozzle_frame_release(frame);
        }
    }

    sg_begin_pass(&(sg_pass){.action = state.pass_action, .swapchain = sglue_swapchain()});
    if (state.has_texture) {
        sg_apply_pipeline(state.pip);
        sg_apply_bindings(&state.bind);
        sg_draw(0, 6, 1);
    }
    sg_end_pass();
    sg_commit();

    state.frame_count++;
}

static void cleanup(void) {
    if (state.has_texture) {
        sg_destroy_image(state.shared_img);
    }
    if (state.receiver) {
        nozzle_receiver_destroy(state.receiver);
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
    desc.window_title = "nozzle-sokol receiver";
    desc.logger.func = slog_func;
    return desc;
}
