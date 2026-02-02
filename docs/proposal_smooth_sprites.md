# Smooth Sprite Scaling & Modern Retro Rendering Proposal

## 1. Problem Analysis: Jagged Aliasing
The "jagged aliasing" observed when sprites are close to the camera stems from two sources:
1.  **Integer Truncation (Geometry)**: In `r_things.cpp` (the original/classic render path), screen coordinates are truncated to integers (`int16_t`). This causes "stair-stepping" on sprite edges when magnified.
2.  **Nearest Neighbor Sampling (Texture)**: The renderer (both classic and Vulkan) uses Nearest Neighbor sampling. When a sprite is magnified (texel-to-pixel ratio > 1), this results in large, blocky pixels.

## 2. Current Architecture
- **Classic (`r_things.cpp`)**: Calculates integer start/end positions. Emulates PS1 `POLY_FT4` primitives which snap to the pixel grid.
- **Vulkan (`rv_sprites.cpp`)**: Uses floating-point world coordinates, resolving the Geometry aliasing. However, it still exhibits Texture aliasing because it shares the global `gSampler_draw`.
- **Sampler Constraint**: `VPipelines.cpp` initializes `gSampler_draw` with `unnormalizedCoordinates = true`. In Vulkan, **samplers using unnormalized coordinates MUST use `VK_FILTER_NEAREST`**. Linear filtering is strictly forbidden by the spec.

## 3. Proposal: "Modern Retro" Smooth Sprites

To achieve smooth sprite scaling (Bilinear Filtering) without breaking the retro aesthetic of the world, we propose the following pipeline modifications:

### Step A: Dual-Sampler Architecture
Create a new Sampler in `VPipelines` specifically for smooth sprites:
- `gSampler_SpriteLinear`
- `min/magFilter = VK_FILTER_LINEAR`
- `unnormalizedCoordinates = false` (Normalized UVs are required for Linear filtering)

### Step B: Variable UV Coordinate System
Modify `SpriteFrag` in `rv_sprites.cpp`:
- Add a flag or logic to determine if a sprite requires smoothing (e.g., `magnification_factor > 2.0` or global user preference).
- If smooth:
    - Convert UVs from [0..1024, 0..512] pixel range to [0..1] normalized range.
    - `u_norm = u_pixel / 1024.0f;`
    - `v_norm = v_pixel / 512.0f;`

### Step C: Rendering Pipeline Separation
Since the Main Draw Descriptor Set (`gpDescriptorSet`) binds the VRAM with the Nearest sampler, we cannot simply switch samplers mid-draw.
**Solution**:
1.  Create a second Descriptor Set `gpDescriptorSet_Smooth` that binds VRAM with `gSampler_SpriteLinear`.
2.  In `VDrawing::recordCmdBuffer`, sort draw commands or introduce a `SetDescriptorSet` command.
3.  When encountering a smooth sprite batch, bind `gpDescriptorSet_Smooth`.

### Step D: Implementation in `r_things.cpp` (Classic Path)
For the software/classic path, true bilinear filtering is too CPU intensive for the existing `DrawSpans` architecture. The recommendation is to rely on the Vulkan backend for this visual feature (as enabled by `PSYDOOM_VULKAN_RENDERER` being the default for "Modern" rendering).

## Conclusion
This approach allows "Mix and Match" rendering: crisp pixel-perfect walls (Retro) + smooth, non-blocky sprites when up close (Modern), preserving the 1998 "High-End" pseudo-3D aesthetic.
