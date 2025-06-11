#pragma once
#include <cstdio>
#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-server.h>
#include <wayland-util.h>
#include <xkbcommon/xkbcommon.h>

extern "C" {
    #include <wlr/backend.h>
    #include <wlr/types/wlr_input_device.h>
    #include <wlr/interfaces/wlr_keyboard.h>
    #include <wlr/types/wlr_gamma_control_v1.h>
    #include <wlr/types/wlr_screencopy_v1.h>
    #include <wlr/types/wlr_primary_selection_v1.h>
    #include <wlr/types/wlr_idle_inhibit_v1.h>
    #include <wlr/types/wlr_idle_notify_v1.h>
    #include <wlr/types/wlr_seat.h>
    #include <wlr/types/wlr_compositor.h>
    #include <wlr/types/wlr_xdg_shell.h>
    #include <wlr/types/wlr_scene.h>
    #include <wlr/render/color.h>
    #include <wlr/render/wlr_renderer.h>
    #include <wlr/render/interface.h>
    #include <wlr/render/allocator.h>
    #include <wlr/render/drm_format_set.h>
    #include <wlr/util/box.h>
    #include <drm/drm_fourcc.h>
}

struct tab_server {
    struct wl_display *display;
    struct wl_event_loop *event_loop;

    struct xkb_context *xkb_context;

    struct wlr_backend *backend;
    struct wlr_compositor *compositor;

    struct wlr_scene *scene;

    struct wl_listener new_surface_listener;
    struct wl_listener new_output_listener;
    struct wl_listener new_input_listener;

    struct wlr_renderer *renderer;
    struct wlr_allocator *allocator;

    /* a list of surfaces as individual tabwm_surfaces. */
    struct wl_list surfaces;

    struct wl_list device_inputs;
    struct wl_list device_outputs;

    struct wlr_seat *seat;

    const char *wayland_socket;

    bool is_quitting = false;

    FILE *log_fd;
};
