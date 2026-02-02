# REAPER (PsyDoom Fork) - Copilot Instructions

You are an expert C/C++ coding assistant for **REAPER**, a standalone game built on the **PsyDoom** engine (a reverse-engineered PSX Doom port).

## Project Architecture
- **Engine Core:** Hybrid architecture. Game logic is classic C (Doom-style), while the rendering backend (Vulkan/Metal) and system layers are modern C++ and Objective-C.
- **Goal:** Creating a new game with 100% custom assets (sprites, textures, audio) using the PSX Doom tech limitations for aesthetic purposes.
- **Build System:** CMake.

## Coding Guidelines
1.  **Language Consistency:**
    - If editing `p_*.c` (Game Logic): Use ANSI C style. Maintain strict manual memory management (Z_Malloc) and fixed-point math (`fixed_t`).
    - If editing `r_*.cpp` (Renderer): Use modern C++ features (smart pointers, namespaces) but ensure ABI compatibility with the C core.
    - If editing `sys_apple.mm`: Use Objective-C++ conventions.

2.  **Asset Handling:**
    - We are replacing standard Doom WADs. When referencing asset loading, assume we are loading custom REAPER lumps, not original Doom assets.
    - Prioritize PSX hardware constraints (VRAM limits, geometry precision) to maintain the authentic "wobbly" PSX look.

3.  **Refactoring limits:**
    - Do **not** suggest refactoring the core Doom loop (Tick/Frame) into modern C++ patterns unless explicitly asked. It breaks determinism.
    - Preserve the original variable naming conventions (`mo->target`, `thinking_t`, etc.) to match the original Doom source documentation.

## Context Awareness
- Always scan the `@workspace` to understand the relationship between the `game/` folder (logic) and `vulkan_gl/` (renderer).