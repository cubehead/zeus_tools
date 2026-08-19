#include "zeus/crypto_service.h"
#include "zeus/secure_memory.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <iomanip>
#include <sstream>

#ifdef __APPLE__
#include <CommonCrypto/CommonDigest.h>
#include <CommonCrypto/CommonHMAC.h>
#elif defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <bcrypt.h>
#endif

namespace zeus {
namespace {

std::size_t digest_size(DigestAlgorithm algorithm) {
    switch (algorithm) {
    case DigestAlgorithm::Md5: return 16;
    case DigestAlgorithm::Sha1: return 20;
    case DigestAlgorithm::Sha256: return 32;
    case DigestAlgorithm::Sha384: return 48;
    case DigestAlgorithm::Sha512: return 64;
    case DigestAlgorithm::Crc32: return 4;
    }
    return 0;
}

std::vector<std::uint8_t> crc32_digest(std::string_view input) {
    std::uint32_t value = 0xFFFFFFFFU;
    for (const unsigned char byte : input) {
        value ^= byte;
        for (int bit = 0; bit < 8; ++bit) {
            const std::uint32_t mask = 0U - (value & 1U);
            value = (value >> 1U) ^ (0xEDB88320U & mask);
        }
    }
    value ^= 0xFFFFFFFFU;
    return {
        static_cast<std::uint8_t>((value >> 24U) & 0xFFU),
        static_cast<std::uint8_t>((value >> 16U) & 0xFFU),
        static_cast<std::uint8_t>((value >> 8U) & 0xFFU),
        static_cast<std::uint8_t>(value & 0xFFU),
    };
}

std::string hex_encode(const std::vector<std::uint8_t>& bytes) {
    std::ostringstream output;
    output << std::hex << std::setfill('0');
    for (const std::uint8_t byte : bytes) output << std::setw(2) << static_cast<int>(byte);
    return output.str();
}

std::string base64_encode(const std::vector<std::uint8_t>& bytes) {
    static constexpr char alphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string output;
    output.reserve((bytes.size() + 2) / 3 * 4);
    for (std::size_t index = 0; index < bytes.size(); index += 3) {
        const std::uint32_t first = bytes[index];
        const std::uint32_t second = index + 1 < bytes.size() ? bytes[index + 1] : 0;
        const std::uint32_t third = index + 2 < bytes.size() ? bytes[index + 2] : 0;
        const std::uint32_t value = (first << 16U) | (second << 8U) | third;
        output.push_back(alphabet[(value >> 18U) & 0x3FU]);
        output.push_back(alphabet[(value >> 12U) & 0x3FU]);
        output.push_back(index + 1 < bytes.size() ? alphabet[(value >> 6U) & 0x3FU] : '=');
        output.push_back(index + 2 < bytes.size() ? alphabet[value & 0x3FU] : '=');
    }
    return output;
}

int base64_value(char ch) {
    if (ch >= 'A' && ch <= 'Z') return ch - 'A';
    if (ch >= 'a' && ch <= 'z') return ch - 'a' + 26;
    if (ch >= '0' && ch <= '9') return ch - '0' + 52;
    if (ch == '+' || ch == '-') return 62;
    if (ch == '/' || ch == '_') return 63;
    return -1;
}

bool decode_hmac_key(
    std::string_view encoded,
    HmacKeyEncoding encoding,
    std::string& decoded,
    std::string& error) {
    decoded.clear();
    error.clear();
    if (encoding == HmacKeyEncoding::Utf8) {
        decoded.assign(encoded);
        return true;
    }

    std::string compact;
    compact.reserve(encoded.size());
    for (const unsigned char ch : encoded) {
        if (!std::isspace(ch)) compact.push_back(static_cast<char>(ch));
    }

    if (encoding == HmacKeyEncoding::Hex) {
        if (compact.size() % 2 != 0) {
            error = "Hex key must contain an even number of digits";
            return false;
        }
        decoded.reserve(compact.size() / 2);
        const auto hex = [](char ch) {
            if (ch >= '0' && ch <= '9') return ch - '0';
            if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
            if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
            return -1;
        };
        for (std::size_t index = 0; index < compact.size(); index += 2) {
            const int high = hex(compact[index]);
            const int low = hex(compact[index + 1]);
            if (high < 0 || low < 0) {
                error = "Hex key contains a non-hexadecimal character";
                decoded.clear();
                return false;
            }
            decoded.push_back(static_cast<char>((high << 4) | low));
        }
        return true;
    }

    if (compact.empty()) return true;
    const bool standard_alphabet = compact.find_first_of("+/") != std::string::npos;
    const bool url_safe_alphabet = compact.find_first_of("-_") != std::string::npos;
    if (standard_alphabet && url_safe_alphabet) {
        error = "Base64 key mixes standard and URL-safe alphabets";
        return false;
    }
    const auto padding = compact.find('=');
    if (padding != std::string::npos) {
        const std::size_t padding_count = compact.size() - padding;
        if (compact.size() % 4 != 0 || padding_count > 2 ||
            compact.find_first_not_of('=', padding) != std::string::npos) {
            error = "Base64 key has invalid padding";
            return false;
        }
        compact.resize(padding);
        const std::size_t expected_padding = (4 - compact.size() % 4) % 4;
        if (expected_padding != padding_count) {
            error = "Base64 key has invalid padding";
            return false;
        }
    }
    if (compact.size() % 4 == 1) {
        error = "Base64 key has an invalid length";
        return false;
    }
    decoded.reserve((compact.size() * 3) / 4);
    std::uint32_t buffer = 0;
    int bits = 0;
    for (const char ch : compact) {
        const int value = base64_value(ch);
        if (value < 0) {
            error = "Base64 key contains an invalid character";
            decoded.clear();
            return false;
        }
        buffer = (buffer << 6U) | static_cast<std::uint32_t>(value);
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            decoded.push_back(static_cast<char>((buffer >> bits) & 0xFFU));
        }
    }
    if (bits > 0 && (buffer & ((1U << bits) - 1U)) != 0) {
        error = "Base64 key has non-zero trailing bits";
        decoded.clear();
        return false;
    }
    return true;
}

#ifdef __APPLE__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
bool platform_digest(
    std::string_view input,
    DigestAlgorithm algorithm,
    std::vector<std::uint8_t>& output) {
    output.resize(digest_size(algorithm));
    const auto* data = reinterpret_cast<const unsigned char*>(input.data());
    const CC_LONG size = static_cast<CC_LONG>(input.size());
    switch (algorithm) {
    case DigestAlgorithm::Md5: CC_MD5(data, size, output.data()); return true;
    case DigestAlgorithm::Sha1: CC_SHA1(data, size, output.data()); return true;
    case DigestAlgorithm::Sha256: CC_SHA256(data, size, output.data()); return true;
    case DigestAlgorithm::Sha384: CC_SHA384(data, size, output.data()); return true;
    case DigestAlgorithm::Sha512: CC_SHA512(data, size, output.data()); return true;
    case DigestAlgorithm::Crc32: return false;
    }
    return false;
}
#pragma clang diagnostic pop

CCHmacAlgorithm hmac_algorithm(DigestAlgorithm algorithm) {
    switch (algorithm) {
    case DigestAlgorithm::Md5: return kCCHmacAlgMD5;
    case DigestAlgorithm::Sha1: return kCCHmacAlgSHA1;
    case DigestAlgorithm::Sha256: return kCCHmacAlgSHA256;
    case DigestAlgorithm::Sha384: return kCCHmacAlgSHA384;
    case DigestAlgorithm::Sha512: return kCCHmacAlgSHA512;
    case DigestAlgorithm::Crc32: return kCCHmacAlgSHA256;
    }
    return kCCHmacAlgSHA256;
}

bool platform_hmac(
    std::string_view input,
    std::string_view key,
    DigestAlgorithm algorithm,
    std::vector<std::uint8_t>& output) {
    output.resize(digest_size(algorithm));
    CCHmac(
        hmac_algorithm(algorithm),
        key.data(), key.size(), input.data(), input.size(), output.data());
    return true;
}
#elif defined(_WIN32)
LPCWSTR bcrypt_algorithm(DigestAlgorithm algorithm) {
    switch (algorithm) {
    case DigestAlgorithm::Md5: return BCRYPT_MD5_ALGORITHM;
    case DigestAlgorithm::Sha1: return BCRYPT_SHA1_ALGORITHM;
    case DigestAlgorithm::Sha256: return BCRYPT_SHA256_ALGORITHM;
    case DigestAlgorithm::Sha384: return BCRYPT_SHA384_ALGORITHM;
    case DigestAlgorithm::Sha512: return BCRYPT_SHA512_ALGORITHM;
    case DigestAlgorithm::Crc32: return nullptr;
    }
    return BCRYPT_SHA256_ALGORITHM;
}

bool bcrypt_hash(
    std::string_view input,
    std::string_view key,
    DigestAlgorithm algorithm,
    bool hmac,
    std::vector<std::uint8_t>& output) {
    BCRYPT_ALG_HANDLE provider = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    const ULONG flags = hmac ? BCRYPT_ALG_HANDLE_HMAC_FLAG : 0;
    if (BCryptOpenAlgorithmProvider(
            &provider, bcrypt_algorithm(algorithm), nullptr, flags) < 0) return false;
    const auto close_provider = [&] { BCryptCloseAlgorithmProvider(provider, 0); };
    ULONG object_size = 0;
    ULONG result_size = 0;
    if (BCryptGetProperty(
            provider, BCRYPT_OBJECT_LENGTH,
            reinterpret_cast<PUCHAR>(&object_size), sizeof(object_size),
            &result_size, 0) < 0) {
        close_provider();
        return false;
    }
    std::vector<std::uint8_t> hash_object(object_size);
    PUCHAR key_data = hmac
        ? reinterpret_cast<PUCHAR>(const_cast<char*>(key.data()))
        : nullptr;
    const ULONG key_size = hmac ? static_cast<ULONG>(key.size()) : 0;
    if (BCryptCreateHash(
            provider, &hash, hash_object.data(), object_size,
            key_data, key_size, 0) < 0) {
        close_provider();
        return false;
    }
    // CNG rejects a null input pointer even when the byte count is zero.
    // A default-constructed string_view may expose nullptr for the valid empty
    // message, so finalize the newly-created hash directly in that case.
    auto* data = reinterpret_cast<PUCHAR>(const_cast<char*>(input.data()));
    if (!input.empty() &&
        BCryptHashData(hash, data, static_cast<ULONG>(input.size()), 0) < 0) {
        BCryptDestroyHash(hash);
        close_provider();
        return false;
    }
    output.resize(digest_size(algorithm));
    const bool ok = BCryptFinishHash(
        hash, output.data(), static_cast<ULONG>(output.size()), 0) >= 0;
    BCryptDestroyHash(hash);
    close_provider();
    return ok;
}

bool platform_digest(std::string_view input, DigestAlgorithm algorithm, std::vector<std::uint8_t>& output) {
    return bcrypt_hash(input, {}, algorithm, false, output);
}

bool platform_hmac(
    std::string_view input,
    std::string_view key,
    DigestAlgorithm algorithm,
    std::vector<std::uint8_t>& output) {
    return bcrypt_hash(input, key, algorithm, true, output);
}
#else
bool platform_digest(std::string_view, DigestAlgorithm, std::vector<std::uint8_t>&) { return false; }
bool platform_hmac(std::string_view, std::string_view, DigestAlgorithm, std::vector<std::uint8_t>&) { return false; }
#endif

CryptoResult finish(bool ok, std::vector<std::uint8_t> bytes) {
    CryptoResult result;
    result.ok = ok;
    if (!ok) {
        result.error = "Digest algorithm is unavailable on this platform";
        return result;
    }
    result.bytes = std::move(bytes);
    result.hex = hex_encode(result.bytes);
    result.base64 = base64_encode(result.bytes);
    return result;
}

} // namespace

