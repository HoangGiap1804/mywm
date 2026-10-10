#include <iostream>
#include <math.h>

extern "C" {
#define static
#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/xcursor.h>
#undef static
}

int main() {
  wl_display *display = wl_display_create();
  if (!display) {
    std::cerr << "Khong the tao Wayland display!" << std::endl;
    return 1;
  }

  std::cout << "Khoi tao mywm thanh cong!" << std::endl;
  wl_display_destroy(display);
  return 0;
}
