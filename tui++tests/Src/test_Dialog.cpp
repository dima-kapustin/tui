// Exercises the dialogs: the JDialog surface (packing, sizing, centering,
// closing), the modality of java.awt.Dialog.ModalityType -- a modal dialog
// holds the mouse and the keyboard of the windows it stands over, refuses
// them the focus, and gives them back when it closes -- the exception Java's
// modality makes for the dialog's own windows (a context menu it carries
// still opens), and the nested event pump a modal dialog's set_visible(true)
// runs, so the call returns only after the dialog is dismissed.
//
// The whole project (and therefore this test) is usually built with NDEBUG
// (Release), which would compile the assert()s out and make the test pass
// vacuously; CHECK below aborts regardless, so the test always verifies.
#include <tui++/Button.h>
#include <tui++/Dialog.h>
#include <tui++/Frame.h>
#include <tui++/KeyboardFocusManager.h>
#include <tui++/MenuItem.h>
#include <tui++/Panel.h>
#include <tui++/PopupMenu.h>
#include <tui++/Screen.h>
#include <tui++/TextField.h>
#include <tui++/Timer.h>

#include <tui++/BorderLayout.h>
#include <tui++/event/Event.h>
#include <tui++/event/InvocationEvent.h>
#include <tui++/event/KeyEvent.h>
#include <tui++/event/MouseEvent.h>
#include <tui++/terminal/Terminal.h>
#include <tui++/terminal/text/TextScreen.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

using namespace tui;

#define CHECK(cond)                                                                                                    \
  do {                                                                                                                 \
    if (not(cond)) {                                                                                                   \
      std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);                                            \
      std::abort();                                                                                                    \
    }                                                                                                                  \
  } while (0)

// Dispatches the queued events the way the running event loop does (the focus
// events a show/hide posts, the repaint invocations), so a test sees the state
// the application would.
static void pump() {
  while (auto event = screen.get_event_queue().pop(std::chrono::milliseconds::zero())) {
    if (event->id == InvocationEvent::INVOCATION) {
      static_cast<InvocationEvent&>(*event).dispatch();
    } else if (auto component = std::dynamic_pointer_cast<Component>(event->source)) {
      component->dispatch_event(*event);
    }
  }
}

static void drain() {
  while (screen.get_event_queue().pop(std::chrono::milliseconds::zero())) {
  }
}

// Posts a key event to `window` and dispatches it, returning it.
static std::shared_ptr<Event> dispatch_key(std::shared_ptr<Window> const &window, KeyEvent::Type type, KeyEvent::KeyCode code, InputEvent::Modifiers modifiers = InputEvent::NO_MODIFIERS) {
  drain();
  screen.post<KeyEvent>(window, type, code, modifiers);
  auto event = screen.get_event_queue().pop();
  window->dispatch_event(*event);
  drain();
  return event;
}

// Presses and releases the left button at (x, y) window-local.
static void click(std::shared_ptr<Window> const &window, int x, int y) {
  drain();
  screen.post<MousePressEvent>(window, MousePressEvent::MOUSE_PRESSED, MousePressEvent::LEFT_BUTTON, InputEvent::LEFT_BUTTON_DOWN, x, y, false);
  window->dispatch_event(*screen.get_event_queue().pop());
  screen.post<MousePressEvent>(window, MousePressEvent::MOUSE_RELEASED, MousePressEvent::LEFT_BUTTON, InputEvent::NO_MODIFIERS, x, y, false);
  window->dispatch_event(*screen.get_event_queue().pop());
  drain();
}

// A window covering the whole screen: the frame every test starts from.
static std::shared_ptr<Frame> make_frame() {
  auto frame = make_component<Frame>();
  frame->set_size({ 80, 24 });
  frame->set_visible(true);
  pump();
  return frame;
}

