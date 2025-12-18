#pragma once

#include <tui++/AbstractButton.h>

namespace tui {
namespace laf {
class ButtonUI;
}

class Button: public AbstractButton {
public:
  std::shared_ptr<laf::ButtonUI> get_ui() const;

protected:
  Button(std::string const &text);

  template<typename T, typename ... Args>
  requires (is_component_v<T> )
  friend auto make_component(Args&&...);

  std::shared_ptr<laf::ComponentUI> create_ui() override;
};

}

