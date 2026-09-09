#include "langswitch/popup_controller.h"

#include <gtest/gtest.h>

using namespace langswitch;

TEST(XkbGroupToIndex, PassesThroughZeroBasedGroups) {
  EXPECT_EQ(xkb_group_to_index(0), 0);
  EXPECT_EQ(xkb_group_to_index(1), 1);
  EXPECT_EQ(xkb_group_to_index(3), 3);
}

TEST(XkbGroupToIndex, ClampsNegativeGroupsToZero) {
  EXPECT_EQ(xkb_group_to_index(-1), 0);
  EXPECT_EQ(xkb_group_to_index(-3), 0);
}

TEST(PopupController, HiddenInitially) {
  PopupController controller;
  EXPECT_FALSE(controller.visible_layout(0).has_value());
  EXPECT_FALSE(controller.visible_layout(10000).has_value());
  EXPECT_EQ(controller.remaining_ms(0), 0);
}

TEST(PopupController, VisibleRightAfterChange) {
  PopupController controller;
  controller.on_layout_changed(1, 1000);
  EXPECT_EQ(controller.visible_layout(1000), 1);
  EXPECT_EQ(controller.remaining_ms(1000), kPopupDurationMs);
}

TEST(PopupController, VisibleJustBeforeDeadline) {
  PopupController controller;
  controller.on_layout_changed(0, 1000);
  EXPECT_EQ(controller.visible_layout(1000 + kPopupDurationMs - 1), 0);
  EXPECT_EQ(controller.remaining_ms(1000 + kPopupDurationMs - 1), 1);
}

TEST(PopupController, HiddenExactlyAfterOneSecond) {
  PopupController controller;
  controller.on_layout_changed(0, 1000);
  EXPECT_FALSE(controller.visible_layout(1000 + kPopupDurationMs).has_value());
  EXPECT_FALSE(controller.visible_layout(1000 + kPopupDurationMs + 5000)
                   .has_value());
}

TEST(PopupController, RemainingIsZeroWhenHidden) {
  PopupController controller;
  controller.on_layout_changed(2, 0);
  EXPECT_EQ(controller.remaining_ms(kPopupDurationMs * 2), 0);
}

TEST(PopupController, ChangeWhileVisibleSwitchesLayoutAndResetsTimer) {
  PopupController controller;
  controller.on_layout_changed(0, 0);
  controller.on_layout_changed(1, 2000);
  EXPECT_EQ(controller.visible_layout(2000), 1);
  EXPECT_EQ(controller.remaining_ms(2000), kPopupDurationMs);
  EXPECT_EQ(controller.visible_layout(2000 + kPopupDurationMs - 1), 1);
  EXPECT_FALSE(
      controller.visible_layout(2000 + kPopupDurationMs).has_value());
}

TEST(PopupController, DefaultDurationIsOneSecond) {
  EXPECT_EQ(kPopupDurationMs, 1000);
}

TEST(PopupController, CustomDurationIsHonored) {
  PopupController controller(500);
  controller.on_layout_changed(3, 0);
  EXPECT_EQ(controller.visible_layout(499), 3);
  EXPECT_FALSE(controller.visible_layout(500).has_value());
}