// The dialog's surface: it packs itself to its content on the first show --
// border included, so the content lands inside the box -- it can be centered
// over its owner, and closing it takes it off the screen.
static void test_dialog_show_and_pack() {
  terminal.set_type("text");
  drain();
  auto frame = make_frame();

  auto dialog = make_component<Dialog>(frame, "Modeless", Dialog::ModalityType::MODELESS);
  CHECK(not dialog->is_modal());
  CHECK(dialog->get_title() == "Modeless");
  CHECK(dialog->get_owner() == frame);
  CHECK(dialog->get_ui() != nullptr);
  CHECK(dialog->get_root_pane()->get_window_decoration_style() == RootPane::PLAIN_DIALOG);
  // The theme's dialog chrome: an opaque face (a window is opaque when it has
  // a background color) and the dialog shadow.
  CHECK(dialog->get_background_color().has_value());
  CHECK(dialog->get_shadow().has_value());

  auto content = make_component<Panel>();
  content->set_layout(std::make_shared<BorderLayout>());
  content->set_preferred_size(Dimension { 24, 5 });
  auto button = make_component<Button>("OK");
  content->add(button, BorderLayout::CENTER);
  dialog->add(content);

  CHECK(dialog->get_size().empty());
  dialog->set_visible(true);
  pump();

  // A dialog with no size of its own packs to its content: the panel's
  // preferred size plus the dialog border on every side, and the content is
  // laid out inside that border.
  CHECK(dialog->is_showing());
  auto border_insets = dialog->get_root_pane()->get_insets();
  CHECK(border_insets.left > 0 and border_insets.top > 0);
  CHECK(dialog->get_width() == 24 + border_insets.left + border_insets.right);
  CHECK(dialog->get_height() == 5 + border_insets.top + border_insets.bottom);
  CHECK(screen.get_window_at(dialog->get_location_on_screen()) == dialog);
  CHECK(content->get_width() == 24 and content->get_height() == 5);

  // The keyboard belongs to the dialog that opened: it is the focused window
  // and its focus owner is inside it, so its buttons answer Enter.
  CHECK(dialog->is_focused());
  CHECK(dialog->get_focus_owner() != nullptr);
  CHECK(dialog->get_focus_owner()->get_containing_window() == dialog);

  // Centering over the owner puts the dialog in the middle of it.
  dialog->set_location_relative_to(frame);
  CHECK(dialog->get_x() == (frame->get_width() - dialog->get_width()) / 2);
  CHECK(dialog->get_y() == (frame->get_height() - dialog->get_height()) / 2);

  // dispose() takes the dialog down; the frame keeps the screen and gets the
  // focus back.
  dialog->dispose();
  pump();
  CHECK(not dialog->is_showing());
  CHECK(screen.has_windows());
  CHECK(screen.get_window_at(0, 0) == frame);
  CHECK(frame->is_focused());

  frame->set_visible(false);
  drain();
}

// A modeless dialog blocks nothing: the frame still receives its mouse and
// keys, and the keyboard can move between the two windows.
static void test_modeless_dialog() {
  terminal.set_type("text");
  drain();
  auto frame = make_frame();

  auto field = make_component<TextField>("frame field", 20);
  frame->add(field);
  pump();
  field->request_input_focus();
  pump();
  CHECK(field->is_focus_owner());

  auto dialog = make_component<Dialog>(frame, "Modeless", Dialog::ModalityType::MODELESS);
  auto content = make_component<Panel>();
  content->set_preferred_size(Dimension { 10, 3 });
  dialog->add(content);
  dialog->set_visible(true);
  pump();
  CHECK(dialog->is_showing());
  CHECK(not frame->is_modal_blocked());
  CHECK(frame->get_modal_blocker() == nullptr);

  // The frame's own listeners only see the events that land on the frame
  // itself; the mouse events of a click land on the component under the
  // pointer, so the probe listens on the field that fills the frame's content.
  auto presses = 0;
  field->add_listener([&presses](MousePressEvent &) {
    ++presses;
  });
  click(frame, frame->get_width() / 2, frame->get_height() - 2);
  CHECK(presses == 2); // pressed and released

  // With no blocker in the way, the focus follows the click: the frame's
  // field takes it back from the dialog.
  pump();
  CHECK(field->is_focus_owner());
  CHECK(frame->is_focused());

  dialog->set_visible(false);
  pump();
  CHECK(not dialog->is_showing());

  frame->set_visible(false);
  drain();
}

