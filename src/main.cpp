#include <cassert>

#include <cstdlib>
#include <tabwm_server.hpp>
#include <tabwm_input.hpp>
#include <tabwm_output.hpp>

#include <fmt/base.h>
#include <fmt/format.h>
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlroots-0.18/wlr/types/wlr_gamma_control_v1.h>

int main() {
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

    wl_list_init(&server.device_inputs);
    wl_list_init(&server.device_outputs);

    server.new_output_listener.notify = new_output;
    wl_signal_add(&server.backend->events.new_output, &server.new_output_listener);
    server.new_input_listener.notify = new_input;
    wl_signal_add(&server.backend->events.new_input, &server.new_input_listener);

    server.seat = wlr_seat_create(server.display, "seat0");
    assert(server.seat);

    server.wayland_socket = wl_display_add_socket_auto(server.display);
    assert(server.wayland_socket);

    server.log_fd = stdout;
    
    server.xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

    if (!wlr_backend_start(server.backend)) {
        fmt::println("fatal: wlr_backend_start returned false");
        wl_display_destroy(server.display);
        return EXIT_FAILURE;
    }

    fmt::println("Running on display {}", server.wayland_socket);
    setenv("WAYLAND_DISPLAY", server.wayland_socket, true);

    wl_display_init_shm(server.display);
    wlr_gamma_control_manager_v1_create(server.display);
    wlr_screencopy_manager_v1_create(server.display);
    wlr_primary_selection_v1_device_manager_create(server.display);
    wlr_idle_inhibit_v1_create(server.display);
    wlr_idle_notifier_v1_create(server.display);

    /* this function enters a loop so we just wait for it to end before we destroy and return EXIT_SUCCESS */
    wl_display_run(server.display);

    wl_display_destroy(server.display);
    return EXIT_SUCCESS;
}
