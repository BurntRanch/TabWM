#pragma once
#include <ctime>
#include <wayland-server.h>
#include <wayland-util.h>

extern "C" {
    #include <wlr/backend.h>
    #include <wlr/types/wlr_input_device.h>
    #include <wlr/interfaces/wlr_keyboard.h>
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
    struct wl_listener new_input_listener;

    struct wlr_renderer *renderer;
    struct wlr_allocator *allocator;

    struct wl_list device_inputs;
    struct wl_list device_outputs;
};

struct tabwm_output {
    struct wlr_output *output;
    struct tabwm_wl_server *server;
    struct timespec last_frame_presented;

    struct wl_listener output_rmd_listener;
    struct wl_listener frame_listener;

    struct wl_list link;
};

struct tabwm_input {
    struct wlr_input_device *input;
    struct tabwm_wl_server *server;

    struct timespec last_event_handled;

    /* could be a key, etc. */
    struct wl_listener input_event_listener;
    struct wl_listener input_rmd_listener;

    struct wl_list link;
};
