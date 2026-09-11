// Exercises the Swing-style toggleable widgets: ToggleButton, CheckBox and
// RadioButton (with ButtonGroup exclusivity), CheckBoxMenuItem and
// RadioButtonMenuItem (also groupable), and the ComboBox (model, dropdown
// selection and lookup editing).
#include <tui++/Button.h>
#include <tui++/ButtonGroup.h>
#include <tui++/BorderLayout.h>
#include <tui++/CheckBox.h>
#include <tui++/CheckBoxMenuItem.h>
#include <tui++/ComboBox.h>
#include <tui++/DefaultComboBoxModel.h>
#include <tui++/Frame.h>
#include <tui++/KeyboardFocusManager.h>
#include <tui++/Menu.h>
#include <tui++/MenuItem.h>
#include <tui++/Panel.h>
#include <tui++/RadioButton.h>
#include <tui++/RadioButtonMenuItem.h>
#include <tui++/Screen.h>
#include <tui++/Shadow.h>
#include <tui++/TextField.h>
#include <tui++/ToggleButton.h>
#include <tui++/Window.h>

#include <tui++/lookandfeel/LookAndFeel.h>

#include <tui++/event/Event.h>
#include <tui++/event/FocusEvent.h>
#include <tui++/event/KeyEvent.h>
#include <tui++/event/MouseEvent.h>
#include <tui++/terminal/Terminal.h>
#include <tui++/terminal/text/TextScreen.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
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

static std::shared_ptr<Event> dispatch_typed(std::shared_ptr<Frame> const &frame, Char c, InputEvent::Modifiers modifiers = InputEvent::NO_MODIFIERS) {
  drain();
  screen.post<KeyEvent>(frame, c, modifiers);
  auto event = screen.get_event_queue().pop();
  frame->dispatch_event(*event);
  drain();
  return event;
}

// Dispatches a mouse wheel event at (x, y), window-local, through the
// window's normal dispatch path (the way the terminal input does).
static std::shared_ptr<Event> dispatch_wheel(std::shared_ptr<Window> const &window, int x, int y, int rotation) {
  drain();
  screen.post<MouseWheelEvent>(window, InputEvent::NO_MODIFIERS, x, y, rotation);
  auto event = screen.get_event_queue().pop();
  window->dispatch_event(*event);
  drain();
  return event;
}

// Presses and releases the left button at (x, y), window-local. The press
// carries its own button-down modifier, as the terminal input reports it, so
// the dispatcher records the pressed component as the release's target.
static void click_window(std::shared_ptr<Window> const &window, int x, int y) {
  drain();
  screen.post<MousePressEvent>(window, MousePressEvent::MOUSE_PRESSED, MousePressEvent::LEFT_BUTTON, InputEvent::LEFT_BUTTON_DOWN, x, y, false);
  window->dispatch_event(*screen.get_event_queue().pop());
  screen.post<MousePressEvent>(window, MousePressEvent::MOUSE_RELEASED, MousePressEvent::LEFT_BUTTON, InputEvent::NO_MODIFIERS, x, y, false);
  window->dispatch_event(*screen.get_event_queue().pop());
  drain();
}

// The toggle buttons and their grouping.
static void test_toggle_buttons() {
  // A CheckBox is a toggle: a click selects it, the next click clears it;
  // each pick fires exactly one ActionEvent.
  auto check = make_component<CheckBox>("Bold");
  auto check_actions = 0;
  check->add_listener([&check_actions](ActionEvent &) {
    ++check_actions;
  });
  CHECK(not check->is_selected());
  check->do_click(std::chrono::milliseconds::zero());
  CHECK(check->is_selected());
  check->do_click(std::chrono::milliseconds::zero());
  CHECK(not check->is_selected());
  CHECK(check_actions == 2);

  // A plain toggle button keeps its pressed-in state the same way.
  auto toggle = make_component<ToggleButton>("Run");
  toggle->do_click(std::chrono::milliseconds::zero());
  CHECK(toggle->is_selected());
  // The text bezel is the two vertical edges beside the label and takes no row
  // of its own: the label's cell is the button's whole height.
  CHECK(toggle->get_preferred_size().height == 1);

  // Radio buttons are mutually exclusive inside a ButtonGroup: picking a new
  // member clears the previous selection, and re-picking the selected member
  // leaves it selected (a radio cannot be turned off by clicking it).
  auto group = std::make_shared<ButtonGroup>();
  auto a = make_component<RadioButton>("Alpha");
  auto b = make_component<RadioButton>("Beta");
  auto c = make_component<RadioButton>("Gamma");
  group->add(a);
  group->add(b);
  group->add(c);

  a->do_click(std::chrono::milliseconds::zero());
  CHECK(a->is_selected());
  CHECK(not b->is_selected());
  CHECK(not c->is_selected());
  CHECK(group->get_selection() == a->get_model());

  c->do_click(std::chrono::milliseconds::zero());
  CHECK(not a->is_selected());
  CHECK(c->is_selected());
  CHECK(group->get_selection() == c->get_model());

  c->do_click(std::chrono::milliseconds::zero());
  CHECK(c->is_selected());

  // A checkbox inside a group can be cleared by clicking it again (the group
  // only arbitrates NEW selections).
  auto shared = std::make_shared<ButtonGroup>();
  auto box = make_component<CheckBox>("Snap");
  auto radio = make_component<RadioButton>("Grid");
  shared->add(box);
  shared->add(radio);
  box->do_click(std::chrono::milliseconds::zero());
  CHECK(box->is_selected());
  CHECK(group->get_button_count() == 3);
  radio->do_click(std::chrono::milliseconds::zero());
  CHECK(not box->is_selected());
  CHECK(radio->is_selected());
}

