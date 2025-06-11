#pragma once
#include "server.hpp"

struct tab_output {
    struct wlr_output *output;
    struct tab_server *server;
    struct timespec last_frame_presented;

    struct wlr_scene_output *scene_output;
    struct wlr_scene_timer scene_timer;

    struct wl_listener output_rmd_listener;
    struct wl_listener frame_listener;

    struct wl_list link;
};

void rm_output(struct wl_listener *listener, void *_);

void new_output(struct wl_listener *listener, void *data);
