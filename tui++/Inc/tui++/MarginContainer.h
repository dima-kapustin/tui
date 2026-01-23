#pragma once

#include <tui++/Insets.h>

#include <optional>

namespace tui {

class MarginContainer {
public:
  virtual ~MarginContainer() {
  }

public:
  virtual std::optional<Insets> const& get_margin() const = 0;
  virtual void set_margin(std::optional<Insets> const &margin) = 0;
};

}
