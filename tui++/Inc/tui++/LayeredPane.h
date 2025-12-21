#pragma once

#include <tui++/Component.h>

namespace tui {

class LayeredPane: public Component {
  constexpr static auto LAYER_PROPERTY = "layered-pane-layer";

public:
  constexpr static auto DEFAULT_LAYER = Constraints { 0 };
  constexpr static auto PALETTE_LAYER = Constraints { 100 };
  constexpr static auto MODAL_LAYER = Constraints { 200 };
  constexpr static auto POPUP_LAYER = Constraints { 300 };
  constexpr static auto DRAG_LAYER = Constraints { 400 };
  constexpr static auto FRAME_CONTENT_LAYER = Constraints { -30000 };

public:
  static std::shared_ptr<LayeredPane> get_layered_pane_above(const std::shared_ptr<Component> &c) {
    return c ? c->get_parent<LayeredPane>() : std::shared_ptr<LayeredPane> { };
  }

  int get_layer(const std::shared_ptr<Component> &c) {
    if (auto *layer = c->get_client_property<int>(LAYER_PROPERTY)) {
      return *layer;
    }
    return std::get<int>(DEFAULT_LAYER);
  }

  void set_layer(const std::shared_ptr<Component> &c, int layer, int position = -1);

  int get_position(const std::shared_ptr<Component> &c);

private:
  int insert_index_for_layer(const std::shared_ptr<Component> &c, int layer, int position);
};

}
