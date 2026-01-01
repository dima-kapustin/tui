#include <tui++/BoxLayout.h>

namespace tui::laf {

class MenuLayout: public BoxLayout {
  using base = BoxLayout;
public:
  MenuLayout(Component *target, Axis axis) :
      base(target, axis) {
  }

public:
  std::optional<Dimension> get_preferred_layout_size(const std::shared_ptr<const Component> &target) override;
};

}
