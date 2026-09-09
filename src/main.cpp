#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include <SDL.h>
#include <SDL_syswm.h>
#include <SDL_ttf.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "langswitch/layout_names.h"
#include "langswitch/popup_controller.h"
#include "langswitch/xkb_monitor.h"

namespace {

constexpr int kFontSizePx = 56;
constexpr int kPaddingX = 56;
constexpr int kPaddingY = 32;
constexpr int kBottomThirdDivisor = 6;  // popup center at 5/6 of screen height

constexpr const char* kFontCandidates[] = {
    "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
    "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
    "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf",
    "/usr/share/fonts/truetype/noto/NotoSans-Bold.ttf",
};

enum : Uint32 {
  kLayoutChangedEvent = SDL_USEREVENT,
  kQuitEvent = SDL_USEREVENT + 1,
};

std::atomic<bool> g_running{true};

void on_signal(int) {
  SDL_Event event{};
  event.type = kQuitEvent;
  SDL_PushEvent(&event);
}

bool index_in_range(const std::vector<std::string>& items, int index) {
  return index >= 0 && static_cast<size_t>(index) < items.size();
}

int64_t now_ms() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

std::string find_font() {
  for (const char* path : kFontCandidates) {
    if (SDL_LoadFile(path, nullptr) != nullptr) {
      return path;
    }
  }
  return {};
}

void configure_x11_window(SDL_Window* window) {
  SDL_SysWMinfo info;
  SDL_VERSION(&info.version);
  if (!SDL_GetWindowWMInfo(window, &info) ||
      info.subsystem != SDL_SYSWM_X11) {
    return;
  }
  Display* display = info.info.x11.display;
  ::Window xwindow = info.info.x11.window;

  auto atom = [display](const char* name) {
    return XInternAtom(display, name, False);
  };

  Atom notification = atom("_NET_WM_WINDOW_TYPE_NOTIFICATION");
  Atom window_type = atom("_NET_WM_WINDOW_TYPE");
  XChangeProperty(display, xwindow, window_type, XA_ATOM, 32, PropModeReplace,
                  reinterpret_cast<unsigned char*>(&notification), 1);

  Atom skip_taskbar = atom("_NET_WM_STATE_SKIP_TASKBAR");
  Atom skip_pager = atom("_NET_WM_STATE_SKIP_PAGER");
  Atom above = atom("_NET_WM_STATE_ABOVE");
  Atom state = atom("_NET_WM_STATE");
  Atom states[] = {skip_taskbar, skip_pager, above};
  XChangeProperty(display, xwindow, state, XA_ATOM, 32, PropModeReplace,
                  reinterpret_cast<unsigned char*>(states), 3);

  // Make the window override-redirect before it is ever mapped: the window
  // manager then ignores it completely, so it never appears in the taskbar
  // and is always drawn above regular (managed) windows.
  XSetWindowAttributes attributes{};
  attributes.override_redirect = True;
  XChangeWindowAttributes(display, xwindow, CWOverrideRedirect, &attributes);
  XSync(display, False);
}

class Popup {
public:
  bool init(std::string* error) {
    font_path_ = find_font();
    if (font_path_.empty()) {
      *error = "no usable TrueType font found";
      return false;
    }
    font_ = TTF_OpenFont(font_path_.c_str(), kFontSizePx);
    if (!font_) {
      *error = std::string("cannot open font: ") + TTF_GetError();
      return false;
    }
    window_ = SDL_CreateWindow("lang-switch", 0, 0, 100, 50,
                               SDL_WINDOW_BORDERLESS | SDL_WINDOW_HIDDEN |
                                   SDL_WINDOW_SKIP_TASKBAR | SDL_WINDOW_ALWAYS_ON_TOP);
    if (!window_) {
      *error = std::string("cannot create window: ") + SDL_GetError();
      return false;
    }
    return true;
  }

