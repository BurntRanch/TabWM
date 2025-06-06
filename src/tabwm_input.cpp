#include "tabwm_input.hpp"
#include "tabwm_server.hpp"
#include <cassert>
#include <csignal>
#include <cstdint>
#include <fmt/format.h>
#include <unistd.h>
#include <wayland-server-core.h>
#include <xkbcommon/xkbcommon.h>

void rm_input(struct wl_listener *listener, void *_) {
    struct tabwm_input *wm_input = wl_container_of(listener, wm_input, input_rmd_listener);
    wl_list_remove(&wm_input->link);
    wl_list_remove(&wm_input->input_event_listener.link);
    wl_list_remove(&wm_input->input_rmd_listener.link);
    free(wm_input);
}

void input_key(struct wl_listener *listener, void *data) {
    struct tabwm_input *wm_input = wl_container_of(listener, wm_input, input_event_listener);
    struct tabwm_wl_server *server = wm_input->server;

    struct wlr_keyboard *keyboard = wlr_keyboard_from_input_device(wm_input->input);

    fmt::println(server->log_fd, "Key event from input {}!", fmt::ptr(wm_input->input));
    fflush(server->log_fd);

    struct wlr_keyboard_key_event *key_event = reinterpret_cast<wlr_keyboard_key_event *>(data);
    uint32_t xkb_keycode = key_event->keycode + 8;

    uint32_t modifiers = wlr_keyboard_get_modifiers(keyboard);

    fmt::println(server->log_fd, "modifiers: {}", modifiers);
    fflush(server->log_fd);

    /* if Win-ESC is pressed, terminate the display. (only once) */
    if ((modifiers & WLR_MODIFIER_LOGO) && xkb_keycode == 9 && !server->is_quitting) {
        server->is_quitting = true;
        wl_display_terminate(server->display);
    }

    clock_gettime(CLOCK_MONOTONIC, &wm_input->last_event_handled);
}

void new_input(struct wl_listener *listener, void *data) {
    struct tabwm_wl_server *server = wl_container_of(listener, server, new_input_listener);
    struct wlr_input_device *input = reinterpret_cast<wlr_input_device *>(data);

    fmt::println(server->log_fd, "New input (name: {}, ptr: {})!", input->name, fmt::ptr(data));
    fflush(server->log_fd);

    if (input->type != WLR_INPUT_DEVICE_KEYBOARD || strcmp(input->name, "Power Button") == 0) {
        fmt::println(server->log_fd, "Ignoring non-keyboard input device.");
        
        if (input->type == WLR_INPUT_DEVICE_KEYBOARD) {
            fmt::println(server->log_fd, "WARN: this input device is a power button pretending to be a keyboard. So we have disabled it.");
        }

        fflush(server->log_fd);
        return;
    }

    struct wlr_keyboard *keyboard = wlr_keyboard_from_input_device(input);

    struct xkb_rule_names rule_names{};
    rule_names.rules = "base";
    rule_names.layout = "us";
    rule_names.model = "pc105";

    struct xkb_keymap *keymap = xkb_keymap_new_from_names(server->xkb_context, &rule_names, XKB_KEYMAP_COMPILE_NO_FLAGS);

    assert(wlr_keyboard_set_keymap(keyboard, keymap));

    struct tabwm_input *wm_input = reinterpret_cast<tabwm_input *>(calloc(1, sizeof(tabwm_input)));
    wm_input->server = server;
    wm_input->input = input;

    clock_gettime(CLOCK_MONOTONIC, &wm_input->last_event_handled);

    wm_input->input_event_listener.notify = input_key;
    wl_signal_add(&keyboard->events.key, &wm_input->input_event_listener);
    wm_input->input_rmd_listener.notify = rm_input;
    wl_signal_add(&input->events.destroy, &wm_input->input_rmd_listener);

    int size = wl_list_length(&server->device_inputs);
    wl_list *tail = &server->device_inputs;
    for (int i = 0; i < size; i++) {
        tail = tail->next;
    }

    wl_list_insert(tail, &wm_input->link);
}
