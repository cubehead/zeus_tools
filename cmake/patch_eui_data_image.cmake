if(NOT DEFINED SOURCE_DIR OR NOT IS_DIRECTORY "${SOURCE_DIR}")
    message(FATAL_ERROR "SOURCE_DIR must point to an EUI-NEO checkout")
endif()

set(path "${SOURCE_DIR}/core/render/image_source.cpp")
if(NOT EXISTS "${path}")
    message(FATAL_ERROR "EUI Data URL image patch target is missing: ${path}")
endif()
file(READ "${path}" contents)

set(marker "bool decodeBase64ImageDataUrl(")
string(FIND "${contents}" "${marker}" already_patched)
if(NOT already_patched EQUAL -1)
    return()
endif()

set(before [=[
bool hasSvgExtension(const std::string& path) {
]=])
set(after [=[
bool decodeBase64ImageDataUrl(const std::string& source, std::vector<unsigned char>& bytes) {
    constexpr const char* pngPrefix = "data:image/png;base64,";
    constexpr const char* jpegPrefix = "data:image/jpeg;base64,";
    constexpr const char* jpgPrefix = "data:image/jpg;base64,";
    std::size_t offset = 0;
    if (source.rfind(pngPrefix, 0) == 0) offset = std::char_traits<char>::length(pngPrefix);
    else if (source.rfind(jpegPrefix, 0) == 0) offset = std::char_traits<char>::length(jpegPrefix);
    else if (source.rfind(jpgPrefix, 0) == 0) offset = std::char_traits<char>::length(jpgPrefix);
    else return false;

    const auto value = [](unsigned char ch) -> int {
        if (ch >= 'A' && ch <= 'Z') return ch - 'A';
        if (ch >= 'a' && ch <= 'z') return ch - 'a' + 26;
        if (ch >= '0' && ch <= '9') return ch - '0' + 52;
        if (ch == '+') return 62;
        if (ch == '/') return 63;
        return -1;
    };
    bytes.clear();
    bytes.reserve((source.size() - offset) * 3 / 4);
    std::uint32_t buffer = 0;
    int bits = 0;
    for (std::size_t i = offset; i < source.size(); ++i) {
        const unsigned char ch = static_cast<unsigned char>(source[i]);
        if (ch == '=') break;
        const int digit = value(ch);
        if (digit < 0) return false;
        buffer = (buffer << 6U) | static_cast<std::uint32_t>(digit);
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            bytes.push_back(static_cast<unsigned char>((buffer >> bits) & 0xFFU));
        }
    }
    return !bytes.empty();
}

bool hasSvgExtension(const std::string& path) {
]=])
string(FIND "${contents}" "${before}" found_helper_target)
if(found_helper_target EQUAL -1)
    message(FATAL_ERROR "EUI Data URL helper patch no longer applies to: ${path}")
endif()
string(REPLACE "${before}" "${after}" contents "${contents}")

set(before [=[
std::string imageCacheKey(const std::string& resolvedPath, bool flipVertically) {
    return baseImageCacheKey(resolvedPath, flipVertically) + imageFileVersionSuffix(resolvedPath);
}

std::string resolveImagePath(const std::string& source, bool* pending) {
]=])
set(after [=[
std::string imageCacheKey(const std::string& resolvedPath, bool flipVertically) {
    if (resolvedPath.rfind("data:image/", 0) == 0) {
        return "data-image:" + std::to_string(resolvedPath.size()) + ":" +
               std::to_string(std::hash<std::string>{}(resolvedPath)) +
               (flipVertically ? "#flip" : "#noflip");
    }
    return baseImageCacheKey(resolvedPath, flipVertically) + imageFileVersionSuffix(resolvedPath);
}

std::string resolveImagePath(const std::string& source, bool* pending) {
    if (source.rfind("data:image/", 0) == 0) {
        if (pending != nullptr) *pending = false;
        return source;
    }
]=])
string(FIND "${contents}" "${before}" found_resolve_target)
if(found_resolve_target EQUAL -1)
    message(FATAL_ERROR "EUI Data URL resolver patch no longer applies to: ${path}")
endif()
string(REPLACE "${before}" "${after}" contents "${contents}")

set(before [=[
    std::vector<unsigned char> svgPixels;
    unsigned char* pixels = nullptr;

    if (hasSvgExtension(resolvedPath) || looksLikeSvgFile(resolvedPath)) {
]=])
set(after [=[
    std::vector<unsigned char> svgPixels;
    std::vector<unsigned char> encodedImageBytes;
    unsigned char* pixels = nullptr;

    if (decodeBase64ImageDataUrl(resolvedPath, encodedImageBytes)) {
        constexpr int kMaxPreviewDimension = 8192;
        constexpr std::int64_t kMaxPreviewPixels = 40LL * 1024LL * 1024LL;
        if (stbi_info_from_memory(encodedImageBytes.data(),
                                  static_cast<int>(encodedImageBytes.size()),
                                  &width, &height, &channels) == 0 ||
            width <= 0 || height <= 0 ||
            width > kMaxPreviewDimension || height > kMaxPreviewDimension ||
            static_cast<std::int64_t>(width) * static_cast<std::int64_t>(height) >
                kMaxPreviewPixels) {
            return {};
        }
        stbi_set_flip_vertically_on_load(flipVertically ? 1 : 0);
        pixels = stbi_load_from_memory(encodedImageBytes.data(),
                                       static_cast<int>(encodedImageBytes.size()),
                                       &width, &height, &channels, STBI_rgb_alpha);
        if (pixels == nullptr || width <= 0 || height <= 0) {
            if (pixels != nullptr) stbi_image_free(pixels);
            return {};
        }
    } else if (resolvedPath.rfind("data:image/", 0) == 0) {
        return {};
    } else if (hasSvgExtension(resolvedPath) || looksLikeSvgFile(resolvedPath)) {
]=])
string(FIND "${contents}" "${before}" found_loader_target)
if(found_loader_target EQUAL -1)
    message(FATAL_ERROR "EUI Data URL loader patch no longer applies to: ${path}")
endif()
string(REPLACE "${before}" "${after}" contents "${contents}")

file(WRITE "${path}" "${contents}")
