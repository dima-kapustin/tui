#pragma once

namespace tui {

struct Char {
  char utf8[8];
};

}

namespace tui::BoxDrawing {

constexpr Char VERTICAL_LIGHT { u8"│"};
constexpr Char VERTICAL_HEAVY { u8"┃"};
constexpr Char VERTICAL_LIGHT_DOUBLE_DASH { u8"╎"};
constexpr Char VERTICAL_HEAVY_DOUBLE_DASH { u8"╏"};
constexpr Char VERTICAL_LIGHT_TRIPLE_DASH { u8"┆"};
constexpr Char VERTICAL_HEAVY_TRIPLE_DASH { u8"┇"};
constexpr Char VERTICAL_LIGHT_QUADRUPLE_DASH { u8"┊"};
constexpr Char VERTICAL_HEAVY_QUADRUPLE_DASH { u8"┋"};

constexpr Char HORIZONTAL_LIGHT { u8"─"};
constexpr Char HORIZONTAL_HEAVY { u8"━"};
constexpr Char HORIZONTAL_LIGHT_DOUBLE_DASH { u8"╌"};
constexpr Char HORIZONTAL_HEAVY_DOUBLE_DASH { u8"╍"};
constexpr Char HORIZONTAL_LIGHT_TRIPLE_DASH { u8"┄"};
constexpr Char HORIZONTAL_HEAVY_TRIPLE_DASH { u8"┅"};
constexpr Char HORIZONTAL_LIGHT_QUADRUPLE_DASH { u8"┈"};
constexpr Char HORIZONTAL_HEAVY_QUADRUPLE_DASH { u8"┉"};
}

