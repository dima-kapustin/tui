#pragma once

#include <tui++/Component.h>

#include <tui++/border/LineBorder.h>

#include <tui++/lookandfeel/LookAndFeel.h>

namespace tui::laf {

class FrameUI: public ComponentUI {
  virtual void install_ui(std::shared_ptr<Component> const &c) override {
//    c->set_border(std::make_shared<LineBorder>(Stroke::DOUBLE, CYAN_COLOR, BLUE_COLOR));
    c->set_border(std::make_shared<LineBorder>(Stroke::DOUBLE, Color { 0, 255, 255 }, Color { 0, 0, 255 }));
  }
};

}

