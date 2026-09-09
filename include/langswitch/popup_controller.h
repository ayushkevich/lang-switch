#pragma once

#include <cstdint>
#include <optional>

namespace langswitch {

inline constexpr int64_t kPopupDurationMs = 1000;
inline constexpr int kInvalidLayout = -1;

// Converts the group value of an XkbStateNotify event (zero-based) to a
// layout index, clamping invalid values to zero.
inline int xkb_group_to_index(int group) { return group > 0 ? group : 0; }

// Pure state machine that decides when the popup is visible.
// All timestamps are monotonic milliseconds supplied by the caller.
class PopupController {
public:
  explicit PopupController(int64_t durationMs = kPopupDurationMs)
      : durationMs_(durationMs) {}

  // Must be called whenever the active layout changes.
  // Shows the popup and (re)starts the visibility timer.
  void on_layout_changed(int layout_index, int64_t now_ms);

  // Layout index currently shown, or std::nullopt when the popup is hidden.
  std::optional<int> visible_layout(int64_t now_ms) const;

  // Milliseconds left before auto-hide; 0 when hidden.
  int64_t remaining_ms(int64_t now_ms) const;

private:
  int64_t durationMs_;
  int layout_ = kInvalidLayout;
  int64_t shown_at_ms_ = 0;
  bool visible_ = false;
};

}  // namespace langswitch
