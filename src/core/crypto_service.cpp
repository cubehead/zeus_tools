#include "zeus/crypto_service.h"

#include <array>
#include <iomanip>
#include <sstream>

#ifdef __APPLE__
#include <CommonCrypto/CommonDigest.h>
#include <CommonCrypto/CommonHMAC.h>
#elif defined(_WIN32)
#define NOMINMAX
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
    case DigestAlgorithm::Sha512: return 64;
    }
    return 0;
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
    case DigestAlgorithm::Sha512: CC_SHA512(data, size, output.data()); return true;
    }
    return false;
}
#pragma clang diagnostic pop

CCHmacAlgorithm hmac_algorithm(DigestAlgorithm algorithm) {
    switch (algorithm) {
    case DigestAlgorithm::Md5: return kCCHmacAlgMD5;
    case DigestAlgorithm::Sha1: return kCCHmacAlgSHA1;
    case DigestAlgorithm::Sha256: return kCCHmacAlgSHA256;
    case DigestAlgorithm::Sha512: return kCCHmacAlgSHA512;
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
    case DigestAlgorithm::Sha512: return BCRYPT_SHA512_ALGORITHM;
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
    PUCHAR key_data = hmac
        ? reinterpret_cast<PUCHAR>(const_cast<char*>(key.data()))
        : nullptr;
    const ULONG key_size = hmac ? static_cast<ULONG>(key.size()) : 0;
    if (BCryptCreateHash(provider, &hash, nullptr, 0, key_data, key_size, 0) < 0) {
        close_provider();
        return false;
    }
    auto* data = reinterpret_cast<PUCHAR>(const_cast<char*>(input.data()));
    if (BCryptHashData(hash, data, static_cast<ULONG>(input.size()), 0) < 0) {
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
    case DigestAlgorithm::Sha512: return "SHA-512";
    }
    return "SHA-256";
}

bool digest_algorithm_is_weak(DigestAlgorithm algorithm) {
    return algorithm == DigestAlgorithm::Md5 || algorithm == DigestAlgorithm::Sha1;
}

CryptoResult compute_digest(std::string_view input, DigestAlgorithm algorithm) {
    std::vector<std::uint8_t> bytes;
    return finish(platform_digest(input, algorithm, bytes), std::move(bytes));
}

CryptoResult compute_hmac(
    std::string_view input,
    std::string_view key,
    DigestAlgorithm algorithm) {
    std::vector<std::uint8_t> bytes;
    return finish(platform_hmac(input, key, algorithm, bytes), std::move(bytes));
}

} // namespace zeus
