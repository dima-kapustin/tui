// Exercises keyboard focus traversal on the text screen the way a Swing
// application uses it: Tab/Shift+Tab move the focus through the focusable
// components of the window in container order, the menu bar is not a Tab stop
// (it is reached with F10 / Alt+mnemonic, as on Windows), and while the menu
// system holds the keyboard a Tab or Shift+Tab dismisses the menus and moves
// the focus into the content -- from the menu bar to the text component.
//
// The whole project (and therefore this test) is usually built with NDEBUG
// (Release), which would compile the assert()s out and make the test pass
// vacuously; CHECK below aborts regardless, so the test always verifies.
#include <tui++/Frame.h>
#include <tui++/KeyboardFocusManager.h>
#include <tui++/Menu.h>
#include <tui++/MenuBar.h>
#include <tui++/MenuItem.h>
#include <tui++/Screen.h>
#include <tui++/TextArea.h>

#include <tui++/event/Event.h>
#include <tui++/event/InputEvent.h>
#include <tui++/event/KeyEvent.h>
#include <tui++/terminal/Terminal.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>

using namespace tui;

#define CHECK(cond)                                                                                                    \
  do {                                                                                                                 \
    if (not(cond)) {                                                                                                   \
      std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);                                            \
      std::abort();                                                                                                    \
    }                                                                                                                  \
  } while (0)

// A minimal focusable leaf for the traversal cycle: the toolkit has no plain
// focusable control without its own machinery (Button is a stub), and the
// text area deliberately keeps Tab for the editor. This one only accepts
// focus; it paints nothing and consumes nothing.
namespace tui {
class Traversable: public Component {
protected:
  Traversable() {
  }

  template<typename T, typename ... Args>
  requires (is_component_v<T> )
  friend auto make_component(Args&&...);

  std::shared_ptr<laf::ComponentUI> create_ui() override {
    return {};
  }
};
}

using namespace tui;

static void drain() {
  while (screen.get_event_queue().pop(std::chrono::milliseconds::zero())) {
  }
}

static std::shared_ptr<Event> dispatch_key(std::shared_ptr<Frame> const &frame, KeyEvent::Type type, KeyEvent::KeyCode code, InputEvent::Modifiers modifiers = InputEvent::NO_MODIFIERS) {
  drain();
  screen.post<KeyEvent>(frame, type, code, modifiers);
  auto event = screen.get_event_queue().pop();
  frame->dispatch_event(*event);
  drain();
  return event;
}

void test_FocusTraversal() {
  terminal.set_type("text");

  auto frame = make_component<Frame>();
  frame->set_size({ 100, 30 });

  // The menu bar must never become a Tab stop (Swing reaches it with F10 and
  // mnemonics, not with focus traversal), so it cannot stand between the
  // text area and the buttons in the traversal cycle.
  auto menu_bar = make_component<MenuBar>();
  auto file_menu = make_component<Menu>("File");
  file_menu->set_mnemonic('F');
  file_menu->add(make_component<MenuItem>("Dump"));
  menu_bar->add(file_menu);
  frame->set_menu_bar(menu_bar);

  // The window's content: the text area (first) and two focusable leaves.
  // The area keeps Tab for the editor (it inserts a tab character), but the
  // focus still enters it through traversal -- it is the cycle's first
  // component.
  auto content = frame->get_content_pane();
  auto area = make_component<TextArea>();
  area->set_name("editor");
  auto one = make_component<Traversable>();
  one->set_name("one");
  auto two = make_component<Traversable>();
  two->set_name("two");
  content->add(area);
  content->add(one);
  content->add(two);

  frame->set_visible(true);
  drain();

  auto owner = [] {
    return KeyboardFocusManager::single->get_focus_owner();
  };

  // The demo hands the initial focus to the text area.
  area->request_input_focus();
  CHECK(owner() == area);

  // Tab in the text area belongs to the editor: the focus stays and the key
  // is consumed by the area's own handler (which inserts a tab character),
  // not by focus traversal.
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_TAB)->consumed);
  CHECK(owner() == area);

  // From a button, Tab walks the cycle in container order -- area, One,
  // Two -- skipping the menu bar, and wraps around at the ends. (The area
  // keeps Tab for the editor, so the backward checks start from a component
  // whose traversal keys are on.)
  one->request_focus(FocusEvent::Cause::ACTIVATION);
  CHECK(owner() == one);
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_TAB)->consumed);
  CHECK(owner() == two);
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_TAB)->consumed);
  CHECK(owner() == area); // wrapped around past the menu bar to the first
  two->request_focus(FocusEvent::Cause::ACTIVATION);
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_BACK_TAB)->consumed);
  CHECK(owner() == one);
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_BACK_TAB)->consumed);
  CHECK(owner() == area); // ... and backwards to the first

  // F10 arms the bar; Tab then leaves the menus and moves the focus to the
  // content (first component -- the text area, as when the keyboard enters a
  // Windows application's content after the menu bar). The armed highlight
  // goes, and the keys belong to the editor again.
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_F10)->consumed);
  CHECK(file_menu->is_armed());
  one->request_focus(FocusEvent::Cause::ACTIVATION);
  CHECK(owner() == one);
  auto tab = dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_TAB);
  CHECK(tab->consumed);
  CHECK(not file_menu->is_armed());
  CHECK(owner() == area);

  // Typing now reaches the editor again (the typed character is consumed by
  // the area's document, the proof that the menu system let go of the keys).
  auto typed_char = [frame](Char c, InputEvent::Modifiers modifiers) {
    drain();
    screen.post<KeyEvent>(frame, c, modifiers);
    auto event = screen.get_event_queue().pop();
    frame->dispatch_event(*event);
    drain();
    return event;
  };
  auto typed = typed_char(Char { 'x' }, InputEvent::NO_MODIFIERS);
  CHECK(typed->consumed);
  CHECK(owner() == area);

  // Shift+Tab from the armed bar (no popup) does the same backwards: the
  // focus moves to the cycle's last component.
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_F10)->consumed);
  CHECK(file_menu->is_armed());
  auto back_tab = dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_BACK_TAB);
  CHECK(back_tab->consumed);
  CHECK(not file_menu->is_armed());
  CHECK(owner() == two);

  // With a popup open the same keys close the popup first and then move the
  // focus: Alt+F opens File, Tab hands the keyboard to the editor.
  CHECK(typed_char(Char { 'f' }, InputEvent::ALT_DOWN)->consumed);
  CHECK(file_menu->is_popup_menu_visible());
  auto tab_exit = dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_TAB);
  CHECK(tab_exit->consumed);
  CHECK(not file_menu->is_popup_menu_visible());
  CHECK(not file_menu->is_armed());
  CHECK(owner() == area);

  // Take the frame off the screen, as the screen tests that follow run on an
  // empty screen.
  frame->set_visible(false);
  drain();

  std::printf("PASS focus traversal (Tab cycle skips the menu bar; menus hand the keyboard back)\n");
}
