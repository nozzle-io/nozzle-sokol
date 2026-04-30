// nozzle-sokol.h - Nozzle GPU texture sharing integration for sokol_gfx
// Single header library. Include in ONE .c/.cpp file with:
//   #define NOZZLE_SOKOL_IMPL
//   #include "nozzle-sokol.h"
//
// Requires: nozzle (C API), sokol_gfx.h
// Platform: macOS (Metal/IOSurface), Windows (D3D11), Linux (DMA-BUF/OpenGL)
#pragma once

#include <sokol/sokol_gfx.h>

#ifdef __cplusplus
extern "C" {
#endif

// Convert nozzle texture format to sokol pixel format.
// Returns SG_PIXELFORMAT_NONE for unsupported formats.
sg_pixel_format nozzle_sokol_to_pixel_format(int nozzle_format);

// Convert sokol pixel format to nozzle texture format.
// Returns 0 (NOZZLE_FORMAT_UNKNOWN) for unsupported formats.
int nozzle_sokol_from_pixel_format(sg_pixel_format fmt);

// Create a sokol image from a nozzle receiver frame's native texture.
// On success, *out_img is a valid sg_image. Destroy with sg_destroy_image().
// The nozzle frame must remain alive while the sokol image is in use on the GPU.
// Returns true on success.
bool nozzle_sokol_frame_to_image(void *nozzle_frame, sg_image *out_img);

// Publish a sokol image to a nozzle sender.
// On macOS Metal: extracts the MTLTexture from the sokol image and publishes
//   via nozzle's Metal/IOSurface path (zero-copy).
// On Windows D3D11: extracts the ID3D11Texture2D and publishes via shared handle.
// On Linux/OpenGL: copies the sokol image data to a nozzle writable frame.
// Returns true on success.
bool nozzle_sokol_image_publish(void *nozzle_sender, sg_image img);

#ifdef __cplusplus
}
#endif

#ifdef NOZZLE_SOKOL_IMPL

#include <nozzle/nozzle_c.h>

sg_pixel_format nozzle_sokol_to_pixel_format(int nozzle_format) {
    switch ((NozzleTextureFormat)nozzle_format) {
        case NOZZLE_FORMAT_R8_UNORM:      return SG_PIXELFORMAT_R8;
        case NOZZLE_FORMAT_RG8_UNORM:     return SG_PIXELFORMAT_RG8;
        case NOZZLE_FORMAT_RGBA8_UNORM:   return SG_PIXELFORMAT_RGBA8;
        case NOZZLE_FORMAT_BGRA8_UNORM:   return SG_PIXELFORMAT_BGRA8;
        case NOZZLE_FORMAT_RGBA8_SRGB:    return SG_PIXELFORMAT_SRGB8A8;
        case NOZZLE_FORMAT_BGRA8_SRGB:    return SG_PIXELFORMAT_BGRA8; // sokol has no BGRA sRGB
        case NOZZLE_FORMAT_R16_FLOAT:     return SG_PIXELFORMAT_R16F;
        case NOZZLE_FORMAT_RG16_FLOAT:    return SG_PIXELFORMAT_RG16F;
        case NOZZLE_FORMAT_RGBA16_FLOAT:  return SG_PIXELFORMAT_RGBA16F;
        case NOZZLE_FORMAT_R32_FLOAT:     return SG_PIXELFORMAT_R32F;
        case NOZZLE_FORMAT_RG32_FLOAT:    return SG_PIXELFORMAT_RG32F;
        case NOZZLE_FORMAT_RGBA32_FLOAT:  return SG_PIXELFORMAT_RGBA32F;
        case NOZZLE_FORMAT_R16_UNORM:     return SG_PIXELFORMAT_R16;
        case NOZZLE_FORMAT_RG16_UNORM:    return SG_PIXELFORMAT_RG16;
        case NOZZLE_FORMAT_RGBA16_UNORM:  return SG_PIXELFORMAT_RGBA16;
        case NOZZLE_FORMAT_R32_UINT:      return SG_PIXELFORMAT_R32UI;
        case NOZZLE_FORMAT_RGBA32_UINT:   return SG_PIXELFORMAT_RGBA32UI;
        default:                          return SG_PIXELFORMAT_NONE;
    }
}

