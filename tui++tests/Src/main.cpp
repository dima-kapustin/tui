// tui++tests: the automated unit tests plus the interactive font tools.
//
// Without arguments every unit test runs, then the program exits.
//
//   tui++tests font                       interactive glyph visual test (sixel)
//   tui++tests fontedit                   interactive 16x32 glyph editor (sixel)
//   tui++tests fontedit bench|scrollbench|sigtest
//
// The interactive MenuBar demo used to live here as the default mode (a
// "sixel" argument selected the pixel backend). It is now its own project,
// MenuBarDemo [text|sixel].

#include <cstdio>
#include <string_view>

void test_utf8();
void test_Char();
void test_EnumMask();
void test_KeyStroke();
void test_EventSource();
void test_CharIterator();
void test_Action();
void test_Color();
void test_Font();
void test_SixelEncoder();
void test_Menu();
void test_MenuKeyboard();
void test_MenuSubmenu();
void test_MenuAccelerators();
void test_FocusTraversal();
void test_TextBuffer();
void test_ScrollPane();
void test_TextArea_editing();
void test_TextScreen_scroll();
void test_TextScreen_horizontal_scroll();
void test_TextScreen_popup_over_scroll();
void test_TextScreen_scroll_keeps_fixed_ui_out_of_band();
void test_TextScreen_popup_close_erasure();
void test_TextScreen_combo_arrow_repaint();
void test_TextScreen_clipped_border_repaint();
void test_TextScreen_repaint_over_unknown_content();
void test_TextArea_caret();
void test_TextArea_selection();
void test_TextArea_find_all();
void test_InputTranslation();
void test_text_field();
void test_Widgets();
void test_Shadow();
void test_ContextMenu();

void run_font_visual_test();
void run_font_editor(bool bench = false, bool scrollbench = false, bool sigtest = false);

int main(int argc, char *argv[]) {
  // stderr/stdout are file-buffered when redirected; abort() would lose the
  // buffer, hiding which test failed. Unbuffered output keeps the diagnostics.
  setvbuf(stderr, nullptr, _IONBF, 0);
  setvbuf(stdout, nullptr, _IONBF, 0);

  test_utf8();
  test_Char();
  test_EnumMask();
  test_KeyStroke();
  test_EventSource();
  test_CharIterator();
  test_Action();
  test_Color();
  test_Font();
  test_SixelEncoder();
  test_Menu();
  test_MenuKeyboard();
  test_MenuSubmenu();
  test_MenuAccelerators();
  test_FocusTraversal();
  test_TextBuffer();
  test_ScrollPane();
  test_TextArea_editing();
  test_TextScreen_scroll();
  test_TextScreen_horizontal_scroll();
  test_TextScreen_popup_over_scroll();
  test_TextScreen_scroll_keeps_fixed_ui_out_of_band();
  test_TextScreen_popup_close_erasure();
  test_TextScreen_combo_arrow_repaint();
  test_TextScreen_clipped_border_repaint();
  test_TextScreen_repaint_over_unknown_content();
  test_TextArea_caret();
  test_TextArea_selection();
  test_TextArea_find_all();
  test_InputTranslation();
  test_text_field();
  test_Widgets();
  test_Shadow();
  test_ContextMenu();

  if (argc > 1) {
    auto arg = std::string_view(argv[1]);

    // The interactive font visual test: renders every glyph of the graphic
    // font in all styles and lets the user rate each letter.
    if (arg == "font") {
      run_font_visual_test();
      return 0;
    }

    // The interactive font editor: a 16x32 pixel grid per glyph, mouse-editable,
    // that dumps the corrected bitmaps as C rows for Font16x32.h. The optional
    // "bench" argument replaces the event loop with a full-repaint benchmark,
    // "scrollbench" feeds the wheel handler a synthetic event burst, and
    // "sigtest" raises SIGINT to verify the abnormal-exit terminal restore.
    if (arg == "fontedit") {
      run_font_editor(argc > 2 and std::string_view(argv[2]) == "bench", argc > 2 and std::string_view(argv[2]) == "scrollbench", argc > 2 and std::string_view(argv[2]) == "sigtest");
      return 0;
    }

    // Backend names that used to select the embedded MenuBar demo point at
    // its new home instead of failing silently.
    std::fprintf(stderr, "The interactive MenuBar demo is the separate project:\n"
                         "  MenuBarDemo [text|sixel]\n");
    return 0;
  }

  std::fprintf(stderr, "MenuBar demo: run `MenuBarDemo [text|sixel]`; font tools: run `tui++tests font` / `tui++tests fontedit`.\n");
  return 0;
}