// An application-modal dialog holds the frame's mouse and keyboard, refuses it
// the focus, and releases everything when it closes. The dialog is modal, so
// set_visible(true) does not return until it is hidden: the observations run
// from a timer inside the nested pump, which then hides the dialog.
static void test_application_modal_dialog() {
  terminal.set_type("text");
  drain();
  auto frame = make_frame();

  auto field = make_component<TextField>("frame field", 20);
  frame->add(field);
  pump();
  field->request_input_focus();
  pump();
  CHECK(field->is_focus_owner());

  auto modal = make_component<Dialog>(frame, "Modal", Dialog::ModalityType::APPLICATION_MODAL);
  CHECK(modal->is_modal());
  auto content = make_component<Panel>();
  content->set_preferred_size(Dimension { 20, 4 });
  modal->add(content);
  modal->set_location_relative_to(frame);

  // The probe listens on the field that fills the frame's content: a press
  // that reaches the frame lands on the component under the pointer.
  auto presses = 0;
  field->add_listener([&presses](MousePressEvent &) {
    ++presses;
  });

  auto probe = std::shared_ptr<Timer> { };
  probe = std::make_shared<Timer>(std::chrono::milliseconds { 1 }, [&, modal] {
    CHECK(modal->is_showing());
    CHECK(frame->is_modal_blocked());
    CHECK(frame->get_modal_blocker() == modal);
    CHECK(not modal->is_modal_blocked());
    CHECK(modal->is_focused());

    // A press on the frame is swallowed whole: the component under the
    // pointer does not see any of it.
    click(frame, frame->get_width() / 2, frame->get_height() - 2);
    CHECK(presses == 0);

    // A key aimed at the frame is swallowed too, even though the running
    // terminal posts keys to the focused window (the modal dialog).
    CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_BACK_SPACE)->consumed);

    // The frame cannot take the focus back while the modal dialog is up.
    field->request_input_focus();
    pump();
    CHECK(modal->is_focused());
    CHECK(not field->is_focus_owner());

    // The dialog's own context menu opens (the modality excludes the modal
    // window's children) and its rows run.
    auto menu = make_component<PopupMenu>();
    auto item = menu->add("Dialog command");
    auto commands = 0;
    item->add_listener([&commands](ActionEvent &) {
      ++commands;
    });
    content->set_component_popup_menu(menu);
    auto at = content->get_location_on_screen();
    auto local = Point { at.x - modal->get_x(), at.y - modal->get_y() };
    screen.post<MousePressEvent>(modal, MousePressEvent::MOUSE_PRESSED, MousePressEvent::RIGHT_BUTTON, InputEvent::RIGHT_BUTTON_DOWN, local.x + 1, local.y + 1, true);
    modal->dispatch_event(*screen.get_event_queue().pop());
    drain();
    CHECK(menu->is_popup_showing());
    CHECK(menu->get_invoker() == content);
    CHECK(not menu->get_containing_window()->is_modal_blocked());
    dispatch_key(modal, KeyEvent::KEY_PRESSED, KeyEvent::VK_DOWN);
    dispatch_key(modal, KeyEvent::KEY_PRESSED, KeyEvent::VK_ENTER);
    CHECK(commands == 1);
    CHECK(not menu->is_popup_showing());

    // End the nested pump: show() returns to the caller.
    modal->set_visible(false);
    probe->stop();
  });
  probe->start();
  modal->set_visible(true);
  pump();

  CHECK(not modal->is_showing());
  CHECK(not frame->is_modal_blocked());
  CHECK(presses == 0);
  CHECK(frame->is_focused());

  // A modeless dialog that turns modal mid-life (Swing's setModal) blocks from
  // then on, and stops blocking when it is turned back.
  auto toggled = make_component<Dialog>(frame, "Toggled", Dialog::ModalityType::MODELESS);
  auto toggled_content = make_component<Panel>();
  toggled_content->set_preferred_size(Dimension { 12, 3 });
  toggled->add(toggled_content);
  toggled->set_visible(true);
  pump();
  CHECK(not frame->is_modal_blocked());

  toggled->set_modal(true);
  CHECK(toggled->get_modality_type() == Dialog::ModalityType::APPLICATION_MODAL);
  CHECK(frame->is_modal_blocked());
  CHECK(frame->get_modal_blocker() == toggled);
  toggled->set_modal(false);
  CHECK(toggled->get_modality_type() == Dialog::ModalityType::MODELESS);
  CHECK(not frame->is_modal_blocked());

  toggled->set_visible(false);
  frame->set_visible(false);
  drain();
}

