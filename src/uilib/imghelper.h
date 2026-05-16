#pragma once

#include <algorithm>
#include <cassert>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "size.h"
#include "../core/fs.h"

namespace Ui {

inline SDL_Surface* LoadImage(const fs::path& path)
{
#ifdef _WIN32
    // on Windows, SDL_Image uses SDL's IO functions, which expect UTF8
    return IMG_Load(path.u8string().c_str());
#else
    // otherwise it's probably fopen, which is "native" encoding
    return IMG_Load(path.c_str());
#endif
}

enum class ImageFormat {
    Unknown,
    // in order of most common to least common
    PNG,
    JPEG,
    GIF,
    BMP,
};

constexpr char PNG_SIGNATURE[8] = {'\x89', 'P', 'N', 'G', '\x0D', '\x0A', '\x1A', '\x0A'};
constexpr char JPEG_SIGNATURE[3] = {'\xff', '\xd8', '\xff'};
constexpr char GIF87_SIGNATURE[6] = {'G', 'I', 'F', '8', '7', 'a'};
constexpr char GIF89_SIGNATURE[6] = {'G', 'I', 'F', '8', '9', 'a'};
constexpr char BMP_SIGNATURE[2] = {'B', 'M'};

constexpr size_t getMaxImageSignatureLength()
{
    return std::max({
        sizeof(PNG_SIGNATURE),
        sizeof(JPEG_SIGNATURE),
        sizeof(GIF87_SIGNATURE),
        sizeof(GIF89_SIGNATURE),
        sizeof(BMP_SIGNATURE),
    });
}

constexpr size_t getMaxImageHeaderLength()
{
    // TODO: instead have a "needs more data" result or a stream?
    return std::max({
        getMaxImageSignatureLength(),
        sizeof(PNG_SIGNATURE) + 16,
        sizeof(JPEG_SIGNATURE) - 1 + 253, // JPEGs have varying fields before SOF, best effort.
        sizeof(GIF87_SIGNATURE) + 4,
        sizeof(GIF89_SIGNATURE) + 4,
        sizeof(BMP_SIGNATURE) + 24,
    });
}

inline ImageFormat detectImageFormat(const std::string& data)
{
    // in order of most common to least common
    if (data.length() >= sizeof(PNG_SIGNATURE)) {
        if (memcmp(data.data(), PNG_SIGNATURE, sizeof(PNG_SIGNATURE)) == 0) {
            return ImageFormat::PNG;
        }
    }
    if (data.length() >= sizeof(JPEG_SIGNATURE)) {
        if (memcmp(data.data(), JPEG_SIGNATURE, sizeof(JPEG_SIGNATURE)) == 0) {
            return ImageFormat::JPEG;
        }
    }
    if (data.length() >= sizeof(GIF89_SIGNATURE)) {
        if (memcmp(data.data(), GIF89_SIGNATURE, sizeof(GIF89_SIGNATURE)) == 0) {
            return ImageFormat::GIF;
        }
    }
    if (data.length() >= sizeof(GIF87_SIGNATURE)) {
        if (memcmp(data.data(), GIF87_SIGNATURE, sizeof(GIF87_SIGNATURE)) == 0) {
            return ImageFormat::GIF;
        }
    }
    if (data.length() >= sizeof(BMP_SIGNATURE)) {
        if (memcmp(data.data(), BMP_SIGNATURE, sizeof(BMP_SIGNATURE)) == 0) {
            // TODO: also validate that filesize and data offset are sane?
            return ImageFormat::BMP;
        }
    }
    return ImageFormat::Unknown;
}

inline uint32_t pngReadU32(const void* data)
{
    const auto* p = static_cast<const uint8_t*>(data);
    return  static_cast<uint32_t>(p[3])
        | (static_cast<uint32_t>(p[2]) << 8)
        | (static_cast<uint32_t>(p[1]) << 16)
        | (static_cast<uint32_t>(p[0]) << 24);
}

inline Size getPNGSize(const std::string& data)
{
    assert(data.length() > sizeof(PNG_SIGNATURE) && memcmp(data.data(), PNG_SIGNATURE, sizeof(PNG_SIGNATURE)) == 0);
    Size res = Size::UNDEFINED;
    if (data.length() >= 24) {
        const uint32_t iHdrLen = pngReadU32(data.data() + 8);
        if (iHdrLen >= 8 && memcmp(data.data() + 12, "IHDR", 4) == 0) {
            const uint32_t w = pngReadU32(data.data() + 16);
            const uint32_t h = pngReadU32(data.data() + 20);
            if (w <= std::numeric_limits<int>::max() && h <= std::numeric_limits<int>::max()
                    && static_cast<uint64_t>(w) * static_cast<uint64_t>(h) <= std::numeric_limits<uint32_t>::max()) {
                res.width = static_cast<int>(w);
                res.height = static_cast<int>(h);
            }
        }
    }
    return res;
}

inline uint16_t bmpReadU16(const void* data)
{
    const auto* p = static_cast<const uint8_t*>(data);
    return  static_cast<uint16_t>(p[0])
        | (static_cast<uint16_t>(p[1]) << 8);
}

inline uint32_t bmpReadU32(const void* data)
{
    const auto* p = static_cast<const uint8_t*>(data);
    return  static_cast<uint16_t>(p[0])
        | (static_cast<uint16_t>(p[1]) << 8)
        | (static_cast<uint16_t>(p[2]) << 16)
        | (static_cast<uint16_t>(p[3]) << 24);
}

inline int32_t bmpReadI32(const void* data)
{
    const uint32_t v = bmpReadU32(data);
    return static_cast<int32_t>(v);
}

inline Size getBMPSize(const std::string& data)
{
    assert(data.length() > sizeof(BMP_SIGNATURE) && memcmp(data.data(), BMP_SIGNATURE, sizeof(BMP_SIGNATURE)) == 0);
    Size res = Size::UNDEFINED;
    if (data.length() >= 22) {
        const auto hdrSize = bmpReadU32(data.data() + 14);
        if (hdrSize == 12) {
            // OS/2 header
            res.width = bmpReadU16(data.data() + 18);
            res.height = bmpReadU16(data.data() + 20);
        } else if (hdrSize == 64 || hdrSize == 16) {
            // OS/2 V2 and V2 short variant not implemented
        } else if (hdrSize >= 40 && hdrSize <= 124) {
            // Windows header V1-V5
            if (data.size() >= 26) {
                const int32_t w = bmpReadI32(data.data() + 18);
                const int32_t h = bmpReadI32(data.data() + 22);
                if (w >= 0) {
                    if (h < 0 && h != std::numeric_limits<int32_t>::min()) {
                        res.width = w;
                        res.height = -1 * h;
                    } else if (h >= 0) {
                        res.width = w;
                        res.height = h;
                    }
                }
            }
        }
    }
    return res;
}

inline uint16_t gifReadU16(const void* data)
{
    const auto* p = static_cast<const uint8_t*>(data);
    return  static_cast<uint16_t>(p[0])
        | (static_cast<uint16_t>(p[1]) << 8);
}

inline Size getGIFSize(const std::string& data)
{
    assert(
        (data.length() >= sizeof(GIF87_SIGNATURE)
            && memcmp(data.data(), GIF87_SIGNATURE, sizeof(GIF87_SIGNATURE)) == 0)
        || (data.length() >= sizeof(GIF89_SIGNATURE)
            && memcmp(data.data(), GIF89_SIGNATURE, sizeof(GIF89_SIGNATURE)) == 0)
    );
    Size res = Size::UNDEFINED;
    if (data.length() >= 10) {
        res.width = gifReadU16(data.data() + 6);
        res.height = gifReadU16(data.data() + 8);
    }
    return res;
}

inline uint16_t jpegReadU16(const void* data)
{
    const auto* p = static_cast<const uint8_t*>(data);
    return  static_cast<uint16_t>(p[1])
        | (static_cast<uint16_t>(p[0]) << 8);
}

inline Size getJPEGSize(const std::string& data)
{
    assert(data.length() >= sizeof(JPEG_SIGNATURE) && memcmp(data.data(), JPEG_SIGNATURE, sizeof(JPEG_SIGNATURE)) == 0);
    Size res = Size::UNDEFINED;
    // TODO: have a way to signal that we need more data, but with a cap to avoid reading the whole file
    if (data.length() > 2) {
        std::string_view sv{data.data() + 2, data.length() - 2};
        while (sv.length() >= 2) {
            if (sv[0] != '\xff')
                break;
            switch (sv[1]) {
                case '\xda': // image data
                case '\xd9': // end of image
                    goto done; // past headers -> done
                case '\xc0': // SOF0 - baseline
                case '\xc1': // SOF1 - extended, huffman
                case '\xc2': // SOF2 - progressive, huffman
                case '\xc3': // SOF3 - lossless, huffman
                case '\xc9': // SOF9 - extended, arithmetic
                case '\xca': // SOF10 - progressive, arithmetic
                case '\xcb': // SOF11 - lossless, arithmetic
                // c5-c7 (SOF5-SOF7) unsupported
                // cd-cf (SOF13-SOF15) unsupported
                    if (sv.length() >= 9 && jpegReadU16(sv.data() + 2) >= 7) {
                        res.height = jpegReadU16(sv.data() + 5);
                        res.width = jpegReadU16(sv.data() + 7);
                    }
                    goto done; // only 1 SOF allowed -> done
                default:
                    if (sv.length() >= 4) {
                        const uint16_t len = jpegReadU16(sv.data() + 2);
                        if (len < sv.length() - 2) {
                            sv = {sv.data() + 2 + len, sv.length() - 2 - len};
                        }
                        else
                            goto done;
                    } else {
                        goto done;
                    }
            }
        }
    }
done:
    return res;
}

inline Size getImageSize(const std::string& data)
{
    // TODO: differentiate between unknown/unsupported and error?
    const ImageFormat format = detectImageFormat(data);
    if (format == ImageFormat::PNG)
        return getPNGSize(data);
    if (format == ImageFormat::JPEG)
        return getJPEGSize(data);
    // beware that GIF can have a different "canvas" and "data" size - for some reason, SDL_Image ignores the "canvas",
    // so we can't actually support GIF in detectImageFormat unless we change LoadImage
    if (format == ImageFormat::GIF)
        return Size::UNDEFINED; // getGIFSize(data);
    if (format == ImageFormat::BMP)
        return getBMPSize(data);
    return Size::UNDEFINED;
}

} // namespace Ui
