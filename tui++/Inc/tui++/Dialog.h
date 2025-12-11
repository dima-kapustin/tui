#pragma once

#include <tui++/Frame.h>

namespace tui {

class Dialog: public Window {

protected:
  Property<std::string> title { this, "title" };

public:
  Dialog(const std::shared_ptr<Dialog> &owner);
  Dialog(const std::shared_ptr<Frame> &owner);
  Dialog(const std::shared_ptr<Window> &owner);
};

}
