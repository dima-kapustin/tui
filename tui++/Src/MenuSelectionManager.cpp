#include <tui++/MenuSelectionManager.h>
#include <tui++/Component.h>

namespace tui {

void MenuSelectionManager::set_selected_path(std::vector<std::shared_ptr<MenuElement>> const &path) {
  auto first_diff_index = 0;
  for (auto i = size_t { }, n = path.size(); i != n; ++i) {
    if (i < this->selection.size() and this->selection[i] == path[i]) {
      first_diff_index += 1;
    } else {
      break;
    }
  }

  for (auto i = (int) this->selection.size() - 1; i >= first_diff_index; --i) {
    auto element = this->selection[i];
    this->selection.erase(std::next(this->selection.begin(), i));
    element->menu_selection_changed(false);
  }

  this->selection.reserve(first_diff_index + path.size());
  for (auto i = size_t(first_diff_index), n = path.size(); i < n; ++i) {
    if (path[i]) {
      this->selection.emplace_back(path[i]);
      path[i]->menu_selection_changed(true);
    }
  }

  fire_state_changed();
}

void MenuSelectionManager::process_mouse_event(MouseEvent &e) {
  if (auto component = e.component(); component and component->is_showing()) {
    auto done = false;
    auto selection = this->selection;
    auto&& [screen_x, screen_y] = e.source ? convert_point_to_screen(e.point, component) : e.point;

    auto path = std::vector<std::shared_ptr<MenuElement>> { };
    path.reserve(selection.size());
    for (auto i = (int) selection.size() - 1; not done and i >= 0; --i) {
      path.clear();
      for (auto &&sub_element : selection[i]->get_sub_elements()) {
        if (sub_element) {
          if (auto c = sub_element->get_component(); c->is_showing()) {
            auto p = convert_point_from_screen(screen_x, screen_y, c);

            /** Send the event to visible menu element if menu element currently in
             *  the selected path or contains the event location
             */
            if ((p.x >= 0 and p.x < c->get_width() and p.y >= 0 and p.y < c->get_height())) {
              if (path.empty()) {
                std::copy_n(selection.begin(), i, std::back_inserter(path));
              }

              path.push_back(sub_element);

              // Enter/exit detection -- needs tuning...
              if (selection[selection.size() - 1] != path[i + 1] and (selection.size() < 2 or selection[selection.size() - 2] != path[i + 1])) {
                auto exit_event = make_event < MouseOverEvent > (selection[selection.size() - 1]->get_component(), MouseOverEvent::MOUSE_EXITED, e.modifiers, p.x, p.y, e.when);
                selection[selection.size() - 1]->process_mouse_event(exit_event, path, shared_from_this());

                auto enter_event = make_event < MouseOverEvent > (c, MouseOverEvent::MOUSE_ENTERED, e.modifiers, p.x, p.y, e.when);
                sub_element->process_mouse_event(enter_event, path, shared_from_this());
              }

              auto temp_point = e.point;
              e.point = p;
              try {
                sub_element->process_mouse_event(e, path, shared_from_this());
              } catch (std::exception const&) {
                e.point = temp_point;
                throw;
              }

              e.point = temp_point;
              done = true;
              e.consume();
            }

            if (done) {
              break;
            }
          }
        }
      }
    }
  }
}

void MenuSelectionManager::process_key_event(KeyEvent &e) {
  auto selection = this->selection;
  if (selection.size() < 1) {
    return;
  }

  auto path = std::vector<std::shared_ptr<MenuElement>> { };
  path.reserve(selection.size());
  for (auto i = (int) selection.size() - 1; i >= 0; --i) {
    path.clear();
    for (auto &&sub_element : selection[i]->get_sub_elements()) {
      if (sub_element and sub_element->get_component()->is_showing() and sub_element->get_component()->is_enabled()) {
        if (path.empty()) {
          std::copy_n(selection.begin(), i + 1, std::back_inserter(path));
        }
        path.push_back(sub_element);
        sub_element->process_key_event(e, path, shared_from_this());
        if (e.consumed) {
          return;
        }
      }
    }
  }

  // finally dispatch event to the first component in path
  path.clear();
  path.push_back(selection[0]);
  selection[0]->process_key_event(e, path, shared_from_this());
}

std::shared_ptr<Component> MenuSelectionManager::component_for_point(const std::shared_ptr<Component> &source, const Point &source_point) {
  auto selection = this->selection;
  auto&& [screenX, screenY] = convert_point_to_screen(source_point, source);
  for (auto i = (int) selection.size() - 1; i >= 0; --i) {
    for (auto &&sub_element : selection[i]->get_sub_elements()) {
      if (sub_element) {
        if (auto c = sub_element->get_component(); c->is_showing()) {
          auto p = convert_point_from_screen(screenX, screenY, c);
          /** Return the deepest component on the selection
           *  path in whose bounds the event's point occurs
           */
          if (p.x >= 0 and p.x < c->get_width() and p.y >= 0 and p.y < c->get_height()) {
            return c;
          }
        }
      }
    }
  }
  return nullptr;
}

}
