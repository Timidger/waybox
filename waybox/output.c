#include <waybox/output.h>

void output_frame_notify(struct wl_listener *listener, void *data) {
	struct wb_output *output = wl_container_of(listener, output, frame);
//	struct wlr_backend *backend = output->server->backend;
	struct wlr_output *wlr_output = data;
	struct wlr_renderer *renderer = wlr_backend_get_renderer(
								wlr_output->backend);

	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);

	wlr_output_attach_render(wlr_output, NULL);
	wlr_renderer_begin(renderer, wlr_output->width, wlr_output->height);

	float color[4] = {0.4f, 0.4f, 0.4f, 1.0f};
	wlr_renderer_clear(renderer, color);

	wlr_output_commit(wlr_output);
	wlr_renderer_end(renderer);
	output->last_frame = now;
}

void output_destroy_notify(struct wl_listener *listener, void *data) {
	struct wb_output *output = wl_container_of(listener, output, destroy);
	wlr_output_layout_remove(output->server->layout, output->wlr_output);
	wl_list_remove(&output->link);
	wl_list_remove(&output->destroy.link);
	wl_list_remove(&output->frame.link);
	free(output);
}

void new_output_notify(struct wl_listener *listener, void *data) {
	struct wb_server *server = wl_container_of(
			listener, server, new_output
			);
	struct wlr_output *wlr_output = data;

	if (!wl_list_empty(&wlr_output->modes)) {
		struct wlr_output_mode *mode =
			wl_container_of(wlr_output->modes.prev, mode, link);
		wlr_output_set_mode(wlr_output, mode);

		wlr_output_create_global(wlr_output);
	}

	struct wb_output *output = calloc(1, sizeof(struct wb_output));
	clock_gettime(CLOCK_MONOTONIC, &output->last_frame);
	output->server = server;
	output->wlr_output = wlr_output;
	wl_list_insert(&server->outputs, &output->link);

	output->destroy.notify = output_destroy_notify;
	wl_signal_add(&wlr_output->events.destroy, &output->destroy);
	output->frame.notify = output_frame_notify;
	wl_signal_add(&wlr_output->events.frame, &output->frame);

	wlr_output_layout_add_auto(server->layout, output->wlr_output);
	wlr_xcursor_manager_load(server->cursor->xcursor_manager, output->wlr_output->scale);
	wlr_xcursor_manager_set_cursor_image(server->cursor->xcursor_manager, "left_ptr", server->cursor->cursor);
}