  void show(const std::string& text) {
    SDL_Surface* rendered =
        TTF_RenderUTF8_Blended(font_, text.c_str(),
                               SDL_Color{240, 240, 240, 255});
    if (!rendered) {
      return;
    }
    const int width = rendered->w + kPaddingX * 2;
    const int height = rendered->h + kPaddingY * 2;

    int display_width = 0;
    int display_height = 0;
    SDL_DisplayMode mode{};
    if (SDL_GetDesktopDisplayMode(0, &mode) == 0) {
      display_width = mode.w;
      display_height = mode.h;
    }
    const int x = (display_width - width) / 2;
    const int y = display_height * 5 / kBottomThirdDivisor - height / 2;

    SDL_SetWindowSize(window_, width, height);
    SDL_SetWindowPosition(window_, x, y);

    SDL_Surface* surface = SDL_GetWindowSurface(window_);
    if (surface) {
      SDL_FillRect(surface, nullptr,
                   SDL_MapRGBA(surface->format, 32, 32, 32, 255));
      SDL_Rect dst{(surface->w - rendered->w) / 2,
                   (surface->h - rendered->h) / 2, rendered->w,
                   rendered->h};
      SDL_BlitSurface(rendered, nullptr, surface, &dst);
      SDL_UpdateWindowSurface(window_);
    }
    SDL_FreeSurface(rendered);

    // Re-apply every time: SDL recreates the underlying X window when it
    // first creates the window framebuffer, wiping custom attributes.
    configure_x11_window(window_);
    SDL_ShowWindow(window_);
    SDL_RaiseWindow(window_);
    shown_ = true;
  }

  void hide() {
    if (shown_) {
      SDL_HideWindow(window_);
      shown_ = false;
    }
  }

  ~Popup() {
    if (window_) {
      SDL_DestroyWindow(window_);
    }
    if (font_) {
      TTF_CloseFont(font_);
    }
  }

private:
  std::string font_path_;
  TTF_Font* font_ = nullptr;
  SDL_Window* window_ = nullptr;
  bool shown_ = false;
};

}  // namespace

int main() {
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
    return 1;
  }
  if (TTF_Init() != 0) {
    std::fprintf(stderr, "TTF_Init failed: %s\n", TTF_GetError());
    SDL_Quit();
    return 1;
  }
  std::signal(SIGINT, on_signal);
  std::signal(SIGTERM, on_signal);

  std::string error;
  const auto raw_list = langswitch::XkbMonitor::query_layout_list(&error);
  if (raw_list) {
    std::printf("Layouts: %s\n", raw_list->c_str());
  } else {
    std::fprintf(stderr, "warning: %s\n", error.c_str());
  }
  const std::vector<std::string> layouts =
      langswitch::parse_layout_list(raw_list.value_or(""));

  Popup popup;
  if (!popup.init(&error)) {
    std::fprintf(stderr, "popup init failed: %s\n", error.c_str());
    SDL_Quit();
    return 1;
  }

  langswitch::XkbMonitor monitor;
  if (!monitor.start(
          [](int layout_index) {
            SDL_Event event{};
            event.type = kLayoutChangedEvent;
            event.user.code = 0;
            event.user.data1 =
                reinterpret_cast<void*>(static_cast<intptr_t>(layout_index));
            SDL_PushEvent(&event);
          },
          &error)) {
    std::fprintf(stderr, "cannot start XKB monitor: %s\n", error.c_str());
    SDL_Quit();
    return 1;
  }

  std::printf("lang-switch is running. Press Ctrl+C to quit.\n");

  langswitch::PopupController controller;
  while (g_running) {
    SDL_Event event;
    const int64_t current = now_ms();
    const int timeout = static_cast<int>(controller.remaining_ms(current));
    const bool got_event = SDL_WaitEventTimeout(
        &event, timeout > 0 ? timeout : (controller.visible_layout(current)
                                             ? 50
                                             : -1));
    if (got_event) {
      if (event.type == kQuitEvent) {
        break;
      }
      if (event.type == SDL_QUIT) {
        break;
      }
      if (event.type == kLayoutChangedEvent) {
        const auto index = static_cast<int>(
            reinterpret_cast<intptr_t>(event.user.data1));
        controller.on_layout_changed(index, now_ms());
      }
    }

    const int64_t now = now_ms();
    const auto visible = controller.visible_layout(now);
    if (visible) {
      std::string text;
      if (index_in_range(layouts, *visible)) {
        text = langswitch::popup_text(layouts[static_cast<size_t>(*visible)]);
      } else {
        text = "Layout " + std::to_string(*visible + 1);
      }
      popup.show(text);
    } else {
      popup.hide();
    }
  }

  monitor.stop();
  SDL_Quit();
  return 0;
}