// Document modality blocks the dialog's own document (the frame it belongs to
// and its windows) while other frames keep working.
static void test_document_modality() {
  terminal.set_type("text");
  drain();
  auto frame = make_frame();

  auto other_frame = make_component<Frame>();
  other_frame->set_size({ 40, 10 });
  other_frame->set_visible(true);
  pump();

  auto dialog = make_component<Dialog>(frame, "Document", Dialog::ModalityType::DOCUMENT_MODAL);
  auto content = make_component<Panel>();
  content->set_preferred_size(Dimension { 12, 3 });
  dialog->add(content);

  auto probe = std::shared_ptr<Timer> { };
  probe = std::make_shared<Timer>(std::chrono::milliseconds { 1 }, [&, dialog] {
    CHECK(dialog->is_showing());
    CHECK(frame->is_modal_blocked());
    CHECK(frame->get_modal_blocker() == dialog);
    CHECK(not other_frame->is_modal_blocked());

    dialog->set_visible(false);
    probe->stop();
  });
  probe->start();
  dialog->set_visible(true);
  pump();

  CHECK(not frame->is_modal_blocked());
  CHECK(not other_frame->is_modal_blocked());

  // An application-modal dialog blocks the other frame instead.
  auto application = make_component<Dialog>(frame, "Application", Dialog::ModalityType::APPLICATION_MODAL);
  auto application_content = make_component<Panel>();
  application_content->set_preferred_size(Dimension { 12, 3 });
  application->add(application_content);

  auto application_probe = std::shared_ptr<Timer> { };
  application_probe = std::make_shared<Timer>(std::chrono::milliseconds { 1 }, [&, application] {
    CHECK(frame->is_modal_blocked());
    CHECK(other_frame->is_modal_blocked());
    CHECK(other_frame->get_modal_blocker() == application);

    application->set_visible(false);
    application_probe->stop();
  });
  application_probe->start();
  application->set_visible(true);
  pump();

  CHECK(not frame->is_modal_blocked());
  CHECK(not other_frame->is_modal_blocked());

  // A document-modal dialog without an owner has no document to limit itself
  // to: it holds every window, the way Java resolves the same case.
  auto ownerless = make_component<Dialog>(std::shared_ptr<Window> { }, "Ownerless", Dialog::ModalityType::DOCUMENT_MODAL);
  auto ownerless_content = make_component<Panel>();
  ownerless_content->set_preferred_size(Dimension { 12, 3 });
  ownerless->add(ownerless_content);

  auto ownerless_probe = std::shared_ptr<Timer> { };
  ownerless_probe = std::make_shared<Timer>(std::chrono::milliseconds { 1 }, [&, ownerless] {
    CHECK(frame->is_modal_blocked());
    CHECK(other_frame->is_modal_blocked());
    CHECK(other_frame->get_modal_blocker() == ownerless);

    ownerless->set_visible(false);
    ownerless_probe->stop();
  });
  ownerless_probe->start();
  ownerless->set_visible(true);
  pump();

  CHECK(not frame->is_modal_blocked());
  CHECK(not other_frame->is_modal_blocked());

  other_frame->set_visible(false);
  frame->set_visible(false);
  drain();
}

