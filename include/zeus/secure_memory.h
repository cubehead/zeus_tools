#pragma once

#include <cstddef>
#include <string>

namespace zeus {

void secure_zero(void* data, std::size_t size) noexcept;
void secure_clear(std::string& value) noexcept;

} // namespace zeus
