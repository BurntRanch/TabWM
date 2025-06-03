#pragma once
#include "tabwm_server.hpp"

struct tabwm_output {
    struct wlr_output *output;
    struct tabwm_wl_server *server;
    struct timespec last_frame_presented;

    struct wl_listener output_rmd_listener;
    struct wl_listener frame_listener;

    struct wl_list link;
};

void rm_output(struct wl_listener *listener, void *_);

void new_output(struct wl_listener *listener, void *data);
