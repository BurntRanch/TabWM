#include <cstdio>
#include <ctime>
#include <tabwm_server.hpp>
#include <cassert>
#include <fmt/base.h>
#include <fmt/format.h>
#include <wayland-server-core.h>
#include <wayland-util.h>

FILE *log_fd = nullptr;

static void rm_output(struct wl_listener *listener, void *data) {
    struct tabwm_output *wm_output = wl_container_of(listener, wm_output, output_rmd_listener);
    wl_list_remove(&wm_output->link);
    wl_list_remove(&wm_output->frame_listener.link);
    wl_list_remove(&wm_output->output_rmd_listener.link);
    free(wm_output);
}

static void output_frame(struct wl_listener *listener, void *data) {
    struct tabwm_output *wm_output = wl_container_of(listener, wm_output, frame_listener);
    struct tabwm_wl_server *server = wm_output->server;
    struct wlr_output *output = wm_output->output;

    struct wlr_output_state state{};
    wlr_output_state_init(&state);

    struct wlr_render_pass *render_pass = wlr_output_begin_render_pass(output, &state, NULL, NULL);
    // wlr_renderer_begin_buffer_pass(renderer, wm_output->state.buffer, NULL);

    struct wlr_box box{};
    box.x = 0;
    box.y = 0;
    box.width = 400;
    box.height = 400;

    /* If there were any recent inputs, set this to true so that we can change the color. */
    bool any_recent_inputs = false;
    struct tabwm_input *wm_input;
    wl_list_for_each(wm_input, &server->device_inputs, link) {
        double timediff = difftime(wm_output->last_frame_presented.tv_sec, wm_input->last_event_handled.tv_sec);
        fmt::println(log_fd, "input {} has had a key event {}s ago!", fmt::ptr(wm_input->input), timediff);
        fflush(log_fd);
        if (timediff < 1.0) {
            any_recent_inputs = true;
            break;
        }
    }

    struct wlr_render_color color{};
    color.r = 1.0f * !any_recent_inputs;
    color.g = 1.0f * any_recent_inputs;
    color.b = 1.0f * any_recent_inputs;
    color.a = 1.0f;

    struct wlr_render_rect_options rect_options{};
    rect_options.blend_mode = WLR_RENDER_BLEND_MODE_NONE;
    rect_options.box = box;
    rect_options.clip = NULL;
    rect_options.color = color;

    wlr_render_pass_add_rect(render_pass, &rect_options);

    wlr_render_pass_submit(render_pass);

    wlr_output_commit_state(output, &state);
    wlr_output_state_finish(&state);

    clock_gettime(CLOCK_MONOTONIC, &wm_output->last_frame_presented);

    fmt::println(log_fd, "Finished frame!");
    fflush(log_fd);

    wlr_output_schedule_frame(output);
}

static void new_output(struct wl_listener *listener, void *data) {
    fmt::println(log_fd, "New output!");
    fflush(log_fd);

    struct tabwm_wl_server *server = wl_container_of(listener, server, new_output_listener);
    struct wlr_output *output = reinterpret_cast<wlr_output *>(data);
   
    wlr_output_init_render(output, server->allocator, server->renderer);

    struct tabwm_output *wm_output = reinterpret_cast<tabwm_output *>(calloc(1, sizeof(tabwm_output)));

    clock_gettime(CLOCK_MONOTONIC, &wm_output->last_frame_presented);
    wm_output->server = server;
    wm_output->output = output;
    
    int size = wl_list_length(&server->device_outputs);
    wl_list *tail = &server->device_outputs;
    for (int i = 0; i < size; i++) {
        tail = tail->next;
    }

    wl_list_insert(tail, &wm_output->link);

    wm_output->output_rmd_listener.notify = rm_output;
    wl_signal_add(&output->events.destroy, &wm_output->output_rmd_listener);
    wm_output->frame_listener.notify = output_frame;
    wl_signal_add(&output->events.frame, &wm_output->frame_listener);

    struct wlr_output_state state{};
    wlr_output_state_init(&state);
    wlr_output_state_set_enabled(&state, true);

    struct wlr_output_mode *mode = wlr_output_preferred_mode(output);

    if (mode != NULL) {
        wlr_output_state_set_mode(&state, mode);
    }

    wlr_output_commit_state(output, &state);
    wlr_output_state_finish(&state);
}

static void rm_input(struct wl_listener *listener, void *data) {
    struct tabwm_input *wm_input = wl_container_of(listener, wm_input, input_rmd_listener);
    wl_list_remove(&wm_input->link);
    wl_list_remove(&wm_input->input_event_listener.link);
    wl_list_remove(&wm_input->input_rmd_listener.link);
    free(wm_input);
}

static void input_key(struct wl_listener *listener, void *data) {
    struct tabwm_input *wm_input = wl_container_of(listener, wm_input, input_event_listener);
    fmt::println(log_fd, "Key event from input {}!", fmt::ptr(wm_input->input));
    fflush(log_fd);
    clock_gettime(CLOCK_MONOTONIC, &wm_input->last_event_handled);
}

static void new_input(struct wl_listener *listener, void *data) {
    struct tabwm_wl_server *server = wl_container_of(listener, server, new_input_listener);
    struct wlr_input_device *input = reinterpret_cast<wlr_input_device *>(data);

    fmt::println(log_fd, "New input (name: {}, ptr: {})!", input->name, fmt::ptr(data));
    fflush(log_fd);

    if (input->type != WLR_INPUT_DEVICE_KEYBOARD) {
        fmt::println(log_fd, "Ignoring non-keyboard input device.");
        fflush(log_fd);
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

int main(int argc, char **argv) {
    struct tabwm_wl_server server{};

    server.display = wl_display_create();
    assert(server.display);
    server.event_loop = wl_display_get_event_loop(server.display);
    assert(server.event_loop);

    server.backend = wlr_backend_autocreate(server.event_loop, nullptr);
    assert(server.backend);

    server.renderer = wlr_renderer_autocreate(server.backend);
    assert(server.renderer);
    server.allocator = wlr_allocator_autocreate(server.backend, server.renderer);
    assert(server.allocator);

    wl_list_init(&server.device_inputs);
    wl_list_init(&server.device_outputs);

    server.new_output_listener.notify = new_output;
    wl_signal_add(&server.backend->events.new_output, &server.new_output_listener);
    server.new_input_listener.notify = new_input;
    wl_signal_add(&server.backend->events.new_input, &server.new_input_listener);

    log_fd = fopen("log.txt", "w+");

    if (!wlr_backend_start(server.backend)) {
        fmt::println("fatal: wlr_backend_start returned false");
        wl_display_destroy(server.display);
        return EXIT_FAILURE;
    }

    /* this function enters a loop so we just wait for it to end before we destroy and return EXIT_SUCCESS */
    wl_display_run(server.display);

    wl_display_destroy(server.display);
    return EXIT_SUCCESS;
}
