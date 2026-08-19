#include "core/render/image_source.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

void expect(bool condition, const char* message) {
    if (condition) return;
    std::cerr << "FAIL: " << message << '\n';
    std::exit(1);
}

} // namespace

int main() {
    const std::string png =
        "data:image/png;base64,"
        "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mP8/x8AAusB9Y9Zr6sAAAAASUVORK5CYII=";
    bool pending = true;
    const auto decoded = core::render::image::loadStaticImage(png, false, &pending);
    expect(!pending, "in-memory Data URL images must not enter the network pending state");
    expect(decoded != nullptr, "a valid PNG Data URL should decode from memory");
    expect(decoded->width == 1 && decoded->height == 1,
           "the decoded PNG should retain its source dimensions");
    expect(decoded->pixels.size() == 4,
           "the decoded one-pixel PNG should use RGBA output");

    const auto cached = core::render::image::loadStaticImage(png, false, nullptr);
    expect(cached == decoded, "repeated Data URL previews should reuse the decoded image cache");
    expect(core::render::image::isSourceReady(png),
           "a supported Data URL image should be immediately ready");

    const auto invalid = core::render::image::loadStaticImage(
        "data:image/png;base64,not-base64", false, nullptr);
    expect(invalid == nullptr, "invalid Base64 image data must be rejected safely");
    const auto unsupported = core::render::image::loadStaticImage(
        "data:image/gif;base64,R0lGODlhAQABAIAAAAAAAP///ywAAAAAAQABAAACAUwAOw==",
        false,
        nullptr);
    expect(unsupported == nullptr,
           "the in-memory preview bridge should remain limited to PNG and JPEG");

    std::cout << "Data URL image smoke checks passed.\n";
    return 0;
}