// A modal dialog's show() blocks the caller until the dialog is hidden and
// the nested pump keeps dispatching the events that arrive while it is up:
// a keystroke aimed at the dialog is handled, and a timer that fires inside
// the pump ends the dialog (the dispatch thread never stops pumping).
static void test_modal_event_pump() {
  terminal.set_type("text");
  drain();
  auto frame = make_frame();

  auto dialog = make_component<Dialog>(frame, "Blocking", Dialog::ModalityType::APPLICATION_MODAL);
  auto content = make_component<Panel>();
  content->set_preferred_size(Dimension { 16, 3 });
  dialog->add(content);

  auto keys = std::vector<KeyEvent::KeyCode> { };
  dialog->add_listener([&keys](KeyEvent &e) {
    if (e.id == KeyEvent::KEY_PRESSED) {
      keys.push_back(e.get_key_code());
    }
  });

  // The key is queued before the dialog opens; the nested pump dispatches it
  // while the dialog is up (the outer test drives the events by hand, so the
  // pump is the only thing running).
  screen.post<KeyEvent>(dialog, KeyEvent::KEY_PRESSED, KeyEvent::VK_SPACE, InputEvent::NO_MODIFIERS);

  // A timer inside the pump closes the dialog after one turn.
  auto started = std::chrono::steady_clock::now();
  auto closer = std::shared_ptr<Timer> { };
  closer = std::make_shared<Timer>(std::chrono::milliseconds { 1 }, [dialog, &closer] {
    dialog->set_visible(false);
    closer->stop();
  });
  closer->start();

  dialog->set_visible(true); // blocks until the timer hides the dialog
  auto elapsed = std::chrono::steady_clock::now() - started;

  CHECK(not dialog->is_showing());
  CHECK(elapsed >= std::chrono::milliseconds { 1 });
  CHECK(keys.size() == 1 and keys.front() == KeyEvent::VK_SPACE && "the nested pump dispatches the queued key");

  // The queued key was consumed by the dialog, not by the frame's field.
  pump();
  CHECK(not frame->is_modal_blocked());

  frame->set_visible(false);
  drain();
}

// A window's face is painted at the window's own place: the screen used to
// hand each window the untranslated screen context, so an opaque window that
// does not cover the screen -- the dialog -- filled (0, 0, width, height) at
// the screen's origin as well, leaving a second, gray copy of the dialog in
// the upper-left corner of the frame.
static void test_dialog_paints_at_its_own_place() {
  terminal.set_type("text");
  drain();

  constexpr auto frame_color = Color { 0, 200, 0 };
  constexpr auto dialog_color = Color { 0, 0, 200 };

  auto frame = make_frame();
  frame->set_background_color(frame_color);
  pump();

  auto dialog = make_component<Dialog>(frame, "Placed", Dialog::ModalityType::MODELESS);
  dialog->set_background_color(dialog_color);
  auto content = make_component<Panel>();
  content->set_preferred_size(Dimension { 20, 4 });
  dialog->add(content);
  dialog->pack();
  dialog->set_location(24, 5);
  dialog->set_visible(true);
  pump();

  CHECK(dialog->is_showing());
  auto bounds = dialog->get_bounds();
  CHECK(bounds.x == 24 and bounds.y == 5 and bounds.width > 1 and bounds.height > 1);

  // The screen emits through std::cout; probe one cell at a time and clear the
  // buffer in between, so each probe looks at one repaint only.
  auto capture = std::ostringstream { };
  auto *old_cout = std::cout.rdbuf(capture.rdbuf());
  auto background_code = [](Color const &color) {
    return "\x1b[48;2;" + std::to_string(int(color.red())) + ';' + std::to_string(int(color.green())) + ';' + std::to_string(int(color.blue())) + 'm';
  };
  auto cell = [&](int x, int y) {
    dynamic_cast<TextScreen&>(screen).clear();
    capture.str({ });
    screen.add_damage(Rectangle { x, y, 1, 1 });
    screen.repaint_damaged();
    auto bytes = capture.str();
    capture.str({ });
    return bytes;
  };

  // The dialog's face is where the dialog is...
  auto inside = cell(bounds.x + bounds.width / 2, bounds.y + bounds.height / 2);
  CHECK(inside.find(background_code(dialog_color)) != std::string::npos);

  // ... and the screen's upper-left corner belongs to the frame.
  auto corner = cell(1, 1);
  CHECK(corner.find(background_code(frame_color)) != std::string::npos);
  CHECK(corner.find(background_code(dialog_color)) == std::string::npos);

  std::cout.rdbuf(old_cout);
  dialog->set_visible(false);
  pump();
  frame->set_visible(false);
  drain();
}

void test_Dialog() {
  std::fprintf(stderr, "test_Dialog: dialogs, modality and the modal event pump\n");
  test_dialog_show_and_pack();
  test_modeless_dialog();
  test_application_modal_dialog();
  test_document_modality();
  test_modal_event_pump();
  test_dialog_paints_at_its_own_place();
  std::fprintf(stderr, "test_Dialog: ok\n");
}
