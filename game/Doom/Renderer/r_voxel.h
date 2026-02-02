#pragma once

#include "Doom/doomtype.h"
#include "Doom/Renderer/r_local.h" // For fixed_t, mobj_t, etc.

//------------------------------------------------------------------------------------------------------------------------------------------
// Voxel Model Data Structure
// Designed to be loaded as a single lump via Z_Malloc.
//
// To hook into R_DrawSubsectorSprites:
// 1. Check if mobj_t->flags has MF_VOXEL set (or similar logic).
// 2. If so, call R_ProjectVoxel(thing) instead of the standard sprite projection.
// 3. R_ProjectVoxel creates a visvoxel_t (or reuses vissprite_t with a flag) and adds it to the sort list.
// 4. During the draw loop, check if the sorted item is a voxel, and call R_DrawVisVoxel.
//------------------------------------------------------------------------------------------------------------------------------------------

typedef struct {
    int32_t     width;          // x axis size
    int32_t     height;         // z axis size (up/down in Doom is usually height, but Voxels often use z as height)
    int32_t     depth;          // y axis size
    
    int32_t     pivotType;      // 0 = center, 1 = bottom-center, etc.
    fixed_t     offsetX;        // Pivot offset X (fixed_t)
    fixed_t     offsetY;        // Pivot offset Y (fixed_t)
    fixed_t     offsetZ;        // Pivot offset Z (fixed_t)

    // Palette data or remap table index could go here if voxels have own palettes
    // ...

    // Raw Voxel Data follows immediately after this struct in memory.
    // Format: Slab6 / KVX style run-length encoded columns for efficiency.
    // Data is accessed as: voxel_t* v = (voxel_t*)Z_Malloc(...); uint8_t* data = v->data;
    uint8_t     data[];     
} voxel_t;

//------------------------------------------------------------------------------------------------------------------------------------------
// Runtime Voxel Visibility Structure
// Analogous to vissprite_t, but carries 3D rotation data.
// This structure needs to be compatible with the sorting mechanism in r_things.cpp.
// Existing vissprite_t has: viewx, scale, thing, next.
// We might need to extend vissprite_t or cast between types if we want to share the sort list.
//------------------------------------------------------------------------------------------------------------------------------------------
typedef struct visvoxel_s {
    struct visvoxel_s* next;    // Must match vissprite_t::next layout for shared sorting if possible
    
    int32_t         viewx;      // Viewspace x position (Must match vissprite_t::viewx)
    fixed_t         scale;      // Scale due to perspective (Must match vissprite_t::scale)
    mobj_t*         thing;      // The thing (Must match vissprite_t::thing)

    // Voxel specific fields below
    fixed_t         gy;         // World coordinates
    fixed_t         gz;         // World Z (height)
    fixed_t         gz_top;
    
    angle_t         angle;      // View angle relative to the object
    
    voxel_t*        model;      // Pointer to the cached voxel data
    int32_t         colormap;   // Lighting level index
} visvoxel_t;


//------------------------------------------------------------------------------------------------------------------------------------------
// Public Interface
//------------------------------------------------------------------------------------------------------------------------------------------

// Called during R_Init to set up lookup tables for voxel rendering
void R_InitVoxels(void);

// Called by R_DrawSubsectorSprites in r_things.cpp when a voxel flag is detected
void R_ProjectVoxel(mobj_t* thing);

// Adds a visible voxel to the draw list (called by R_ProjectVoxel)
void R_AddVisVoxel(visvoxel_t* vis);

// Called during the standard draw phase (sorted with sprites)
void R_DrawVisVoxel(visvoxel_t* vis);

// Memory management
voxel_t* R_GetVoxelForLump(int32_t lumpIdx);

// Debug / Utilities
void R_VoxelPrecache(int32_t lumpIdx);