// Focus requests on toggle buttons: a request lands on the button itself
// when no group member can answer for it -- a standalone toggle, a group
// with no selection yet, or the group's own selection. (Regression: these
// cases used to return a null "group selection" that the focus request
// dereferenced, crashing on the first focus grant of a window.)
static void test_toggle_button_focus() {
  terminal.set_type("text");

  auto frame = make_component<Frame>();
  frame->set_size({ 80, 24 });
  auto content = frame->get_content_pane();

  auto bold = make_component<ToggleButton>("Bold");
  content->add(bold);

  auto group = std::make_shared<ButtonGroup>();
  auto left = make_component<RadioButton>("Align Left");
  auto center = make_component<RadioButton>("Align Center");
  group->add(left);
  group->add(center);
  content->add(left);
  content->add(center);

  frame->set_visible(true);
  drain();

  auto owner = [] {
    return KeyboardFocusManager::single->get_focus_owner();
  };

  // The standalone toggle keeps the request for itself (both focus paths).
  bold->request_focus(FocusEvent::Cause::ACTIVATION);
  CHECK(owner() == bold);
  bold->request_focus_in_window(FocusEvent::Cause::TRAVERSAL_FORWARD);
  CHECK(owner() == bold);

  // A group member requests focus for itself before the group has any
  // selection.
  left->request_focus(FocusEvent::Cause::ACTIVATION);
  CHECK(owner() == left);

  // Once the group has a selection it answers for an unselected member...
  center->do_click(std::chrono::milliseconds::zero());
  CHECK(center->is_selected());
  // ... and the selected member keeps the request for itself.
  center->request_focus(FocusEvent::Cause::ACTIVATION);
  CHECK(owner() == center);

  frame->set_visible(false);
  drain();
}

// The toggle widgets answer the mouse the way Swing's buttons do (Swing's
// BasicButtonListener): a left press arms and presses the model -- the
// "down" look -- and takes the focus, and the release over the button drops
// the pressed state, which fires the action. A click on the label or on the
// indicator picks the button; the grouped radio buttons stay exclusive.
static void test_toggle_button_mouse_clicks() {
  terminal.set_type("text");

  auto frame = make_component<Frame>();
  frame->set_size({ 80, 24 });
  auto content = frame->get_content_pane();
  auto row = make_component<Panel>(); // a row of controls, like the demo's

  auto bold = make_component<ToggleButton>("Bold");
  auto italic = make_component<CheckBox>("Italic");

  auto group = std::make_shared<ButtonGroup>();
  auto left = make_component<RadioButton>("Left");
  auto right = make_component<RadioButton>("Right");
  group->add(left);
  group->add(right);

  row->add(bold);
  row->add(italic);
  row->add(left);
  row->add(right);
  content->add(row);
  frame->set_visible(true);
  drain();

  auto click = [&](std::shared_ptr<Component> const &c, int dx) {
    auto at = c->get_location_on_screen();
    auto local = convert_point_from_screen(Point { at.x + dx, at.y + c->get_height() / 2 }, frame);
    click_window(frame, local.x, local.y);
  };

  auto bold_actions = 0;
  bold->add_listener([&bold_actions](ActionEvent &) {
    ++bold_actions;
  });

  // A click selects a toggle button, the next one clears it, and each click
  // fires exactly one action.
  click(bold, 1);
  CHECK(bold->is_selected());
  CHECK(bold_actions == 1);
  click(bold, 1);
  CHECK(not bold->is_selected());
  CHECK(bold_actions == 2);

  // The label belongs to the check box: a click on its last cell toggles it.
  click(italic, italic->get_width() - 1);
  CHECK(italic->is_selected());

  // Clicked radio buttons stay exclusive, and the click takes the focus.
  click(left, 1);
  CHECK(left->is_selected());
  CHECK(KeyboardFocusManager::single->get_focus_owner() == left);
  click(right, 1);
  CHECK(right->is_selected());
  CHECK(not left->is_selected());
  CHECK(KeyboardFocusManager::single->get_focus_owner() == right);

  // A button that is held and then released away from it does not fire: the
  // exit disarms it (the press's action would otherwise fire on the release).
  auto actions_before = bold_actions;
  drain();
  auto at = bold->get_location_on_screen();
  auto inside = convert_point_from_screen(Point { at.x + 1, at.y + bold->get_height() / 2 }, frame);
  auto outside = convert_point_from_screen(Point { at.x + 1, at.y + bold->get_height() + 1 }, frame);
  screen.post<MousePressEvent>(frame, MousePressEvent::MOUSE_PRESSED, MousePressEvent::LEFT_BUTTON, InputEvent::LEFT_BUTTON_DOWN, inside.x, inside.y, false);
  frame->dispatch_event(*screen.get_event_queue().pop());
  CHECK(bold->get_model()->is_pressed()); // the press shows the "down" look
  screen.post<MouseMoveEvent>(frame, InputEvent::LEFT_BUTTON_DOWN, outside.x, outside.y);
  frame->dispatch_event(*screen.get_event_queue().pop());
  screen.post<MousePressEvent>(frame, MousePressEvent::MOUSE_RELEASED, MousePressEvent::LEFT_BUTTON, InputEvent::NO_MODIFIERS, outside.x, outside.y, false);
  frame->dispatch_event(*screen.get_event_queue().pop());
  drain();
  CHECK(not bold->get_model()->is_pressed());
  CHECK(not bold->is_selected()); // the release away from the button did not pick it
  CHECK(bold_actions == actions_before);

  frame->set_visible(false);
  drain();
}

