#include "tabwm_output.hpp"
#include "tabwm_input.hpp"
#include <fmt/base.h>
#include <fmt/format.h>

void rm_output(struct wl_listener *listener, void *_) {
    struct tabwm_output *wm_output = wl_container_of(listener, wm_output, output_rmd_listener);
    wl_list_remove(&wm_output->link);
    wl_list_remove(&wm_output->frame_listener.link);
    wl_list_remove(&wm_output->output_rmd_listener.link);
    free(wm_output);
}

void output_frame(struct wl_listener *listener, void *_) {
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
        fmt::println(server->log_fd, "input {} has had a key event {}s ago!", fmt::ptr(wm_input->input), timediff);
        fflush(server->log_fd);
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

    fmt::println(server->log_fd, "Finished frame!");
    fflush(server->log_fd);

    wlr_output_schedule_frame(output);
}

void new_output(struct wl_listener *listener, void *data) {
    struct tabwm_wl_server *server = wl_container_of(listener, server, new_output_listener);
    struct wlr_output *output = reinterpret_cast<wlr_output *>(data);

    fmt::println(server->log_fd, "New output!");
    fflush(server->log_fd);
   
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
