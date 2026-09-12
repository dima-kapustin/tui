#include <tui++/Messages.h>

#include <tui++/Screen.h>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <optional>
#include <unordered_map>
#include <vector>

namespace tui {

namespace {

// locale -> key -> translation. The base bundle lives under the empty locale.
using Bundle = std::unordered_map<std::string, std::string>;
std::unordered_map<std::string, Bundle> bundles;

std::string current_locale;
std::string default_locale;

// "de-at", "DE_AT " -> "de_AT": the language lower-case, the rest upper-case,
// the two joined with '_', so a lookup does not depend on how the program
// spells the locale.
std::string normalize_locale(std::string_view locale) {
  auto normalized = std::string { };
  auto part = 0;
  auto start = std::size_t { 0 };
  while (start <= locale.size()) {
    auto end = locale.find_first_of("_-", start);
    if (end == std::string_view::npos) {
      end = locale.size();
    }
    auto piece = std::string(locale.substr(start, end - start));
    while (not piece.empty() and std::isspace(static_cast<unsigned char>(piece.front()))) {
      piece.erase(piece.begin());
    }
    while (not piece.empty() and std::isspace(static_cast<unsigned char>(piece.back()))) {
      piece.pop_back();
    }
    for (auto &&c : piece) {
      c = char(part == 0 ? std::tolower(static_cast<unsigned char>(c)) : std::toupper(static_cast<unsigned char>(c)));
    }
    if (not piece.empty()) {
      if (not normalized.empty()) {
        normalized += '_';
      }
      normalized += piece;
      ++part;
    }
    if (end == locale.size()) {
      break;
    }
    start = end + 1;
  }
  return normalized;
}

// The language part of a locale: "de_AT" -> "de" ("" stays "").
std::string language_of(std::string const &locale) {
  if (auto pos = locale.find('_'); pos != std::string::npos) {
    return locale.substr(0, pos);
  }
  return locale;
}

// The locales a lookup tries, most specific first: the current locale, its
// language, the default locale, its language, and the base bundle.
std::vector<std::string> lookup_chain() {
  auto chain = std::vector<std::string> { };
  auto add = [&chain](std::string const &locale) {
    if (not locale.empty() and std::find(chain.begin(), chain.end(), locale) == chain.end()) {
      chain.emplace_back(locale);
    }
  };
  add(current_locale);
  add(language_of(current_locale));
  add(default_locale);
  add(language_of(default_locale));
  chain.emplace_back(""); // the base bundle always applies
  return chain;
}

// The translation of `key` in the chain, or nothing.
std::optional<std::string> find_translation(std::string_view key) {
  for (auto &&locale : lookup_chain()) {
    auto bundle = bundles.find(locale);
    if (bundle == bundles.end()) {
      continue;
    }
    if (auto entry = bundle->second.find(std::string(key)); entry != bundle->second.end()) {
      return entry->second;
    }
  }
  return std::nullopt;
}

// A translation change can change the size of every text on the screen. The
// look-and-feel looks the texts up when it measures and paints them (see
// Messages), so dropping the layouts of the live windows and repainting them
// is all it takes; a screen with nothing on it is left alone.
void refresh_widgets() {
  if (screen.has_windows()) {
    screen.revalidate_windows();
    screen.refresh();
  }
}

// The value of an escape sequence, or the character itself for an escape the
// format does not name.
void append_escaped(std::string &to, char c) {
  switch (c) {
  case 'n':
    to += '\n';
    break;
  case 't':
    to += '\t';
    break;
  case 'r':
    to += '\r';
    break;
  default:
    to += c;
    break;
  }
}

std::string unescape(std::string_view value) {
  auto result = std::string { };
  result.reserve(value.size());
  for (auto i = std::size_t { 0 }; i < value.size(); ++i) {
    if (value[i] == '\\' and i + 1 < value.size()) {
      append_escaped(result, value[++i]);
    } else {
      result += value[i];
    }
  }
  return result;
}

std::string trim(std::string_view text) {
  auto start = std::size_t { 0 }, end = text.size();
  while (start < end and std::isspace(static_cast<unsigned char>(text[start]))) {
    ++start;
  }
  while (end > start and std::isspace(static_cast<unsigned char>(text[end - 1]))) {
    --end;
  }
  return std::string(text.substr(start, end - start));
}

// The separator of a properties line: the first unescaped '=' or ':'.
std::size_t find_separator(std::string_view line) {
  for (auto i = std::size_t { 0 }; i < line.size(); ++i) {
    if (line[i] == '\\') {
      ++i;
    } else if (line[i] == '=' or line[i] == ':') {
      return i;
    }
  }
  return std::string_view::npos;
}

}

void Messages::set_locale(std::string_view locale) {
  auto normalized = normalize_locale(locale);
  if (normalized == current_locale) {
    return;
  }
  current_locale = std::move(normalized);
  refresh_widgets();
}

std::string const& Messages::get_locale() {
  return current_locale;
}

void Messages::set_default_locale(std::string_view locale) {
  auto normalized = normalize_locale(locale);
  if (normalized == default_locale) {
    return;
  }
  default_locale = std::move(normalized);
  refresh_widgets();
}

std::string const& Messages::get_default_locale() {
  return default_locale;
}

void Messages::put(std::string_view locale, std::string_view key, std::string_view translation) {
  bundles[normalize_locale(locale)][std::string(key)] = std::string(translation);
  refresh_widgets();
}

bool Messages::load(std::string_view locale, std::string_view path) {
  auto file = std::ifstream { std::string(path) };
  if (not file) {
    return false;
  }

  auto &bundle = bundles[normalize_locale(locale)];
  auto line = std::string { };
  while (std::getline(file, line)) {
    if (not line.empty() and line.back() == '\r') {
      line.pop_back(); // a file written on Windows
    }
    auto stripped = trim(line);
    if (stripped.empty() or stripped.front() == '#' or stripped.front() == '!') {
      continue;
    }

    auto separator = find_separator(stripped);
    auto key = separator == std::string::npos ? stripped : stripped.substr(0, separator);
    auto value = separator == std::string::npos ? std::string { } : stripped.substr(separator + 1);
    bundle[unescape(trim(key))] = unescape(trim(value));
  }

  refresh_widgets();
  return true;
}

std::string Messages::get(std::string_view key) {
  if (auto translation = find_translation(key)) {
    return *translation;
  }
  return std::string(key);
}

bool Messages::has(std::string_view key) {
  return find_translation(key).has_value();
}

void Messages::clear(std::string_view locale) {
  bundles.erase(normalize_locale(locale));
  refresh_widgets();
}

std::size_t Messages::size(std::string_view locale) {
  if (auto bundle = bundles.find(normalize_locale(locale)); bundle != bundles.end()) {
    return bundle->second.size();
  }
  return 0;
}

}
