#include <tui++/BoxLayout.h>

namespace tui {
class MenuItem;
}

namespace tui::laf {
class MenuItemUI;

class MenuLayout: public BoxLayout {
  using base = BoxLayout;
public:
  MenuLayout(Component *target, Axis axis) :
      base(target, axis) {
  }

public:
  std::optional<Dimension> get_preferred_layout_size(const std::shared_ptr<const Component> &target) override;

private:
  Dimension get_preferred_size(MenuItem const *menu_item);

  friend class MenuItemUI;
};

}
