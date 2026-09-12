#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace tui {

/**
 * The translation bundles: the texts the toolkit draws in the user's language.
 *
 * The look-and-feel looks the texts it draws -- a button's label, a menu
 * item's -- up in the bundles at the moment it measures and paints them (see
 * laf::displayed_text), so the translation is orthogonal to the components:
 * no component carries a translation API, and what a program sets and reads
 * back through get_text is always its own string. A widget whose text is data
 * rather than a phrase opts out through the ordinary client property
 * mechanism:
 *
 *   button->set_client_property(Messages::TRANSLATABLE_PROPERTY, false);
 *
 * A widget's text is its key. A button built as Button("Open") shows the
 * translation of "Open" the current locale carries -- in the current locale,
 * then in the language part of that locale, then in the default locale and
 * finally in the base bundle (the locale ""). A key no bundle carries shows as
 * itself, so a program that installs no bundle behaves exactly as the strings
 * it wrote, and a bundle may be as partial as its translator likes: whatever
 * it does not translate stays in the program's own language.
 *
 * The locale switches at run time and every live widget follows:
 *
 *   Messages::put("de", "Open", "Öffnen");
 *   Messages::set_locale("de");
 *
 * A program can load a translator's file instead (see load). Only the strings
 * the toolkit itself displays are looked up: the labels of buttons, toggles,
 * switches, menu items and menus. What a text field or a text area shows is
 * the program's content and is never touched.
 */
class Messages {
public:
  // The client property a widget sets to false to opt out of the lookup
  // (see Component::set_client_property):
  //
  //   button->set_client_property(Messages::TRANSLATABLE_PROPERTY, false);
  //
  // A client property, so a widget whose text is data rather than a phrase
  // needs no API of its own to say so.
  static constexpr char const *TRANSLATABLE_PROPERTY = "Translatable";

  // The locale whose bundles the texts of the widgets are looked up in:
  // "de", "de_AT" or "pt_BR" (the '-' of a BCP 47 tag is accepted too).
  // "" is the base bundle, which applies to every locale. Changing the locale
  // drops the layouts of the live windows and repaints them, so the widgets on
  // screen follow at once -- their texts and their sizes -- and an open popup
  // is placed again under the component it hangs off (see
  // PopupWindow::reanchor).
  static void set_locale(std::string_view locale);
  static std::string const& get_locale();

  // The locale a lookup falls back to when the current one has no entry
  // (Swing's default locale). "" by default, so the chain without any
  // configuration is: the current locale, its language, the base bundle.
  static void set_default_locale(std::string_view locale);
  static std::string const& get_default_locale();

  // Adds or replaces one translation of a locale.
  static void put(std::string_view locale, std::string_view key, std::string_view translation);

  // Adds the translations of a properties file for a locale: one "key=value"
  // per line ('=' and ':' both separate), '#' and '!' start a comment, blank
  // lines are skipped, keys and values are trimmed, the escapes "\n", "\t",
  // "\r", "\\", "\=", "\:" and "\#" stand for themselves, and the file is
  // read as UTF-8. A line without a separator is a key with an empty
  // translation. Returns false when the file cannot be read (the bundles are
  // left as they were); the translations of a file that is read replace the
  // locale's entries of the same keys.
  static bool load(std::string_view locale, std::string_view path);

  // The translation of `key` in the current locale and its fallbacks, or the
  // key itself when none of them carries it. The look-and-feel calls this for
  // the texts it draws (see laf::displayed_text); a program may call it for
  // its own strings as well.
  static std::string get(std::string_view key);

  // Whether a bundle of the chain carries `key` (a translation may equal the
  // key on purpose, so this is the question `get` cannot answer).
  static bool has(std::string_view key);

  // Drops every translation of a locale ("" clears the base bundle).
  static void clear(std::string_view locale);

  // How many translations a locale carries ("" is the base bundle).
  static std::size_t size(std::string_view locale);
};

}
