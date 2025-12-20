#pragma once

#include <tui++/Component.h>
#include <tui++/Orientation.h>

namespace tui {
namespace laf {
class SeparatorUI;
}

class Separator: public Component {
  using base = Component;

  Property<Orientation> orientation = { this, "orientation" };
public:
  std::shared_ptr<laf::SeparatorUI> get_ui() const;

  Orientation get_orientation() const {
    return this->orientation;
  }

  void set_orientation(Orientation orientation) {
    if (this->orientation != orientation) {
      this->orientation = orientation;
      revalidate();
      repaint();
    }
  }

protected:
  Separator() :
      Separator(Orientation::HORIZONTAL) {
  }

  Separator(Orientation orientation) {
    this->orientation = orientation;
  }

  template<typename T, typename ... Args>
  requires (is_component_v<T> )
  friend auto make_component(Args&&...);

  virtual void init() override {
    base::init();
    set_focusable(false);
  }

  std::shared_ptr<laf::ComponentUI> create_ui() override;
};

}
