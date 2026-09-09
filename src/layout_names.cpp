#include "langswitch/layout_names.h"

#include <algorithm>
#include <cctype>
#include <unordered_map>

namespace langswitch {
namespace {

std::string trim(const std::string& s) {
  const auto begin = s.find_first_not_of(" \t\r\n");
  if (begin == std::string::npos) {
    return {};
  }
  const auto end = s.find_last_not_of(" \t\r\n");
  return s.substr(begin, end - begin + 1);
}

const std::unordered_map<std::string, std::string>& name_map() {
  static const std::unordered_map<std::string, std::string> kNames = {
      {"us", "English"},   {"ru", "Русский"},     {"ua", "Українська"},
      {"by", "Беларуская"}, {"de", "Deutsch"},    {"fr", "Français"},
      {"es", "Español"},   {"it", "Italiano"},    {"pt", "Português"},
      {"pl", "Polski"},    {"cs", "Čeština"},     {"sk", "Slovenčina"},
      {"bg", "Български"}, {"sr", "Српски"},      {"tr", "Türkçe"},
      {"ro", "Română"},    {"nl", "Nederlands"},  {"sv", "Svenska"},
      {"fi", "Suomi"},     {"no", "Norsk"},       {"da", "Dansk"},
      {"hu", "Magyar"},    {"el", "Ελληνικά"},    {"he", "עברית"},
      {"ar", "العربية"},   {"hi", "हिन्दी"},       {"jp", "日本語"},
      {"cz", "Čeština"},   {"hr", "Hrvatski"},    {"sl", "Slovenščina"},
      {"lt", "Lietuvių"},  {"lv", "Latviešu"},    {"et", "Eesti"},
  };
  return kNames;
}

}  // namespace

std::vector<std::string> parse_layout_list(const std::string& list) {
  std::vector<std::string> result;
  std::string::size_type pos = 0;
  while (pos <= list.size()) {
    const auto comma = list.find(',', pos);
    const auto end = comma == std::string::npos ? list.size() : comma;
    auto item = trim(list.substr(pos, end - pos));
    if (!item.empty()) {
      std::transform(item.begin(), item.end(), item.begin(),
                     [](unsigned char c) { return std::tolower(c); });
      result.push_back(std::move(item));
    }
    if (comma == std::string::npos) {
      break;
    }
    pos = comma + 1;
  }
  return result;
}

std::string strip_variant(const std::string& layout) {
  const auto paren = layout.find('(');
  auto result = paren == std::string::npos ? layout : layout.substr(0, paren);
  return trim(result);
}

std::string layout_display_name(const std::string& layout) {
  const auto& names = name_map();
  const auto it = names.find(layout);
  if (it != names.end()) {
    return it->second;
  }
  return layout;
}

std::string popup_text(const std::string& layout) {
  const auto base = strip_variant(layout);
  const auto display = layout_display_name(base);
  if (display == base) {
    return base;
  }
  return display + " (" + base + ")";
}

}  // namespace langswitch
