#include "langswitch/xkb_monitor.h"

#include "langswitch/popup_controller.h"

#include <cstdio>
#include <cstdlib>

#include <X11/Xatom.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XKB.h>
#include <X11/extensions/XKBstr.h>

namespace langswitch {
namespace {
// Event mask for ClientMessage events (same value as in X11/Xproto.h).
constexpr long kClientMessageMask = 1L << 11;
}  // namespace

XkbMonitor::~XkbMonitor() { stop(); }

bool XkbMonitor::start(GroupCallback callback, std::string* error) {
  if (running_) {
    if (error) {
      *error = "monitor already running";
    }
    return false;
  }

  display_ = XOpenDisplay(nullptr);
  if (!display_) {
    if (error) {
      *error = "cannot open X display (is an X session running?)";
    }
    return false;
  }

  int op_code = 0;
  int error_base = 0;
  int major = XkbMajorVersion;
  int minor = XkbMinorVersion;
  if (!XkbQueryExtension(display_, &op_code, &xkb_event_type_, &error_base,
                         &major, &minor)) {
    if (error) {
      *error = "XKB extension is not available";
    }
    XCloseDisplay(display_);
    display_ = nullptr;
    return false;
  }

  // Input-only helper window used to receive XKB events and stop notifications.
  window_ = XCreateSimpleWindow(display_, RootWindow(display_, 0), 0, 0, 1, 1,
                                0, 0, 0);
  wake_atom_ = XInternAtom(display_, "LANGSWITCH_WAKE", False);
  XSelectInput(display_, window_, ExposureMask | kClientMessageMask);
  XkbSelectEvents(display_, XkbUseCoreKbd, XkbStateNotifyMask,
                  XkbStateNotifyMask);
  XSync(display_, False);

  callback_ = std::move(callback);
  running_ = true;
  thread_ = std::thread(&XkbMonitor::run, this);
  return true;
}

void XkbMonitor::stop() {
  if (!thread_.joinable()) {
    return;
  }
  running_ = false;
  XEvent event{};
  event.type = ClientMessage;
  event.xclient.window = window_;
  event.xclient.message_type = wake_atom_;
  event.xclient.format = 8;
  XSendEvent(display_, window_, False, kClientMessageMask, &event);
  XFlush(display_);
  thread_.join();
}

void XkbMonitor::run() {
  while (running_) {
    XEvent event;
    XNextEvent(display_, &event);
    if (const char* dbg = std::getenv("LANGSWITCH_DEBUG")) {
      std::fprintf(stderr, "xkb monitor: event type=%d (want %d)\n",
                   event.type, xkb_event_type_);
    }
    if (!running_) {
      break;
    }
    if (event.type != xkb_event_type_) {
      continue;
    }
    const auto& xkb = reinterpret_cast<const XkbEvent&>(event);
    if (xkb.any.xkb_type != XkbStateNotify) {
      continue;
    }
    const int group = xkb.state.group;
    if (group == last_group_) {
      continue;
    }
    last_group_ = group;
    if (callback_) {
      callback_(xkb_group_to_index(group));
    }
  }
}

std::optional<std::string> XkbMonitor::query_layout_list(std::string* error) {
  Display* display = XOpenDisplay(nullptr);
  if (!display) {
    if (error) {
      *error = "cannot open X display";
    }
    return std::nullopt;
  }

  // The X server exposes the active RMLVO configuration as the
  // _XKB_RULES_NAMES root window property: five NUL-separated strings
  // "rules\0model\0layout\0variant\0options\0".
  std::optional<std::string> result;
  Atom type = None;
  int format = 0;
  unsigned long items = 0;
  unsigned long bytes_after = 0;
  unsigned char* data = nullptr;
  Atom rules_atom = XInternAtom(display, "_XKB_RULES_NAMES", False);
  int status = XGetWindowProperty(display, RootWindow(display, 0), rules_atom,
                                  0, 1024, False, XA_STRING, &type, &format,
                                  &items, &bytes_after, &data);
  if (status == Success && type == XA_STRING && data) {
    std::vector<std::string> fields;
    const char* begin = reinterpret_cast<const char*>(data);
    const char* end = begin + items;
    for (const char* p = begin; p < end;) {
      fields.emplace_back(p);
      p += fields.back().size() + 1;
    }
    if (fields.size() >= 3) {
      result = fields[2];
    }
  }
  if (data) {
    XFree(data);
  }

  XCloseDisplay(display);
  if (!result && error) {
    *error = "cannot read _XKB_RULES_NAMES property";
  }
  return result;
}

}  // namespace langswitch