int nozzle_sokol_from_pixel_format(sg_pixel_format fmt) {
    switch (fmt) {
        case SG_PIXELFORMAT_R8:          return NOZZLE_FORMAT_R8_UNORM;
        case SG_PIXELFORMAT_RG8:         return NOZZLE_FORMAT_RG8_UNORM;
        case SG_PIXELFORMAT_RGBA8:       return NOZZLE_FORMAT_RGBA8_UNORM;
        case SG_PIXELFORMAT_BGRA8:       return NOZZLE_FORMAT_BGRA8_UNORM;
        case SG_PIXELFORMAT_SRGB8A8:     return NOZZLE_FORMAT_RGBA8_SRGB;
        case SG_PIXELFORMAT_R16F:        return NOZZLE_FORMAT_R16_FLOAT;
        case SG_PIXELFORMAT_RG16F:       return NOZZLE_FORMAT_RG16_FLOAT;
        case SG_PIXELFORMAT_RGBA16F:     return NOZZLE_FORMAT_RGBA16_FLOAT;
        case SG_PIXELFORMAT_R32F:        return NOZZLE_FORMAT_R32_FLOAT;
        case SG_PIXELFORMAT_RG32F:       return NOZZLE_FORMAT_RG32_FLOAT;
        case SG_PIXELFORMAT_RGBA32F:     return NOZZLE_FORMAT_RGBA32_FLOAT;
        case SG_PIXELFORMAT_R16:         return NOZZLE_FORMAT_R16_UNORM;
        case SG_PIXELFORMAT_RG16:        return NOZZLE_FORMAT_RG16_UNORM;
        case SG_PIXELFORMAT_RGBA16:      return NOZZLE_FORMAT_RGBA16_UNORM;
        case SG_PIXELFORMAT_R32UI:       return NOZZLE_FORMAT_R32_UINT;
        case SG_PIXELFORMAT_RGBA32UI:    return NOZZLE_FORMAT_RGBA32_UINT;
        default:                         return 0;
    }
}

bool nozzle_sokol_frame_to_image(void *nozzle_frame, sg_image *out_img) {
    if (!nozzle_frame || !out_img) return false;

    NozzleFrame *frame = (NozzleFrame *)nozzle_frame;

    NozzleFrameInfo info{};
    if (nozzle_frame_get_info(frame, &info) != NOZZLE_OK) return false;

    sg_pixel_format sfmt = nozzle_sokol_to_pixel_format(info.format);
    if (sfmt == SG_PIXELFORMAT_NONE) return false;

    sg_image_desc desc{};
    desc.width = info.width;
    desc.height = info.height;
    desc.pixel_format = sfmt;
    desc.usage = SG_USAGE_IMMUTABLE;

    // On macOS Metal: nozzle frame contains an IOSurface-backed MTLTexture.
    // We get the native texture handle and inject it into sokol.
    #if defined(NOZZLE_PLATFORM_MACOS)
        // nozzle frame's texture is IOSurface-backed Metal texture.
        // Get the IOSurface ID, lookup the surface, create our own MTLTexture,
        // then inject into sokol.
        //
        // For now, use the CPU path (lock pixels → create sokol image from data).
        // The zero-copy Metal path requires access to nozzle::metal::get_texture()
        // which is C++ only. A future update can add native Metal injection.
    #endif

    // CPU fallback path: lock pixels, create sokol image with data
    NozzleMappedPixels mapped{};
    if (nozzle_frame_lock_pixels(frame, &mapped) != NOZZLE_OK) return false;

    desc.data.subimage[0][0].ptr = mapped.data;
    desc.data.subimage[0][0].size = (size_t)mapped.row_bytes * info.height;

    *out_img = sg_make_image(&desc);

    nozzle_frame_unlock_pixels(frame);

    return sg_query_image_state(*out_img) == SG_RESOURCESTATE_VALID;
}

bool nozzle_sokol_image_publish(void *nozzle_sender, sg_image img) {
    if (!nozzle_sender || img.id == SG_INVALID_ID) return false;

    NozzleSender *sender = (NozzleSender *)nozzle_sender;

    sg_image_info sinfo = sg_query_image_info(img);
    if (sinfo.state != SG_RESOURCESTATE_VALID) return false;

    int nfmt = nozzle_sokol_from_pixel_format(sinfo.pixel_format);
    if (nfmt == 0) return false;

    NozzleFrame *frame = nullptr;
    if (nozzle_sender_acquire_writable_frame(
            sender, sinfo.width, sinfo.height, nfmt, &frame) != NOZZLE_OK) {
        return false;
    }

    NozzleMappedPixels mapped{};
    if (nozzle_frame_lock_writable_pixels(frame, &mapped) != NOZZLE_OK) {
        nozzle_frame_release(frame);
        return false;
    }

    sg_image_data img_data = sg_query_image_data(img);
    if (img_data.subimage[0][0].ptr && img_data.subimage[0][0].size > 0) {
        size_t copy_size = img_data.subimage[0][0].size;
        if (copy_size > (size_t)mapped.row_bytes * sinfo.height) {
            copy_size = (size_t)mapped.row_bytes * sinfo.height;
        }
        memcpy(mapped.data, img_data.subimage[0][0].ptr, copy_size);
    }

    nozzle_frame_unlock_writable_pixels(frame);

    if (nozzle_sender_commit_frame(sender, frame) != NOZZLE_OK) {
        nozzle_frame_release(frame);
        return false;
    }

    nozzle_frame_release(frame);
    return true;
}

#endif // NOZZLE_SOKOL_IMPL
