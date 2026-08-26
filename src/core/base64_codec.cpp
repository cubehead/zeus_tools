#include "zeus/base64_codec.h"

#include <cstdint>

namespace zeus {

std::string encode_base64(const std::string& input) {
    constexpr char alphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string output;
    output.reserve(((input.size() + 2) / 3) * 4);
    for (std::size_t index = 0; index < input.size(); index += 3) {
        const auto a = static_cast<unsigned char>(input[index]);
        const auto b = index + 1 < input.size()
            ? static_cast<unsigned char>(input[index + 1]) : 0U;
        const auto c = index + 2 < input.size()
            ? static_cast<unsigned char>(input[index + 2]) : 0U;
        const std::uint32_t value = (static_cast<std::uint32_t>(a) << 16U) |
                                    (static_cast<std::uint32_t>(b) << 8U) |
                                    static_cast<std::uint32_t>(c);
        output.push_back(alphabet[(value >> 18U) & 0x3FU]);
        output.push_back(alphabet[(value >> 12U) & 0x3FU]);
        output.push_back(index + 1 < input.size()
            ? alphabet[(value >> 6U) & 0x3FU] : '=');
        output.push_back(index + 2 < input.size()
            ? alphabet[value & 0x3FU] : '=');
    }
    return output;
}

} // namespace zeus
