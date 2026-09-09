#include "langswitch/layout_names.h"

#include <gtest/gtest.h>

using namespace langswitch;

TEST(ParseLayoutList, SplitsCommaSeparatedCodes) {
  EXPECT_EQ(parse_layout_list("us,ru"), (std::vector<std::string>{"us", "ru"}));
  EXPECT_EQ(parse_layout_list("us,ru,de"),
            (std::vector<std::string>{"us", "ru", "de"}));
}

TEST(ParseLayoutList, TrimsWhitespaceAndLowercases) {
  EXPECT_EQ(parse_layout_list(" US , Ru "),
            (std::vector<std::string>{"us", "ru"}));
}

TEST(ParseLayoutList, SkipsEmptyEntries) {
  EXPECT_EQ(parse_layout_list("us,,ru,"),
            (std::vector<std::string>{"us", "ru"}));
  EXPECT_TRUE(parse_layout_list("").empty());
  EXPECT_TRUE(parse_layout_list("  ,  ").empty());
}

TEST(ParseLayoutList, KeepsVariants) {
  EXPECT_EQ(parse_layout_list("us(dvorak),ru(winkeys)"),
            (std::vector<std::string>{"us(dvorak)", "ru(winkeys)"}));
}

TEST(StripVariant, RemovesParenthesizedVariant) {
  EXPECT_EQ(strip_variant("us(dvorak)"), "us");
  EXPECT_EQ(strip_variant("ru(winkeys)"), "ru");
  EXPECT_EQ(strip_variant("de"), "de");
  EXPECT_EQ(strip_variant(" us (dvorak) "), "us");
}

TEST(LayoutDisplayName, MapsKnownCodes) {
  EXPECT_EQ(layout_display_name("us"), "English");
  EXPECT_EQ(layout_display_name("ru"), "Русский");
  EXPECT_EQ(layout_display_name("ua"), "Українська");
  EXPECT_EQ(layout_display_name("de"), "Deutsch");
}

TEST(LayoutDisplayName, ReturnsUnknownCodeUnchanged) {
  EXPECT_EQ(layout_display_name("zz"), "zz");
  EXPECT_EQ(layout_display_name(""), "");
}

TEST(PopupText, CombinesDisplayNameAndCode) {
  EXPECT_EQ(popup_text("ru"), "Русский (ru)");
  EXPECT_EQ(popup_text("us"), "English (us)");
}

TEST(PopupText, StripsVariantBeforeLookup) {
  EXPECT_EQ(popup_text("ru(winkeys)"), "Русский (ru)");
  EXPECT_EQ(popup_text("us(dvorak)"), "English (us)");
}

TEST(PopupText, UnknownLayoutShowsCodeOnly) {
  EXPECT_EQ(popup_text("zz"), "zz");
  EXPECT_EQ(popup_text("zz(qwerty)"), "zz");
}
