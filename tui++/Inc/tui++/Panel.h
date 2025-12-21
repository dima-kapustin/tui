#pragma once

#include <tui++/Component.h>

namespace tui {
namespace laf {
class PanelUI;
}

class Panel: public Component {
  using base = Component;

public:
  void paint(Graphics &g);

  std::shared_ptr<laf::PanelUI> get_ui() const;

protected:
  Panel();

  Panel(const std::shared_ptr<Layout> &layout) {
    set_layout(layout);
  }

  template<typename T, typename ... Args>
  requires (is_component_v<T> )
  friend auto make_component(Args&&...);

protected:
  std::shared_ptr<laf::ComponentUI> create_ui() override;
};

}

