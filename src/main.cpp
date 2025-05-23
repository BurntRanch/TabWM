#include <tabwm_server.hpp>
#include <cassert>
#include <fmt/base.h>

static void rm_output(struct wl_listener *listener, void *data) {
    struct tabwm_output *wm_output = wl_container_of(listener, wm_output, output_rmd_listener);
    wl_list_remove(&wm_output->link);
    wl_list_remove(&wm_output->output_rmd_listener.link);
    free(wm_output);
}

static void output_frame(struct wl_listener *listener, void *data) {
    struct tabwm_output *wm_output = wl_container_of(listener, wm_output, frame_listener);
    struct wlr_output *output = wm_output->output;

    struct wlr_renderer *renderer = output->renderer;

    wlr_output_begin_render_pass(output, NULL, NULL, NULL);
    wlr_renderer_begin_buffer_pass(renderer, output->allocator)
}

static void new_output(struct wl_listener *listener, void *data) {
    struct tabwm_wl_server *server = wl_container_of(listener, server, new_output_listener);
    struct wlr_output *output = reinterpret_cast<wlr_output *>(data);

    struct wlr_output_state state{};
    wlr_output_state_init(&state);

    if (!wl_list_empty(&output->modes)) {
        struct wlr_output_mode *mode = wl_container_of(output->modes.prev, mode, link);
        wlr_output_state_set_mode(&state, mode);
    }

    struct wlr_buffer *buffer = wlr_buffer

    wlr_output_commit_state(output, &state);
    wlr_output_state_finish(&state);

    struct tabwm_output *wm_output = reinterpret_cast<tabwm_output *>(calloc(1, sizeof(tabwm_output)));
    clock_gettime(CLOCK_MONOTONIC, &wm_output->last_frame_presented);
    wm_output->server = server;
    wm_output->output = output;
    wl_list_insert(&wm_output->link, &wm_output->link);

    wm_output->output_rmd_listener.notify = rm_output;
    wl_signal_add(&output->events.destroy, &wm_output->output_rmd_listener);
    wm_output->frame_listener.notify = output_frame;
    wl_signal_add(&output->events.frame, &wm_output->frame_listener);
}

int main(int argc, char **argv) {
    struct tabwm_wl_server server{};

    server.display = wl_display_create();
    assert(server.display);
    server.event_loop = wl_display_get_event_loop(server.display);
    assert(server.event_loop);

    server.backend = wlr_backend_autocreate(server.display, nullptr);
    assert(server.backend);

    wl_list_init(&server.device_outputs);

    server.new_output_listener.notify = 

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