const char* digest_algorithm_name(DigestAlgorithm algorithm) {
    switch (algorithm) {
    case DigestAlgorithm::Md5: return "MD5";
    case DigestAlgorithm::Sha1: return "SHA-1";
    case DigestAlgorithm::Sha256: return "SHA-256";
    case DigestAlgorithm::Sha384: return "SHA-384";
    case DigestAlgorithm::Sha512: return "SHA-512";
    case DigestAlgorithm::Crc32: return "CRC32";
    }
    return "SHA-256";
}

bool digest_algorithm_is_weak(DigestAlgorithm algorithm) {
    return algorithm == DigestAlgorithm::Md5 || algorithm == DigestAlgorithm::Sha1;
}

bool digest_algorithm_is_checksum(DigestAlgorithm algorithm) {
    return algorithm == DigestAlgorithm::Crc32;
}

CryptoResult compute_digest(std::string_view input, DigestAlgorithm algorithm) {
    if (algorithm == DigestAlgorithm::Crc32) {
        return finish(true, crc32_digest(input));
    }
    std::vector<std::uint8_t> bytes;
    const bool ok = platform_digest(input, algorithm, bytes);
    return finish(ok, std::move(bytes));
}

CryptoResult compute_hmac(
    std::string_view input,
    std::string_view key,
    DigestAlgorithm algorithm) {
    if (algorithm == DigestAlgorithm::Crc32) {
        CryptoResult result;
        result.error = "HMAC is not defined for CRC32";
        return result;
    }
    std::vector<std::uint8_t> bytes;
    const bool ok = platform_hmac(input, key, algorithm, bytes);
    return finish(ok, std::move(bytes));
}

CryptoResult compute_hmac_encoded(
    std::string_view input,
    std::string_view key,
    HmacKeyEncoding key_encoding,
    DigestAlgorithm algorithm) {
    std::string decoded_key;
    std::string error;
    if (!decode_hmac_key(key, key_encoding, decoded_key, error)) {
        secure_clear(decoded_key);
        CryptoResult result;
        result.error = std::move(error);
        return result;
    }
    CryptoResult result = compute_hmac(input, decoded_key, algorithm);
    secure_clear(decoded_key);
    return result;
}

} // namespace zeus
