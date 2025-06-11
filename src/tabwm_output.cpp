#include "tabwm_output.hpp"
#include "tabwm_input.hpp"
#include "tabwm_server.hpp"
#include "tabwm_surface.hpp"
#include "util.hpp"

#include <cassert>
#include <cstdio>
#include <ctime>
#include <fmt/base.h>
#include <fmt/format.h>
#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-server.h>
#include <wayland-util.h>

void rm_output(struct wl_listener *listener, void *_) {
    struct tabwm_output *wm_output = wl_container_of(listener, wm_output, output_rmd_listener);
    wl_list_remove(&wm_output->link);
    wl_list_remove(&wm_output->frame_listener.link);
    wl_list_remove(&wm_output->output_rmd_listener.link);
    free(wm_output);
}

void output_frame(struct wl_listener *listener, void *_) {
    struct tabwm_output *wm_output = wl_container_of(listener, wm_output, frame_listener);
    struct tabwm_wl_server *server = wm_output->server;

    struct wlr_scene_output_state_options options{};
    options.timer = &wm_output->scene_timer;
    options.color_transform = wlr_color_transform_init_srgb();
    if (!wlr_scene_output_commit(wm_output->scene_output, &options)) {
        fmt::println(server->log_fd, "Failed to render!");
        fflush(server->log_fd);
        return;
    }

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    wm_output->last_frame_presented = now;

    struct tabwm_surface *surface;
    wl_list_for_each(surface, &server->surfaces, link) {
        wlr_surface_send_frame_done(surface->surface, &now);
    }
}

void new_output(struct wl_listener *listener, void *data) {
    struct tabwm_wl_server *server = wl_container_of(listener, server, new_output_listener);
    struct wlr_output *output = reinterpret_cast<wlr_output *>(data);

    fmt::println(server->log_fd, "New output!");
    fflush(server->log_fd);
   
    wlr_output_init_render(output, server->allocator, server->renderer);

    struct tabwm_output *wm_output = reinterpret_cast<tabwm_output *>(calloc(1, sizeof(tabwm_output)));

    clock_gettime(CLOCK_MONOTONIC, &wm_output->last_frame_presented);
    wm_output->server = server;
    wm_output->output = output;

    wm_output->scene_output = wlr_scene_output_create(server->scene, output);

    wl_list_insert(wl_list_get_last_item(&server->device_outputs), &wm_output->link);

    wm_output->output_rmd_listener.notify = rm_output;
    wl_signal_add(&output->events.destroy, &wm_output->output_rmd_listener);
    wm_output->frame_listener.notify = output_frame;
    wl_signal_add(&output->events.frame, &wm_output->frame_listener);

    wm_output->scene_timer.render_timer = wlr_render_timer_create(wm_output->output->renderer);

    wlr_output_create_global(output, server->display);

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
