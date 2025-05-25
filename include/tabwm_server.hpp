#pragma once
#include <wayland-server.h>

extern "C" {
    #include <wlr/backend.h>
    #include <wlr/render/interface.h>
    #include <wlr/render/allocator.h>
    #include <wlr/render/drm_format_set.h>
    #include <drm/drm_fourcc.h>
}

struct tabwm_wl_server {
    struct wl_display *display;
    struct wl_event_loop *event_loop;

    struct wlr_backend *backend;

    struct wl_listener new_output_listener;

    struct wl_list device_outputs;
};

struct tabwm_output {
    struct wlr_output *output;
    struct tabwm_wl_server *server;
    struct timespec last_frame_presented;

    struct wl_listener output_rmd_listener;
    struct wl_listener frame_listener;

    struct wlr_buffer *buffer;

    struct wl_list link;
};
