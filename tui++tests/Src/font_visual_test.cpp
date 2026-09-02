// Interactive visual test for the graphic (sixel) font.
//
// Run with:  tui++tests font
//
// Shows the current letter of the 95 printable ASCII glyphs, rendered at a
// large size in every style (plain, bold, italic, bold+italic), plus an
// overview strip of all letters in their rated color. The user inspects each
// letter and rates it:
//
//   arrows          previous / next letter
//   g               mark the current letter as good
//   b               mark the current letter as bad
//   u               unrate the current letter
//   q               quit and print the rating summary
//
// The frame is painted at 1px resolution by the sixel backend, so the glyphs
// are shown exactly as the renderer produces them.
#include <tui++/Font.h>
#include <tui++/Char.h>
#include <tui++/Graphics.h>
#include <tui++/Component.h>
#include <tui++/Frame.h>
#include <tui++/Screen.h>

#include <tui++/terminal/Terminal.h>

#include <cstdio>
#include <cstdlib>
#include <string>

using namespace tui;

class FontTestPanel: public Component {
public:
  enum class Rating {
    UNRATED,
    GOOD,
    BAD
  };

private:
  std::string letters;
  std::size_t index = 0;
  std::vector<Rating> ratings;

  int counts(Rating rating) const {
    auto n = 0;
    for (auto r : this->ratings) {
      if (r == rating) {
        ++n;
      }
    }
    return n;
  }

  char current_letter() const {
    return this->letters[this->index];
  }

  static const char* rating_name(Rating rating) {
    switch (rating) {
    case Rating::GOOD:
      return "GOOD";
    case Rating::BAD:
      return "BAD";
    default:
      return "unrated";
    }
  }

public:
  FontTestPanel() {
    for (auto c = 32; c < 127; ++c) {
      this->letters += char(c);
    }
    this->ratings.assign(this->letters.size(), Rating::UNRATED);
  }

  void prev() {
    this->index = (this->index + this->letters.size() - 1) % this->letters.size();
  }

  void next() {
    this->index = (this->index + 1) % this->letters.size();
  }

  void rate(Rating rating) {
    this->ratings[this->index] = rating;
  }

  void quit() {
    std::string good, bad;
    for (auto i = 0; i < int(this->letters.size()); ++i) {
      switch (this->ratings[i]) {
      case Rating::GOOD:
        good += this->letters[i];
        break;
      case Rating::BAD:
        bad += this->letters[i];
        break;
      default:
        break;
      }
    }
    std::printf("FONT TEST RESULT\n");
    std::printf("  good    (%d): %s\n", int(good.size()), good.c_str());
    std::printf("  bad     (%d): %s\n", int(bad.size()), bad.c_str());
    std::printf("  unrated (%d)\n", int(this->letters.size()) - int(good.size()) - int(bad.size()));
    std::fflush(stdout);
    std::exit(0);
  }

