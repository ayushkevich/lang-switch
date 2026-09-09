#pragma once

#include <string>
#include <vector>

namespace langswitch {

// Parses the XKB layout list atom, e.g. "us,ru,de" -> {"us", "ru", "de"}.
// Empty entries and surrounding whitespace are ignored.
std::vector<std::string> parse_layout_list(const std::string& list);

// Strips the variant part: "us(dvorak)" -> "us", "ru(winkeys)" -> "ru".
std::string strip_variant(const std::string& layout);

// Human-readable name for a layout code ("ru" -> "Русский").
// Unknown codes are returned unchanged.
std::string layout_display_name(const std::string& layout);

// Text shown in the popup, e.g. "Русский (ru)".
// For unknown layouts returns just the code.
std::string popup_text(const std::string& layout);

}  // namespace langswitch
