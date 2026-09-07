#pragma once

// Clipboard - an in-memory stand-in for Swing's system clipboard (the object
// java.awt.Toolkit.getSystemClipboard() hands out). Text components cut and
// copy into it and paste from it.
//
// The terminal has no OS clipboard contract, so the storage is process-local
// for now. A later port can keep this interface and back it with the real
// thing (Win32 clipboard, OSC 52, ...) without touching the components.

#include <string>

namespace tui {

class Clipboard {
public:
  static void set_text(std::string const &text) {
    holder() = text;
  }

  static std::string const &get_text() {
    return holder();
  }

  static bool has_text() {
    return not holder().empty();
  }

private:
  static std::string &holder() {
    static std::string text;
    return text;
  }
};

}
