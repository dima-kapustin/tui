#pragma once

#include <string>

namespace tui::BoxDrawing {
using namespace std::string_literals;

constexpr auto VERTICAL_LIGHT = "│"s;
constexpr auto VERTICAL_HEAVY = "┃"s;
constexpr auto VERTICAL_LIGHT_DOUBLE_DASH = "╎"s;
constexpr auto VERTICAL_HEAVY_DOUBLE_DASH = "╏"s;
constexpr auto VERTICAL_LIGHT_TRIPLE_DASH = "┆"s;
constexpr auto VERTICAL_HEAVY_TRIPLE_DASH = "┇"s;
constexpr auto VERTICAL_LIGHT_QUADRUPLE_DASH = "┊"s;
constexpr auto VERTICAL_HEAVY_QUADRUPLE_DASH = "┋"s;

constexpr auto HORIZONTAL_LIGHT = "─"s;
constexpr auto HORIZONTAL_HEAVY = "━"s;
constexpr auto HORIZONTAL_LIGHT_DOUBLE_DASH = "╌"s;
constexpr auto HORIZONTAL_HEAVY_DOUBLE_DASH = "╍"s;
constexpr auto HORIZONTAL_LIGHT_TRIPLE_DASH = "┄"s;
constexpr auto HORIZONTAL_HEAVY_TRIPLE_DASH = "┅"s;
constexpr auto HORIZONTAL_LIGHT_QUADRUPLE_DASH = "┈"s;
constexpr auto HORIZONTAL_HEAVY_QUADRUPLE_DASH = "┉"s;
}

