#pragma once

#include <tui++/Component.h>

namespace tui {
namespace laf {
class MenuBarUI;
}

class MenuBar: public Component {
public:
  std::shared_ptr<laf::MenuBarUI> get_ui() const;

protected:
  MenuBar() {
  }

  template<typename T, typename ... Args>
  requires (is_component_v<T> )
  friend auto make_component(Args&&...);

  virtual std::shared_ptr<laf::ComponentUI> create_ui() override;
};

}
