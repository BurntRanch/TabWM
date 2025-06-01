#include <tabwm_server.hpp>
#include <cassert>
#include <fmt/base.h>

static void rm_output(struct wl_listener *listener, void *data) {
    struct tabwm_output *wm_output = wl_container_of(listener, wm_output, output_rmd_listener);
    wl_list_remove(&wm_output->link);
    wl_list_remove(&wm_output->frame_listener.link);
    wl_list_remove(&wm_output->output_rmd_listener.link);
    free(wm_output);
}

static void output_frame(struct wl_listener *listener, void *data) {
    fmt::println("Frame!");

    struct tabwm_output *wm_output = wl_container_of(listener, wm_output, frame_listener);
    struct wlr_output *output = wm_output->output;

    struct wlr_output_state state{};
    wlr_output_state_init(&state);

    struct wlr_render_pass *render_pass = wlr_output_begin_render_pass(output, &state, NULL, NULL);
    // wlr_renderer_begin_buffer_pass(renderer, wm_output->state.buffer, NULL);

    struct wlr_box box{};
    box.x = 0;
    box.y = 0;
    box.width = output->width;
    box.height = output->height;

    fmt::println("width: {}, height: {}", box.width, box.height);

    struct wlr_render_color color{};
    color.r = 1.0f;
    color.g = 0.0f;
    color.b = 0.0f;
    color.a = 1.0f;

    struct wlr_render_rect_options rect_options{};
    rect_options.blend_mode = WLR_RENDER_BLEND_MODE_NONE;
    rect_options.box = box;
    rect_options.clip = NULL;
    rect_options.color = color;

    wlr_render_pass_add_rect(render_pass, &rect_options);

    wlr_render_pass_submit(render_pass);

    wlr_output_commit_state(output, &state);
    wlr_output_state_finish(&state);

    clock_gettime(CLOCK_MONOTONIC, &wm_output->last_frame_presented);
}

static void new_output(struct wl_listener *listener, void *data) {
    fmt::println("New output!");

    struct tabwm_wl_server *server = wl_container_of(listener, server, new_output_listener);
    struct wlr_output *output = reinterpret_cast<wlr_output *>(data);
   
    wlr_output_init_render(output, server->allocator, server->renderer);
    
    struct tabwm_output *wm_output = reinterpret_cast<tabwm_output *>(calloc(1, sizeof(tabwm_output)));

    clock_gettime(CLOCK_MONOTONIC, &wm_output->last_frame_presented);
    wm_output->server = server;
    wm_output->output = output;
    wl_list_insert(&wm_output->link, &wm_output->link);

    wm_output->output_rmd_listener.notify = rm_output;
    wl_signal_add(&output->events.destroy, &wm_output->output_rmd_listener);
    wm_output->frame_listener.notify = output_frame;
    wl_signal_add(&output->events.frame, &wm_output->frame_listener);

    struct wlr_output_state state{};
    wlr_output_state_init(&state);
    wlr_output_state_set_enabled(&state, true);

    struct wlr_output_mode *mode = wlr_output_preferred_mode(output);

    if (mode != NULL) {
        wlr_output_state_set_mode(&state, mode);
    }

    wlr_output_commit_state(output, &state);
    wlr_output_state_finish(&state);
}

int main(int argc, char **argv) {
    struct tabwm_wl_server server{};

    server.display = wl_display_create();
    assert(server.display);
    server.event_loop = wl_display_get_event_loop(server.display);
    assert(server.event_loop);

    server.backend = wlr_backend_autocreate(server.event_loop, nullptr);
    assert(server.backend);

    server.renderer = wlr_renderer_autocreate(server.backend);
    assert(server.renderer);
    server.allocator = wlr_allocator_autocreate(server.backend, server.renderer);
    assert(server.allocator);

    wl_list_init(&server.device_outputs);

    server.new_output_listener.notify = new_output;
    wl_signal_add(&server.backend->events.new_output, &server.new_output_listener);

    if (!wlr_backend_start(server.backend)) {
        fmt::println("fatal: wlr_backend_start returned false");
        wl_display_destroy(server.display);
        return EXIT_FAILURE;
    }

    /* this function enters a loop so we just wait for it to end before we destroy and return EXIT_SUCCESS */
    wl_display_run(server.display);

    wl_display_destroy(server.display);
    return EXIT_SUCCESS;
}
