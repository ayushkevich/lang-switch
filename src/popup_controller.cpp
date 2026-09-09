#include "langswitch/popup_controller.h"

namespace langswitch {

void PopupController::on_layout_changed(int layout_index, int64_t now_ms) {
  layout_ = layout_index;
  shown_at_ms_ = now_ms;
  visible_ = true;
}

std::optional<int> PopupController::visible_layout(int64_t now_ms) const {
  if (!visible_) {
    return std::nullopt;
  }
  if (now_ms - shown_at_ms_ >= durationMs_) {
    return std::nullopt;
  }
  return layout_;
}

int64_t PopupController::remaining_ms(int64_t now_ms) const {
  const auto left = durationMs_ - (now_ms - shown_at_ms_);
  return visible_ && left > 0 ? left : 0;
}

}  // namespace langswitch
