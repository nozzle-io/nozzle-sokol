# nozzle-sokol

> This codebase is currently in its AI-slob prototyping phase: the code runs on momentum, vibes, and plausible intent.
> Proper debugging will be introduced once demand graduates from hypothetical to measurable.

[Nozzle](https://github.com/nozzle-io/nozzle) GPU texture sharing integration for [sokol_gfx](https://github.com/floooh/sokol).

Share GPU textures between processes using sokol's graphics API — no GPU vendor-specific extensions needed.

## Disclaimer / Notice

This library is currently a work in progress and contains many incomplete features and unverified implementations.
Although it may appear usable at first glance, it may not function correctly.

Please use it with the understanding that no guarantees are made regarding its behavior, and perform debugging, validation, and review as needed.
If you encounter problems, please do not become angry; instead, contributions in the form of Issues or Pull Requests would be greatly appreciated.

## Status

| Platform | Status |
|----------|--------|
| macOS (Metal) | CPU copy path (zero-copy planned) |
| Windows (D3D11) | CPU copy path (zero-copy planned) |
| Linux (OpenGL) | CPU copy path |

## Usage

Single header library. Include in ONE source file:

```c
#define NOZZLE_SOKOL_IMPL
#include "nozzle-sokol.h"
```

### Receiver: nozzle frame → sokol image

```c
NozzleFrame *frame;
NozzleAcquireDesc acq = {0};
if (nozzle_receiver_acquire_frame(receiver, &acq, &frame) == NOZZLE_OK) {
    sg_image img;
    if (nozzle_sokol_frame_to_image(frame, &img)) {
        // use img with sg_apply_bindings(), draw to screen, etc.
        // destroy when done: sg_destroy_image(img);
    }
    nozzle_frame_release(frame);
}
```

### Sender: sokol image → nozzle

```c
sg_image img = sg_make_image(&(sg_image_desc){
    .width = 512,
    .height = 512,
    .pixel_format = SG_PIXELFORMAT_RGBA8,
    .usage = SG_USAGE_STREAM,
});
// ... fill image data ...

nozzle_sokol_image_publish(sender, img);
```

### Format Conversion

```c
sg_pixel_format sfmt = nozzle_sokol_to_pixel_format(NOZZLE_FORMAT_RGBA8_UNORM);
int nfmt = nozzle_sokol_from_pixel_format(SG_PIXELFORMAT_RGBA32F);
```

## Format Support

| Nozzle Format | sokol Pixel Format |
|---|---|
| R8_UNORM | R8 |
| RG8_UNORM | RG8 |
| RGBA8_UNORM | RGBA8 |
| BGRA8_UNORM | BGRA8 |
| RGBA8_SRGB | SRGB8A8 |
| BGRA8_SRGB | BGRA8 (sokol has no BGRA sRGB) |
| R16_UNORM | R16 |
| RG16_UNORM | RG16 |
| RGBA16_UNORM | RGBA16 |
| R16_FLOAT | R16F |
| RG16_FLOAT | RG16F |
| RGBA16_FLOAT | RGBA16F |
| R32_FLOAT | R32F |
| RG32_FLOAT | RG32F |
| RGBA32_FLOAT | RGBA32F |
| R32_UINT | R32UI |
| RGBA32_UINT | RGBA32UI |

## Build

```bash
cmake -B build -DNOOZZLE_SOKOL_BUILD_EXAMPLES=ON
cmake --build build
```

Requires [nozzle](https://github.com/nozzle-io/nozzle) and [sokol](https://github.com/floooh/sokol) as submodules.

## Architecture

```
nozzle-sokol.h    Single header integration layer
├── Format conversion (nozzle ↔ sokol pixel formats)
├── Receiver: nozzle frame → sokol sg_image (CPU copy, zero-copy planned)
└── Sender: sokol sg_image → nozzle sender (CPU copy, zero-copy planned)
```

### Zero-Copy Path (Planned)

sokol supports [native texture injection](https://github.com/floooh/sokol/blob/master/sokol_gfx.h) via `sg_image_desc`:
- `.mtl_textures[]` — Metal `id<MTLTexture>` injection
- `.d3d11_texture` — D3D11 `ID3D11Texture2D*` injection
- `.gl_textures[]` — OpenGL `GLuint` injection

Nozzle exposes native handles via its backend headers (`nozzle/backends/metal.hpp`, etc.). A future update will inject these directly, eliminating the CPU copy.

## License

MIT

Third-party dependencies:

- [nozzle](https://github.com/nozzle-io/nozzle) — MIT
- [sokol](https://github.com/floooh/sokol) — zlib/libpng