// The buttons' keyboard behavior, installed by their UI delegate the way
// Swing installs it: Space clicks the focused button (BasicButtonUI), and
// Enter clicks the window's default button (JRootPane.DefaultAction).
static void test_button_keyboard() {
  terminal.set_type("text");

  auto frame = make_component<Frame>();
  frame->set_size({ 80, 24 });
  auto content = frame->get_content_pane();

  auto run = make_component<Button>("Run");
  auto run_actions = 0;
  run->add_listener([&run_actions](ActionEvent &) {
    ++run_actions;
  });

  auto bold = make_component<CheckBox>("Bold");
  auto ok = make_component<Button>("OK");
  auto ok_actions = 0;
  ok->add_listener([&ok_actions](ActionEvent &) {
    ++ok_actions;
  });

  content->add(run);
  content->add(bold);
  content->add(ok);
  frame->set_visible(true);
  drain();

  auto focus_owner = [] {
    return KeyboardFocusManager::single->get_focus_owner();
  };

  // Space clicks the focused button, once per stroke.
  run->request_focus(FocusEvent::Cause::ACTIVATION);
  CHECK(focus_owner() == run);
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_SPACE);
  CHECK(run_actions == 1);
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_SPACE);
  CHECK(run_actions == 2);

  // The stroke belongs to the focus owner: a toggle clicks on it the way a
  // mouse release does, and a button without the focus ignores it.
  bold->request_focus(FocusEvent::Cause::ACTIVATION);
  CHECK(focus_owner() == bold);
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_SPACE);
  CHECK(bold->is_selected());
  CHECK(run_actions == 2);

  // Enter clicks the window's default button, from the default button itself
  // or from any other component that does not claim the key.
  frame->get_root_pane()->set_default_dutton(ok);
  ok->request_focus(FocusEvent::Cause::ACTIVATION);
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_ENTER);
  CHECK(ok_actions == 1);
  run->request_focus(FocusEvent::Cause::ACTIVATION);
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_ENTER);
  CHECK(ok_actions == 2);
  CHECK(run_actions == 2); // the focused button is not the default one

  // A disabled default button stays put, and without one Enter goes nowhere.
  ok->set_enabled(false);
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_ENTER);
  CHECK(ok_actions == 2);
  ok->set_enabled(true);
  frame->get_root_pane()->set_default_dutton(nullptr);
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_ENTER);
  CHECK(ok_actions == 2);

  // Enter reaches the default button of the window the focused component
  // belongs to, not the one of another window.
  auto other_frame = make_component<Frame>();
  other_frame->set_size({ 40, 10 });
  auto other_ok = make_component<Button>("OK");
  auto other_ok_actions = 0;
  other_ok->add_listener([&other_ok_actions](ActionEvent &) {
    ++other_ok_actions;
  });
  other_frame->get_content_pane()->add(other_ok);
  other_frame->get_root_pane()->set_default_dutton(other_ok);
  other_frame->set_visible(true);
  drain();

  run->request_focus(FocusEvent::Cause::ACTIVATION);
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_ENTER);
  CHECK(other_ok_actions == 0);

  other_frame->set_visible(false);
  frame->set_visible(false);
  drain();

  std::printf("PASS button keyboard (Space clicks, Enter clicks the default button)\n");
}

