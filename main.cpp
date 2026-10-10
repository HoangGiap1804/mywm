#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <math.h>
#include <wayland-util.h>

extern "C" {
#define static
#include <wayland-server-core.h>

#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/util/log.h>
#undef static
}

struct tinywl_server {
  struct wl_display *display;
  struct wl_event_loop *event_loop;
  struct wlr_backend *backend;
  struct wlr_renderer *renderer;
  struct wlr_allocator *allocator;

  struct wlr_compositor *compositor;
  struct wlr_subcompositor *subcompositor;
  struct wlr_data_device_manager *data_device_manager;
  struct wlr_output_layout *output_layout;

  struct wlr_scene *scene;

  struct wl_list *outputs;
  struct wl_listener new_outputs{};

  struct wl_listener output_frame{};
  struct wlr_scene_output *scene_output = nullptr;
};

static tinywl_server *server = nullptr;

static void handle_output_frame(wl_listener *listener, void *data) {
  (void)listener;
  (void)data;

  if (server->scene_output == nullptr) {
    return;
  }

  // Render scene và commit frame lên output.
  if (!wlr_scene_output_commit(server->scene_output, nullptr)) {
    std::fprintf(stderr, "Failed to render output frame\n");
  }
}

static void handle_new_output(wl_listener *listener, void *data) {
  (void)listener;

  auto *output = static_cast<wlr_output *>(data);

  // Initialize renderer and allocator for the output.
  if (!wlr_output_init_render(output, server->allocator, server->renderer)) {
    std::fprintf(stderr, "Failed to initialize output\n");
    return;
  }

  // Initialize output state.
  wlr_output_state state;
  wlr_output_state_init(&state);

  // Enable the output.
  wlr_output_state_set_enabled(&state, true);

  // Select preferred mode if available.
  if (!wl_list_empty(&output->modes)) {
    auto *mode = wlr_output_preferred_mode(output);

    if (mode != nullptr) {
      wlr_output_state_set_mode(&state, mode);
    }
  }

  // Apply the output configuration.
  bool committed = wlr_output_commit_state(output, &state);

  // Release state resources.
  wlr_output_state_finish(&state);

  if (!committed) {
    std::fprintf(stderr, "Failed to commit output\n");
    return;
  }

  // Add output to layout.
  wlr_output_layout_add_auto(server->output_layout, output);

  // Get output resolution.
  int width = 0;
  int height = 0;

  wlr_output_effective_resolution(output, &width, &height);

  // Create a dark blue background.
  const float color[4] = {0.15f, 0.20f, 0.30f, 0.2f};

  wlr_scene_rect_create(&server->scene->tree, width, height, color);

  // Create scene output.
  server->scene_output = wlr_scene_output_create(server->scene, output);

  // Listen for output frame events.
  server->output_frame.notify = handle_output_frame;

  wl_signal_add(&output->events.frame, &server->output_frame);

  std::printf("Output initialized: %dx%d\n", width, height);
}

int main() {
  struct tinywl_server s;
  server = &s;

  s.display = wl_display_create();
  if (!s.display) {
    std::cerr << "Failed to create display" << std::endl;
    return EXIT_FAILURE;
  }

  s.event_loop = wl_display_get_event_loop(s.display);
  s.backend = wlr_backend_autocreate(s.event_loop, NULL);
  if (!s.backend) {
    std::cerr << "Failed to create backend" << std::endl;
    return EXIT_FAILURE;
  }

  s.renderer = wlr_renderer_autocreate(s.backend);
  if (!s.renderer) {
    std::cerr << "Failed to create renderer" << std::endl;

    wlr_backend_destroy(s.backend);
    wl_display_destroy(s.display);

    return EXIT_FAILURE;
  }

  if (!wlr_renderer_init_wl_display(s.renderer, s.display)) {
    std::fprintf(stderr, "Failed to initialize renderer\n");
    wlr_renderer_destroy(s.renderer);
    wlr_backend_destroy(s.backend);
    wl_display_destroy(s.display);
    return EXIT_FAILURE;
  }

  s.allocator = wlr_allocator_autocreate(s.backend, s.renderer);
  if (!s.allocator) {
    std::fprintf(stderr, "Failed to create allocator\n");
    wlr_renderer_destroy(s.renderer);
    wlr_backend_destroy(s.backend);
    wl_display_destroy(s.display);
    return EXIT_FAILURE;
  }

  s.compositor = wlr_compositor_create(s.display, 6, s.renderer);

  if (!s.compositor) {
    std::fprintf(stderr, "Failed to create compositori\n");
    return EXIT_FAILURE;
  }

  // server.subcompositor = wlr_subcompositor_create(server.display);
  // server.data_device_manager =
  // wlr_data_device_manager_create(server.display);

  s.output_layout = wlr_output_layout_create(s.display);
  s.scene = wlr_scene_create();

  if (!s.scene) {
    std::fprintf(stderr, "Failed to create scene\n");
    return EXIT_FAILURE;
  }

  // Register new output listener.
  s.new_outputs.notify = handle_new_output;

  wl_signal_add(&s.backend->events.new_output, &s.new_outputs);

  // Create Wayland socket.
  const char *socket = wl_display_add_socket_auto(s.display);

  if (!socket) {
    std::fprintf(stderr, "Failed to create Wayland socket\n");
    wl_list_remove(&s.new_outputs.link);
    wl_display_destroy(s.display);
    return EXIT_FAILURE;
  }

  // Start backend.
  if (!wlr_backend_start(s.backend)) {
    std::fprintf(stderr, "Failed to start backend\n");
    wl_list_remove(&s.new_outputs.link);
    wl_display_destroy(s.display);
    return EXIT_FAILURE;
  }

  std::printf("MyWM is running on WAYLAND_DISPLAY=%s\n", socket);

  // Keep compositor running.
  wl_display_run(s.display);

  // Cleanup after the event loop exits.
  wl_display_destroy_clients(s.display);
  wl_list_remove(&s.new_outputs.link);
  wl_display_destroy(s.display);

  return EXIT_SUCCESS;
}
