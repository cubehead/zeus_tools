#pragma once

#include <string>

namespace app::platform {

std::string choose_input_file();
std::string choose_export_file(const std::string& suggested_name);

} // namespace app::platform