// The check/radio menu items and their grouping.
static void test_menu_items() {
  auto check_item = make_component<CheckBoxMenuItem>("Show grid");
  auto check_actions = 0;
  check_item->add_listener([&check_actions](ActionEvent &) {
    ++check_actions;
  });
  CHECK(not check_item->is_selected());
  check_item->do_click(std::chrono::milliseconds::zero());
  CHECK(check_item->is_selected());
  check_item->do_click(std::chrono::milliseconds::zero());
  CHECK(not check_item->is_selected());
  CHECK(check_actions == 2);

  // Radio menu items are exclusive through a ButtonGroup, exactly like radio
  // buttons: picking one clears the previous member of the group.
  auto group = std::make_shared<ButtonGroup>();
  auto left = make_component<RadioButtonMenuItem>("Align Left");
  auto center = make_component<RadioButtonMenuItem>("Align Center");
  auto right = make_component<RadioButtonMenuItem>("Align Right");
  group->add(left);
  group->add(center);
  group->add(right);

  center->do_click(std::chrono::milliseconds::zero());
  CHECK(center->is_selected());
  CHECK(not left->is_selected());
  right->do_click(std::chrono::milliseconds::zero());
  CHECK(not center->is_selected());
  CHECK(right->is_selected());

  // A plain menu item (ButtonModel) stays unselected after a click.
  auto plain = make_component<MenuItem>("Dump");
  plain->do_click(std::chrono::milliseconds::zero());
  CHECK(not plain->is_selected());
}

// The ComboBox model: contents, selection and change notifications.
static void test_combo_model() {
  auto model = std::make_shared<DefaultComboBoxModel>(std::vector<std::string> { "Alpha", "Beta", "Gamma" });
  auto changes = 0;
  model->add_listener([&changes](ChangeEvent &) {
    ++changes;
  });
  CHECK(model->get_size() == 3);
  CHECK(model->get_item_at(1) == "Beta");
  CHECK(not model->get_selected_index());

  model->set_selected_index(1);
  CHECK(model->get_selected_index() == 1);
  CHECK(changes == 1);

  model->set_selected_index(1); // no change, no event
  CHECK(changes == 1);

  // Inserting before the selection shifts it; removing the selected item
  // clears the selection; removing an earlier item shifts it down.
  model->insert_item_at("Zed", 0);
  CHECK(model->get_selected_index() == 2);
  model->remove_item_at(0);
  CHECK(model->get_selected_index() == 1);
  model->remove_item_at(1);
  CHECK(not model->get_selected_index());
  model->add_item("Delta");
  model->add_item("Epsilon");
  model->remove_item_at(0);
  CHECK(model->get_size() == 3);
  CHECK(model->get_item_at(0) == "Gamma");
  CHECK(model->get_item_at(1) == "Delta");
  CHECK(model->get_item_at(2) == "Epsilon");

  model->set_items({ "One", "Two", "Three" });
  CHECK(model->get_size() == 3);
  model->remove_all_items();
  CHECK(model->get_size() == 0);
  CHECK(changes > 0);
}

