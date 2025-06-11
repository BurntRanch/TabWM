#include "tabwm_server.hpp"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <fmt/base.h>
#include <fmt/format.h>
#include <wayland-server-core.h>
#include <wayland-util.h>

#include <tabwm_surface.hpp>

void rm_surface(struct wl_listener *listener, void *_) {
    struct tabwm_surface *wm_surface = wl_container_of(listener, wm_surface, destroy_listener);
    struct tabwm_wl_server *server = wm_surface->server;

    fmt::println(server->log_fd, "removing surface!");
    fflush(server->log_fd);

    wlr_scene_node_destroy(&wm_surface->border_rect->node);

    wl_list_remove(&wm_surface->commit_listener.link);
    wl_list_remove(&wm_surface->destroy_listener.link);

    wl_list_remove(&wm_surface->link);
}

/* This just updates the border to fit the surface. No need for any more. */
void commit_surface(struct wl_listener *listener, void *_) {
    struct tabwm_surface *wm_surface = wl_container_of(listener, wm_surface, commit_listener);
    struct tabwm_wl_server *server = wm_surface->server;

    fmt::println(server->log_fd, "Surface {} commited!", fmt::ptr(wm_surface));
    fmt::println(server->log_fd, "w: {}, h: {}", wm_surface->surface->current.width, wm_surface->surface->current.height);
    fflush(server->log_fd);

    wlr_scene_rect_set_size(wm_surface->border_rect, 
                    wm_surface->surface->current.width + 2 * wm_surface->border_size, 
                    wm_surface->surface->current.height + 2 * wm_surface->border_size);
}

void new_surface(struct wl_listener *listener, void *data) {
    struct tabwm_wl_server *server = wl_container_of(listener, server, new_surface_listener);
    struct wlr_surface *surface = reinterpret_cast<wlr_surface *>(data);

    fmt::println(server->log_fd, "new surface! ({})", fmt::ptr(data));
    fflush(server->log_fd);

    struct tabwm_surface *wm_surface = reinterpret_cast<tabwm_surface *>(calloc(1, sizeof(tabwm_surface)));

    wm_surface->surface = surface;
    wm_surface->server = server;

    wm_surface->scene_surface_tree = wlr_scene_subsurface_tree_create(&server->scene->tree, surface);

    const std::array<float, 4> border_color = {1.0f, 1.0f, 1.0f, 1.0f};
    wm_surface->border_rect = wlr_scene_rect_create(&server->scene->tree, 0, 0, border_color.data());

    wm_surface->border_size = 2;

    wlr_scene_node_set_position(&wm_surface->border_rect->node, 0, 0);
    wlr_scene_node_set_position(&wm_surface->scene_surface_tree->node, wm_surface->border_size, wm_surface->border_size);

    wm_surface->commit_listener.notify = commit_surface;
    wl_signal_add(&surface->events.commit, &wm_surface->commit_listener);
    wm_surface->destroy_listener.notify = rm_surface;
    wl_signal_add(&surface->events.destroy, &wm_surface->destroy_listener);

    wl_list_insert(server->surfaces.prev, &wm_surface->link);
}
