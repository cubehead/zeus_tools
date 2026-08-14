#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace zeus {

enum class DigestAlgorithm {
    Md5,
    Sha1,
    Sha256,
    Sha512,
};

enum class HmacKeyEncoding {
    Utf8,
    Hex,
    Base64,
};

struct CryptoResult {
    bool ok = false;
    std::vector<std::uint8_t> bytes;
    std::string hex;
    std::string base64;
    std::string error;
};

const char* digest_algorithm_name(DigestAlgorithm algorithm);
bool digest_algorithm_is_weak(DigestAlgorithm algorithm);
CryptoResult compute_digest(std::string_view input, DigestAlgorithm algorithm);
CryptoResult compute_hmac(
    std::string_view input,
    std::string_view key,
    DigestAlgorithm algorithm);
CryptoResult compute_hmac_encoded(
    std::string_view input,
    std::string_view key,
    HmacKeyEncoding key_encoding,
    DigestAlgorithm algorithm);

} // namespace zeus
