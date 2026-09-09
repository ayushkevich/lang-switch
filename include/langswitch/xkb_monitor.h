#pragma once

#include <functional>
#include <optional>
#include <string>
#include <thread>
#include <atomic>

#include <X11/Xlib.h>

namespace langswitch {

// Listens for XKB layout (group) changes on the X server.
// The callback is invoked from an internal worker thread.
class XkbMonitor {
public:
  using GroupCallback = std::function<void(int layout_index)>;

  XkbMonitor() = default;
  ~XkbMonitor();

  XkbMonitor(const XkbMonitor&) = delete;
  XkbMonitor& operator=(const XkbMonitor&) = delete;

  bool start(GroupCallback callback, std::string* error);
  void stop();

  // Reads the configured XKB layout list from the X server,
  // e.g. "us,ru". Returns std::nullopt on failure.
  static std::optional<std::string> query_layout_list(std::string* error);

private:
  void run();

  std::thread thread_;
  std::atomic<bool> running_{false};
  Display* display_ = nullptr;
  Window window_ = 0;
  Atom wake_atom_ = None;
  int xkb_event_type_ = 0;
  int last_group_ = -1;
  GroupCallback callback_;
};

}  // namespace langswitch
