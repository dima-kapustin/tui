#pragma once

#include <string>

namespace tui::util {

char32_t mb_to_u32(std::array<char, 4> bytes);
std::string u32_to_mb(char32_t c);

}
