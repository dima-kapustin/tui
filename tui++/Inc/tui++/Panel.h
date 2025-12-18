#pragma once

#include <tui++/Component.h>

namespace tui {
namespace laf {
class PanelUI;
}

class Panel: public Component {
  using base = Component;

public:
  Panel();

  Panel(const std::shared_ptr<Layout> &layout) {
    set_layout(layout);
  }

public:
  void paint(Graphics &g);

  std::shared_ptr<laf::PanelUI> get_ui() const;

protected:
  std::shared_ptr<laf::ComponentUI> create_ui() override;
};

}