// The ComboBox on a text screen: dropdown selection and lookup editing.
static void test_combo_box() {
  terminal.set_type("text");

  auto frame = make_component<Frame>();
  frame->set_size({ 80, 24 });

  auto content = frame->get_content_pane();
  auto combo = make_component<ComboBox>(std::vector<std::string> { "Alpha", "Beta", "Gamma", "Delta", "Epsilon" });
  combo->set_name("combo");
  content->add(combo);

  auto actions = std::vector<std::string> { };
  combo->add_listener([&actions](ActionEvent &e) {
    actions.push_back(e.action_command);
  });

  frame->set_visible(true);
  drain();
  combo->request_focus(FocusEvent::Cause::ACTIVATION);
  CHECK(combo->is_focus_owner());

  // ---- programmatic selection fires no ActionEvent ----
  combo->set_selected_index(1);
  CHECK(combo->get_selected_item() == "Beta");
  CHECK(actions.empty());

  // ---- lookup editing: type "Be", Enter commits the matching item ----
  combo->set_editable(true);
  dispatch_typed(frame, Char('B'));
  dispatch_typed(frame, Char('e'));
  CHECK(combo->get_field_text() == "Be"); // draft, nothing committed yet
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_ENTER);
  CHECK(combo->get_selected_item() == "Beta");
  CHECK(combo->get_field_text() == "Beta");
  CHECK(actions.size() == 1);
  CHECK(actions.back() == "Beta");

  // ---- a stray (or repeated, or auto-repeated) Enter after a commit is a
  // no-op: it must not wipe the selection or the field ----
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_ENTER);
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_ENTER);
  CHECK(combo->get_selected_item() == "Beta");
  CHECK(combo->get_field_text() == "Beta");
  CHECK(actions.size() == 1); // the stray Enters fired nothing

  // ---- custom text: type letters no item starts with, Enter keeps them ----
  dispatch_typed(frame, Char('x'));
  dispatch_typed(frame, Char('y'));
  CHECK(combo->get_field_text() == "xy");
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_ENTER);
  CHECK(actions.size() == 2);
  CHECK(actions.back() == "xy");
  CHECK(not combo->get_selected_index()); // the custom value stays unselected
  // Committing a custom value keeps the typed text in the field (the index
  // model cannot hold it, so it stays unselected but visible).
  CHECK(combo->get_field_text() == "xy");

  // ---- Escape reverts an uncommitted edit ----
  dispatch_typed(frame, Char('z'));
  CHECK(combo->get_field_text() == "z");
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_ESCAPE);
  CHECK(combo->get_field_text() == "xy"); // reverted to the custom value

  // ---- the dropdown: arrow down opens it, arrows move, Enter picks ----
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_DOWN);
  CHECK(combo->is_popup_visible());
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_DOWN);
  CHECK(combo->is_popup_visible());
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_DOWN);
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_DOWN);
  CHECK(combo->is_popup_visible());
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_ENTER);
  CHECK(not combo->is_popup_visible());
  CHECK(combo->get_selected_item() == "Delta"); // Alpha + three downs

  // ---- typing while the dropdown is open moves the highlight to the typed
  // prefix; Enter commits that item ----
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_DOWN);
  CHECK(combo->is_popup_visible());
  dispatch_typed(frame, Char('G'));
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_ENTER);
  CHECK(not combo->is_popup_visible());
  CHECK(combo->get_selected_item() == "Gamma");
  CHECK(actions.back() == "Gamma");

  // ---- Escape closes the dropdown without picking ----
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_DOWN);
  CHECK(combo->is_popup_visible());
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_ESCAPE);
  CHECK(not combo->is_popup_visible());
  CHECK(combo->get_selected_item() == "Gamma");

  // ---- type-ahead on a non-editable combo selects as the letters come in ----
  auto read_only = make_component<ComboBox>(std::vector<std::string> { "Red", "Green", "Blue" });
  read_only->set_name("read-only");
  content->add(read_only);
  frame->validate();
  drain();
  read_only->request_focus(FocusEvent::Cause::ACTIVATION);
  CHECK(read_only->is_focus_owner());
  CHECK(read_only->get_field_text() == "");
  dispatch_typed(frame, Char('B'));
  CHECK(read_only->get_selected_item() == "Blue");
  CHECK(read_only->get_field_text() == "Blue");

  // ---- the dropdown of a long list scrolls to the maximum row count ----
  auto many = make_component<ComboBox>();
  for (auto i = 0; i != 30; ++i) {
    many->add_item("Item " + std::to_string(i));
  }
  many->set_maximum_row_count(5);
  many->set_name("many");
  content->add(many);
  frame->validate();
  drain();
  many->request_focus(FocusEvent::Cause::ACTIVATION);
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_DOWN); // opens the dropdown
  CHECK(many->is_popup_visible());
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_END); // armed at the end
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_ENTER);
  CHECK(not many->is_popup_visible());
  CHECK(many->get_selected_item() == "Item 29");

  // ---- row count and mutation while the popup is open ----
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_DOWN);
  CHECK(many->is_popup_visible());
  many->set_maximum_row_count(3);
  CHECK(many->is_popup_visible());
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_ESCAPE);
  CHECK(not many->is_popup_visible());

  // ---- the mouse wheel scrolls the dropdown's window; a click on a row
  // picks whatever row is visible at the pointer ----
  {
    // The dropdown opens at the bottom of the list (the selection, Item 29,
    // is scrolled into view: rows 27-29) and shows three rows.
    dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_DOWN);
    CHECK(many->is_popup_visible());

    // The popup drops under the combo; each open creates a fresh popup
    // window, so hit-test it on the screen while it is up.
    auto popup_at = [&]() -> std::shared_ptr<Window> {
      auto origin = many->get_location_on_screen();
      return screen.get_window_at(origin.x + 2, origin.y + many->get_height());
    };
    auto popup_window = popup_at();
    CHECK(popup_window);
    CHECK(popup_window.get() != frame.get());

    // One notch scrolls the window three rows toward the earlier items, so
    // the first visible row is Item 24 (rows 24-26).
    dispatch_wheel(popup_window, 2, 0, -1);
    click_window(popup_window, 2, 0);
    CHECK(not many->is_popup_visible());
    CHECK(many->get_selected_item() == "Item 24");

    // The other direction scrolls back toward the later items. Reopening
    // scrolls the selection (Item 24) into view first (rows 22-24); one
    // notch moves the window to rows 25-27.
    dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_DOWN);
    CHECK(many->is_popup_visible());
    auto reopened = popup_at();
    CHECK(reopened);
    CHECK(reopened.get() != popup_window.get()); // every open has its own window
    dispatch_wheel(reopened, 2, 0, +1);
    click_window(reopened, 2, 0);
    CHECK(not many->is_popup_visible());
    CHECK(many->get_selected_item() == "Item 25");
  }

  // ---- a model swap updates the contents and resets the selection ----
  combo->set_model(std::make_shared<DefaultComboBoxModel>(std::vector<std::string> { "Uno", "Dos" }));
  CHECK(combo->get_item_count() == 2);
  CHECK(not combo->get_selected_index());
  CHECK(combo->get_field_text() == "");

  frame->set_visible(false);
  drain();

  std::printf("PASS widgets (toggle buttons, check/radio menu items, combo box)\n");
}

