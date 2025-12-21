#pragma once

#include <variant>
#include <string_view>

namespace tui {

using Constraints = std::variant<std::monostate, int, std::string_view>;


}
