#include "zeus/secure_memory.h"

namespace zeus {

void secure_zero(void* data, std::size_t size) noexcept {
    volatile unsigned char* bytes = static_cast<volatile unsigned char*>(data);
    while (size > 0) {
        *bytes++ = 0;
        --size;
    }
}

void secure_clear(std::string& value) noexcept {
    if (!value.empty()) secure_zero(value.data(), value.size());
    value.clear();
}

} // namespace zeus
