#include "util.hpp"
#include <cassert>

#include <cstdio>
#include <cstdlib>
#include <server.hpp>
#include <input.hpp>
#include <output.hpp>
#include <surface.hpp>

#include <fmt/base.h>
#include <fmt/format.h>
#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-util.h>

int main() {
    struct tab_server server{};

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

    server.compositor = wlr_compositor_create(server.display, 6, server.renderer);
    assert(server.compositor);

    server.scene = wlr_scene_create();

    wlr_xdg_shell_create(server.display, 6);
    
    wl_display_init_shm(server.display);
    wlr_gamma_control_manager_v1_create(server.display);
    wlr_screencopy_manager_v1_create(server.display);
    wlr_primary_selection_v1_device_manager_create(server.display);
    wlr_idle_inhibit_v1_create(server.display);
    wlr_idle_notifier_v1_create(server.display);

    wl_list_init(&server.surfaces);
    wl_list_init(&server.device_inputs);
    wl_list_init(&server.device_outputs);

    server.new_surface_listener.notify = new_surface;
    wl_signal_add(&server.compositor->events.new_surface, &server.new_surface_listener);
    server.new_output_listener.notify = new_output;
    wl_signal_add(&server.backend->events.new_output, &server.new_output_listener);
    server.new_input_listener.notify = new_input;
    wl_signal_add(&server.backend->events.new_input, &server.new_input_listener);

    server.seat = wlr_seat_create(server.display, "seat0");
    assert(server.seat);
    wlr_seat_set_capabilities(server.seat, WL_SEAT_CAPABILITY_KEYBOARD);

    server.wayland_socket = wl_display_add_socket_auto(server.display);
    assert(server.wayland_socket);

    server.log_fd = stdout;
    
    server.xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

    if (!wlr_backend_start(server.backend)) {
        fmt::println(server.log_fd, "fatal: wlr_backend_start returned false");
        fflush(server.log_fd);
        wl_display_destroy(server.display);
        return EXIT_FAILURE;
    }

    fmt::println(server.log_fd, "Running on display {}", server.wayland_socket);
    fflush(server.log_fd);
    setenv("WAYLAND_DISPLAY", server.wayland_socket, true);

    /* this function enters a loop so we just wait for it to end before we destroy and return EXIT_SUCCESS */
    wl_display_run(server.display);

    wl_display_destroy(server.display);
    return EXIT_SUCCESS;
}
