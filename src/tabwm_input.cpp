#include "tabwm_input.hpp"
#include <fmt/format.h>

void rm_input(struct wl_listener *listener, void *data) {
    struct tabwm_input *wm_input = wl_container_of(listener, wm_input, input_rmd_listener);
    wl_list_remove(&wm_input->link);
    wl_list_remove(&wm_input->input_event_listener.link);
    wl_list_remove(&wm_input->input_rmd_listener.link);
    free(wm_input);
}

void input_key(struct wl_listener *listener, void *data) {
    struct tabwm_input *wm_input = wl_container_of(listener, wm_input, input_event_listener);
    struct tabwm_wl_server *server = wm_input->server;
    fmt::println(server->log_fd, "Key event from input {}!", fmt::ptr(wm_input->input));
    fflush(server->log_fd);
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
