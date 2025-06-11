#pragma once
#include <tabwm_server.hpp>
#include <wayland-server-core.h>
#include <wayland-util.h>

struct tabwm_surface {
    struct wlr_surface *surface;
    struct tabwm_wl_server *server;

    struct wlr_scene_tree *scene_surface_tree;
    struct wlr_scene_rect *border_rect;

    struct wl_listener commit_listener;
    struct wl_listener destroy_listener;

    int border_size = 2;

    struct wl_list link;
};

void rm_surface(struct wl_listener *listener, void *data);
void commit_surface(struct wl_listener *listener, void *data);
void new_surface(struct wl_listener *listener, void *data);