// Two combo boxes side by side on one screen: the dropdown's window spans
// the combo's width with its right border aligned to the box's, opening one
// combo's dropdown dismisses another combo's open dropdown (and the
// dismissed combo's bookkeeping follows -- it neither keeps holding the
// window's keys nor toggles a ghost on the next arrow click), and the
// editable field keeps working while its dropdown is open: Backspace edits
// the draft and every change re-highlights the first item (from the top)
// that matches it.
static void test_combo_dropdown() {
  terminal.set_type("text");

  auto frame = make_component<Frame>();
  frame->set_size({ 80, 24 });
  auto content = frame->get_content_pane();
  auto row = make_component<Panel>(); // a FlowLayout row, like the demo's

  auto city = make_component<ComboBox>(std::vector<std::string> { "Barcelona", "Paris", "London", "Rome", "Berlin", "Madrid", "Amsterdam", "Prague", "Vienna" });
  city->set_editable(true);
  city->set_selected_index(0);
  city->set_name("city");
  auto size = make_component<ComboBox>();
  for (auto i = 8; i <= 24; i += 2) {
    size->add_item(std::to_string(i) + " pt");
  }
  size->set_maximum_row_count(5);
  size->set_selected_index(0);
  size->set_name("size");
  row->add(city);
  row->add(size);
  content->add(row, BorderLayout::CENTER);

  frame->set_visible(true);
  drain();

  // The popup window under `c` (null when no dropdown is showing).
  auto dropdown_window = [](std::shared_ptr<ComboBox> const &c) -> std::shared_ptr<Window> {
    auto at = c->get_location_on_screen();
    auto window = screen.get_window_at(at.x + 2, at.y + c->get_height() + 1);
    return window and window.get() != c->get_containing_window().get() ? window : nullptr;
  };
  // The index of the highlighted row of `c`'s open dropdown, or -1.
  auto armed_row = [&dropdown_window](std::shared_ptr<ComboBox> const &c) -> int {
    auto window = dropdown_window(c);
    if (not window or window->get_content_pane()->get_component_count() == 0) {
      return -1;
    }
    auto menu = std::dynamic_pointer_cast<Component>(window->get_content_pane()->get_component(0));
    auto index = 0;
    for (auto &&component : menu->get_components()) {
      if (auto item = std::dynamic_pointer_cast<MenuItem>(component); item and item->is_armed()) {
        return index;
      }
      ++index;
    }
    return -1;
  };
  // Clicks the arrow strip of `c` (its rightmost cell) through the frame.
  auto arrow_click = [&](std::shared_ptr<ComboBox> const &c) {
    auto at = c->get_location_on_screen();
    auto local = convert_point_from_screen(Point { at.x + c->get_width() - 1, at.y + c->get_height() / 2 }, frame);
    click_window(frame, local.x, local.y);
  };

  // ---- the dropdown's width matches the box's, right borders aligned ----
  {
    city->set_popup_visible(true);
    drain();
    auto window = dropdown_window(city);
    CHECK(window);
    auto at = city->get_location_on_screen();
    CHECK(window->get_location().x == at.x);
    CHECK(window->get_size().width == city->get_width());
    CHECK(window->get_location().x + window->get_size().width == at.x + city->get_width());

    auto size_window = dropdown_window(size);
    CHECK(not size_window); // only one dropdown on the screen
    city->set_popup_visible(false);
    drain();
    CHECK(not dropdown_window(city));
  }

  // ---- opening the second combo's dropdown dismisses the first's ----
  {
    city->set_popup_visible(true);
    drain();
    CHECK(city->is_popup_visible());

    size->set_popup_visible(true);
    drain();
    CHECK(not city->is_popup_visible()); // closed, not just hidden
    CHECK(not dropdown_window(city));
    CHECK(size->is_popup_visible());

    // Arrow clicks keep working with a dropdown on the screen: a click on
    // the other box's arrow moves the dropdown to it, another click closes
    // it again.
    arrow_click(city);
    CHECK(city->is_popup_visible());
    CHECK(not size->is_popup_visible());
    arrow_click(city);
    CHECK(not city->is_popup_visible());
    arrow_click(size);
    CHECK(size->is_popup_visible());
    CHECK(not city->is_popup_visible());
    arrow_click(size);
    CHECK(not size->is_popup_visible());
  }

  // ---- a mouse pick in the dropdown commits and closes it (non-editable
  // combo, opened by clicking the arrow): click the second row ("10 pt") ----
  {
    arrow_click(size);
    CHECK(size->is_popup_visible());
    auto window = dropdown_window(size);
    CHECK(window);
    click_window(window, 2, 1); // the second row of the dropdown
    CHECK(not size->is_popup_visible());
    CHECK(size->get_selected_item() == "10 pt");
  }

  // ---- the editable field works while the dropdown is open: Backspace and
  // the live lookup ----
  {
    city->request_focus(FocusEvent::Cause::ACTIVATION);
    CHECK(city->is_focus_owner());
    dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_DOWN); // opens the dropdown
    CHECK(city->is_popup_visible());

    // "be" highlights Berlin (the first "be" from the top); Backspace
    // reverts the draft to "b" and the highlight jumps back to Barcelona,
    // the first "b" from the top -- the lookup follows every edit live.
    dispatch_typed(frame, Char('b'));
    CHECK(armed_row(city) == 0); // Barcelona
    dispatch_typed(frame, Char('e'));
    CHECK(city->get_field_text() == "be");
    CHECK(armed_row(city) == 4); // Berlin
    dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_BACK_SPACE);
    CHECK(city->get_field_text() == "b");
    CHECK(armed_row(city) == 0); // Barcelona again
    dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_ENTER);
    CHECK(not city->is_popup_visible());
    CHECK(city->get_selected_item() == "Barcelona");
    CHECK(city->get_field_text() == "Barcelona");

    // Delete and the caret keys edit the draft with the dropdown open too.
    dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_DOWN);
    CHECK(city->is_popup_visible());
    dispatch_typed(frame, Char('a'));
    CHECK(city->get_field_text() == "a");
    dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_ESCAPE); // revert
    CHECK(city->get_field_text() == "Barcelona");
    CHECK(not city->is_popup_visible());

    // An unmatched draft drops the stale highlight (Enter must not pick the
    // row that was highlighted before the typing); Enter keeps the typed text
    // as the custom value and the field shows it.
    dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_DOWN);
    CHECK(city->is_popup_visible());
    dispatch_typed(frame, Char('q'));
    CHECK(city->get_field_text() == "q");
    CHECK(armed_row(city) == -1); // nothing highlighted
    dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_ENTER);
    CHECK(not city->is_popup_visible());
    CHECK(not city->get_selected_index()); // the custom value stays unselected
    CHECK(city->get_field_text() == "q"); // and stays in the field

    // A press in the editor field moves the caret and keeps the dropdown open
    // (only a press outside the combo and its dropdown dismisses it).
    dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_DOWN);
    CHECK(city->is_popup_visible());
    {
      auto field = city->get_editor();
      CHECK(field);
      auto at = city->get_location_on_screen();
      auto local = convert_point_from_screen(Point { at.x + 1, at.y }, frame);
      click_window(frame, local.x, local.y);
      CHECK(city->is_popup_visible());
      CHECK(not field->has_selection()); // the click collapsed the selection
      CHECK(field->get_caret_position() == 0); // and placed the caret on the click
    }
  }

  // ---- hiding the frame takes an open dropdown down with it ----
  // A popup cannot be displayed over a window that is gone, so the screen
  // takes the popups stacked above a hidden window off with it (see
  // Screen::hide_window). They have to come down for real -- not merely
  // vanish from the screen's list -- or the dropdown keeps believing it is
  // showing and the menu system's next session walks into a "window not
  // visible" error. The dropdown also has to open again on the next click.
  {
    city->request_focus(FocusEvent::Cause::ACTIVATION);
    city->set_popup_visible(true);
    drain();
    CHECK(city->is_popup_visible());
    auto window = dropdown_window(city);
    CHECK(window);
    CHECK(window->is_showing());

    frame->set_visible(false);
    drain();
    CHECK(not window->is_showing());
    CHECK(not city->is_popup_visible());

    frame->set_visible(true);
    drain();
    city->request_focus(FocusEvent::Cause::ACTIVATION);
    city->set_popup_visible(true);
    drain();
    CHECK(city->is_popup_visible());
    auto reopened = dropdown_window(city);
    CHECK(reopened and reopened->is_showing());
    city->set_popup_visible(false);
    drain();
  }

  frame->set_visible(false);
  drain();

  std::printf("PASS combo dropdowns (aligned width, one at a time, live lookup)\n");
}