  void paint(Graphics &g) override {
    auto w = get_width();
    auto h = get_height();
    if (w <= 0 or h <= 0) {
      return;
    }

    auto background = Color { 10, 10, 14 };
    auto text = Color { 215, 215, 215 };
    auto dim = Color { 125, 125, 135 };
    auto good = Color { 60, 205, 90 };
    auto bad = Color { 235, 75, 75 };
    auto highlight = Color { 215, 180, 40 };
    auto on_highlight = Color { 10, 10, 14 };

    constexpr auto font_size = 16;
    constexpr auto line_height = 2 * font_size;

    g.set_foreground_color(background);
    g.set_background_color(background);
    g.fill_rect(0, 0, w, h);

    // ---- header -----------------------------------------------------------
    g.set_font(Font { "Monospaced", font_size, Font::PLAIN });
    g.set_foreground_color(text);

    char title[96];
    std::snprintf(title, sizeof title, "tui++ FONT TEST   '%c' U+%04X   %d/%d", current_letter(), unsigned(char32_t(current_letter())), int(this->index) + 1, int(this->letters.size()));
    g.draw_string(title, 16, 8);

    g.set_foreground_color(dim);
    g.draw_string("arrows: prev/next    g: good    b: bad    u: unrated    q: quit", 16, 8 + line_height);

    auto rating_color = this->ratings[this->index] == Rating::GOOD ? good : (this->ratings[this->index] == Rating::BAD ? bad : dim);
    g.set_foreground_color(dim);
    g.draw_string("rating: ", 16, 8 + 2 * line_height);
    g.set_foreground_color(rating_color);
    g.draw_string(rating_name(this->ratings[this->index]), 16 + 8 * font_size, 8 + 2 * line_height);
    char counts_buf[80];
    std::snprintf(counts_buf, sizeof counts_buf, "    good: %d   bad: %d   unrated: %d", counts(Rating::GOOD), counts(Rating::BAD), counts(Rating::UNRATED));
    g.set_foreground_color(dim);
    g.draw_string(counts_buf, 16 + 8 * font_size + font_size * int(std::strlen(rating_name(this->ratings[this->index]))), 8 + 2 * line_height);

    // ---- the letter in every style ----------------------------------------
    struct Variant {
      Font::Style style;
      const char *caption;
    };
    constexpr Variant variants[] = {
      { Font::PLAIN, "plain" },
      { Font::BOLD, "bold" },
      { Font::ITALIC, "italic" },
      { Font::BOLD | Font::ITALIC, "bold+italic" },
    };

    constexpr auto glyph_size = 32; // 32 x 64 px glyph
    auto grid_top = 56;
    // The strip below the previews must end above the last visible row (the
    // flush stops one cell short of it to avoid scrolling).
    auto footer_y = h - 96;
    auto grid_bottom = footer_y - 24;
    auto cell_w = (w - 32) / 2;
    auto cell_h = (grid_bottom - grid_top) / 2;
    auto caption_h = 18;

    for (auto i = 0; i < 4; ++i) {
      auto cx = 16 + (i % 2) * cell_w;
      auto cy = grid_top + (i / 2) * cell_h;

      g.set_font(Font { "Monospaced", font_size, Font::PLAIN });
      g.set_foreground_color(dim);
      auto caption = variants[i].caption;
      g.draw_string(caption, cx + (cell_w - int(std::strlen(caption)) * font_size) / 2, cy + cell_h - caption_h);

      g.set_font(Font { "Monospaced", glyph_size, variants[i].style });
      g.set_foreground_color(text);
      g.draw_char(Char(char32_t(current_letter())), cx + (cell_w - glyph_size) / 2, cy + (cell_h - caption_h - 2 * glyph_size) / 2);
    }

    // ---- overview strip of all letters ------------------------------------
    auto per_row = (w - 16) / font_size;
    if (per_row <= 0) {
      per_row = 1;
    }
    g.set_font(Font { "Monospaced", font_size, Font::PLAIN });
    for (auto i = 0; i < int(this->letters.size()); ++i) {
      auto fx = 8 + (i % per_row) * font_size;
      auto fy = footer_y + (i / per_row) * line_height;

      if (i == int(this->index)) {
        g.set_background_color(highlight);
        g.fill_rect(fx, fy, font_size, line_height);
        g.set_foreground_color(on_highlight);
      } else {
        switch (this->ratings[i]) {
        case Rating::GOOD:
          g.set_foreground_color(good);
          break;
        case Rating::BAD:
          g.set_foreground_color(bad);
          break;
        default:
          g.set_foreground_color(dim);
          break;
        }
      }
      g.draw_char(Char(char32_t(this->letters[i])), fx, fy);
      // draw_char fills the unset pixels of the glyph box with the background
      // color, so clear the highlight after the selected letter.
      g.set_background_color(std::nullopt);
    }
  }
};

// Runs the interactive font visual test on the graphic (sixel) backend.
// Lives at global scope like the other test entry points declared in main.cpp.
void run_font_visual_test() {
  terminal.set_title("tui++ font visual test");
  terminal.set_type("sixel");

  auto frame = make_component<Frame>();
  frame->set_size(screen.get_size());

  auto panel = make_component<FontTestPanel>();
  frame->add(panel);

  frame->add_listener([panel](KeyEvent &e) {
    auto handled = true;
    if (e.id == KeyEvent::KEY_TYPED) {
      switch (e.get_key_char().get_code()) {
      case 'g':
        panel->rate(FontTestPanel::Rating::GOOD);
        break;
      case 'b':
        panel->rate(FontTestPanel::Rating::BAD);
        break;
      case 'u':
        panel->rate(FontTestPanel::Rating::UNRATED);
        break;
      case 'q':
        panel->quit();
        return;
      default:
        handled = false;
        break;
      }
    } else if (e.id == KeyEvent::KEY_PRESSED) {
      switch (e.get_key_code()) {
      case KeyEvent::VK_LEFT:
      case KeyEvent::VK_UP:
        panel->prev();
        break;
      case KeyEvent::VK_RIGHT:
      case KeyEvent::VK_DOWN:
        panel->next();
        break;
      default:
        handled = false;
        break;
      }
    } else {
      handled = false;
    }

    if (handled) {
      e.consume();
      screen.refresh();
    }
  });

  frame->set_visible(true);

  terminal.run_event_loop();
}
