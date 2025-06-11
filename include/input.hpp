#pragma once
#include "server.hpp"

struct tab_input {
    struct wlr_input_device *input;
    struct tab_server *server;

    struct timespec last_event_handled;

    /* could be a key, etc. */
    struct wl_listener input_event_listener;
    struct wl_listener input_rmd_listener;

    struct wl_list link;
};

void rm_input(struct wl_listener *listener, void *data);

void input_key(struct wl_listener *listener, void *data);

void new_input(struct wl_listener *listener, void *data);