// The SGR the text screen emits for a cell painted on `color`.
static std::string background_code(Color const &color) {
  return "\x1b[48;2;" + std::to_string(int(color.red())) + ';' + std::to_string(int(color.green())) + ';' + std::to_string(int(color.blue())) + 'm';
}

// The color a shadow leaves on a cell painted on `base`: the cell's color
// shifted towards the shadow's, the same arithmetic the text screen applies.
static Color shaded(Color const &base, Shadow const &shadow) {
  auto blend = [&](uint8_t from, uint8_t to) {
    return uint8_t(std::lround(from * (1 - shadow.opacity) + to * shadow.opacity));
  };
  return Color { blend(base.red(), shadow.color.red()), blend(base.green(), shadow.color.green()), blend(base.blue(), shadow.color.blue()) };
}

// Repaints one cell of the screen and hands back what the screen emitted for
// it (see test_Shadow, which probes cells the same way): a full repaint emits
// every cell, while the question here is what one particular cell carries.
static std::string probe_cell(std::ostringstream &capture, int x, int y) {
  dynamic_cast<TextScreen &>(screen).clear();
  capture.str({ });
  screen.add_damage(Rectangle { x, y, 1, 1 });
  screen.repaint_damaged();
  auto bytes = capture.str();
  capture.str({ });
  return bytes;
}

