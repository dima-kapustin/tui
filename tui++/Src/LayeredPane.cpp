#include <tui++/LayeredPane.h>

namespace tui {

void LayeredPane::set_layer(const std::shared_ptr<Component> &c, int layer, int position) {
  if (layer == get_layer(c) and position == get_position(c)) {
    repaint(c->get_bounds());
    return;
  }

  c->set_client_property(LAYER_PROPERTY, layer);

  if (auto parent = c->get_parent(); not parent or parent.get() != this) {
    repaint(c->get_bounds());
    return;
  }

  int index = insert_index_for_layer(c, layer, position);

  set_component_z_order(c, index);
  repaint(c->get_bounds());
}

int LayeredPane::insert_index_for_layer(const std::shared_ptr<Component> &c, int layer, int position) {
  auto components = std::vector<std::shared_ptr<Component>> { };
  components.reserve(this->components.size() - 1);
  for (auto &&child : this->components) {
    if (child != c) {
      components.emplace_back(child);
    }
  }

  int start_layer = -1;
  int end_layer = -1;

  auto count = (int) components.size();
  for (auto i = 0; i < count; i++) {
    auto current_layer = get_layer(components[i]);
    if (start_layer == -1 and current_layer == layer) {
      start_layer = i;
    }
    if (current_layer < layer) {
      if (i == 0) {
        // layer is greater than any current layer
        // [ ASSERT(layer > highest_layer()) ]
        start_layer = 0;
        end_layer = 0;
      } else {
        end_layer = i;
      }
      break;
    }
  }

  // layer requested is lower than any current layer
  // [ ASSERT(layer < lowest_layer()) ]
  // put it on the bottom of the stack
  if (start_layer == -1 and end_layer == -1)
    return count;

  // In the case of a single layer entry handle the degenerative cases
  if (start_layer != -1 and end_layer == -1)
    end_layer = count;

  if (end_layer != -1 and start_layer == -1)
    start_layer = end_layer;

  // If we are adding to the bottom, return the last element
  if (position == -1)
    return end_layer;

  // Otherwise make sure the requested position falls in the
  // proper range
  if (position > -1 and (start_layer + position) <= end_layer)
    return start_layer + position;

  // Otherwise return the end of the layer
  return end_layer;
}

int LayeredPane::get_position(const std::shared_ptr<Component> &c) {
  auto start_position = get_component_index(c);
  if (start_position == npos)
    return -1;

  int pos = 0;
  auto start_layer = get_layer(c);
  for (auto i = (int) start_position - 1; i >= 0; --i) {
    auto current_layer = get_layer(get_component(i));
    if (current_layer == start_layer) {
      pos += 1;
    } else {
      return pos;
    }
  }
  return pos;
}

}