// The optional drop shadow of a button: the border reserves the room it takes,
// so an enabled shadow makes the button's box (and every layout around it)
// larger, and the bands outside the face are shaded towards the shadow's
// color. The theme may define "<Prefix>.Shadow" for a kind; nothing is
// installed by default.
static void test_button_shadow() {
  terminal.set_type("text");
  drain();

  auto plain = make_component<Button>("OK");
  CHECK(not plain->get_shadow().has_value());

  auto shadow = Shadow { BLACK_COLOR, 0.5, Point { 2, 1 } };
  auto shadowed = make_component<Button>("OK");
  shadowed->set_shadow(shadow);
  CHECK(shadowed->get_shadow() == shadow);

  // The room is part of the button's insets: the bezel's own one cell on the
  // sides, plus the two columns and the row the shadow falls outside the face.
  CHECK((plain->get_insets() == Insets { 0, 1, 0, 1 }));
  CHECK((shadowed->get_insets() == Insets { 0, 1, 1, 3 }));
  CHECK(shadowed->get_preferred_size().width == plain->get_preferred_size().width + 2);
  CHECK(shadowed->get_preferred_size().height == plain->get_preferred_size().height + 1);

  // The one global shadow switch turns it off, room included.
  Shadow::set_enabled(false);
  CHECK(not shadowed->get_shadow().has_value());
  CHECK((shadowed->get_insets() == Insets { 0, 1, 0, 1 }));
  CHECK(shadowed->get_preferred_size().width == plain->get_preferred_size().width);
  Shadow::set_enabled(true);
  CHECK(shadowed->get_shadow().has_value());

  // The painting: the band between the face's edge and the button's own box
  // carries the face color shaded towards the shadow's.
  auto frame = make_component<Frame>();
  frame->set_size({ 40, 6 });
  auto content = frame->get_content_pane();
  content->set_layout(std::make_shared<BorderLayout>());
  content->add(shadowed, BorderLayout::NORTH);
  frame->set_visible(true);
  drain();

  // The painting: the shadow's area is the face displaced by (2, 1), so with a
  // one-row button it falls on the box's second row -- shifted right by the
  // offset (the columns before it keep the face color, the box itself is part
  // of the button).
  auto capture = std::ostringstream { };
  auto *old_cout = std::cout.rdbuf(capture.rdbuf());
  auto face = shadowed->get_background_color().value_or(Color { 0xC0, 0xC0, 0xC0 });
  auto at = shadowed->get_location_on_screen();
  auto shadowed_color = shaded(face, shadow);
  auto band = probe_cell(capture, at.x + shadow.offset.x + 1, at.y + shadowed->get_height() - 1);
  CHECK(band.find(background_code(shadowed_color)) != std::string::npos);
  auto outside = probe_cell(capture, at.x, at.y + shadowed->get_height() - 1);
  CHECK(outside.find(background_code(face)) != std::string::npos);
  CHECK(outside.find(background_code(shadowed_color)) == std::string::npos);

  std::cout.rdbuf(old_cout);
  frame->set_visible(false);
  drain();

  std::printf("PASS button shadows (layout room, bands and the global switch)\n");
}

void test_Widgets() {
  test_toggle_buttons();
  test_toggle_button_focus();
  test_toggle_button_mouse_clicks();
  test_button_keyboard();
  test_button_shadow();
  test_menu_items();
  test_combo_model();
  test_combo_box();
  test_combo_dropdown();
}
