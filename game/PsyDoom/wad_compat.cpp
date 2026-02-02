/*
    ===================================
    C - F O R M  ( C )
    C FORMATTING GUIDE
    ELASTIC SOFTWORKS 2025
    ===================================
*/

#include "wad_compat.h"
#include "WadUtils.h"
#include "Doom/Game/doomdata.h"
#include "Asserts.h"

#include <cstring>
#include <vector>

/* 
    ==================================
             --- TYPES ---
    ==================================
*/

/* PC Doom vertex format (16-bit signed integers) */
struct mapvertex_pc_t {
    int16_t x;
    int16_t y;
};

/* PC Doom sector format (uses 8-char names instead of indices/flags) */
struct mapsector_pc_t {
    int16_t floorheight;
    int16_t ceilingheight;
    char    floorpic[8];
    char    ceilingpic[8];
    int16_t lightlevel;
    int16_t special;
    int16_t tag;
};

/* PC Doom sidedef format (uses 8-char names) */
struct mapsidedef_pc_t {
    int16_t textureoffset;
    int16_t rowoffset;
    char    toptexture[8];
    char    bottomtexture[8];
    char    midtexture[8];
    int16_t sector;
};

/* PC Doom linedef format (uses 16-bit flags) */
/* Note: PSX linedef_t is very similar but flags differ slightly */
struct maplinedef_pc_t {
    int16_t v1;
    int16_t v2;
    int16_t flags;
    int16_t special;
    int16_t tag;
    int16_t sidenum[2];
};

BEGIN_NAMESPACE(WadCompat)

/* 
    ==================================
             --- GLOBAL ---
    ==================================
*/

static WadCompatibilityLayer gCompatLayer;

/* 
    ==================================
             --- FUNCS ---
    ==================================
*/

/*

         WadCompatibilityLayer()
           ---
           Constructor. Initializes the state to 'Unknown' format.

*/   

WadCompatibilityLayer::WadCompatibilityLayer() noexcept
    : mCurrentFormat(WadFormat::Unknown)
{
}

/*

         ~WadCompatibilityLayer()
           ---
           Destructor.

*/   

WadCompatibilityLayer::~WadCompatibilityLayer() noexcept
{
}

/*

         detectWadFormat()
           ---
           Analyzes a WAD file to determine if it is a PC Doom WAD, 
           PSX Doom WAD, or other format.
           
           This works by looking for specific map markers (MAP01 vs E1M1)
           and inspecting lump sizes (VERTEXES) to differentiate between
           16-bit integer coords (PC) and 16.16 fixed-point (PSX).

*/   

WadFormatInfo WadCompatibilityLayer::detectWadFormat(const WadFile& wadFile) noexcept {
    WadFormatInfo info = {};
    info.format = WadFormat::Unknown;

    /* Check for standard PC/PSX map markers */
    const int32_t map01Idx = wadFile.findLumpIdx(WadUtils::makeUppercaseLumpName("MAP01"));
    const int32_t e1m1Idx  = wadFile.findLumpIdx(WadUtils::makeUppercaseLumpName("E1M1"));
    
    int32_t mapLumpStart = -1;

    if (map01Idx >= 0) {
        mapLumpStart = map01Idx;
        info.numMaps = 1; /* At least one */
    } else if (e1m1Idx >= 0) {
        mapLumpStart = e1m1Idx;
        info.numMaps = 1;
        info.format  = WadFormat::PC_Doom; /* E1M1 is almost certainly PC */
    }

    if (mapLumpStart >= 0) {
        /* If we found a map marker, check the VERTEXES lump */
        /* to distinguish between PC (int16) and PSX (fixed_t) */
        
        const int32_t vertexIdx = wadFile.findLumpIdx(
            WadUtils::makeUppercaseLumpName("VERTEXES"), 
            mapLumpStart
        );
        
        if (vertexIdx >= 0) {
            const WadLump& lump = wadFile.getLump(vertexIdx);
            const int32_t rawSize = lump.uncompressedSize; // This field is valid even if not loaded? 
                                                           // Let's assume uncompressedSize is set properly for all lumps. 
                                                           // Wait, uncompressedSize is only valid if we know if it is compressed.
                                                           // Actually, in PsyDoom's WadFile, uncompressedSize is populated during open.
            
            /* Check alignment with struct sizes */
            if ((rawSize % sizeof(mapvertex_t)) == 0) {
                /* Fits PSX fixed-point format (8 bytes per vertex) */
                info.format = WadFormat::PSX_Doom;
                info.usesFixedPointVertices = true;
            } else if ((rawSize % sizeof(mapvertex_pc_t)) == 0) {
                /* Fits PC integer format (4 bytes per vertex) */
                info.format = WadFormat::PC_Doom;
                info.usesFixedPointVertices = false;
            }
        }
        
        /* If we defaulted to PSX, check for TEXTURE1/2 usage vs indices? */
        /* That's harder to do quickly. For now rely on vertex format. */
    }

    /* Check for compressed lumps (PSX specific bit) */
    /* Scan a few lumps to see if any have high bit set in name */
    for (int32_t i = 0; i < wadFile.getNumLumps(); i++) {
        WadLumpName name = wadFile.getLumpName(i);
        if ((uint8_t)name.chars[0] & 0x80) {
            info.hasCompressedLumps = true;
            break;
        }
    }

    return info;
}

/*

         detectMapFormat()
           ---
           Checks a specific map within a WAD to determine its format.
           (Currently just delegates to general WAD detection or defaults).

*/   

WadFormat WadCompatibilityLayer::detectMapFormat(const WadFile& wadFile, const char* const mapName) noexcept {
    /* TODO: Implement per-map detection if mixed WADs are supported */
    WadFormatInfo info = detectWadFormat(wadFile);
    return info.format;
}

/*

         beginMapConversion()
           ---
           Prepares the compatibility layer for converting a new map.
           Clears previous buffers and sets the active format.

*/   

void WadCompatibilityLayer::beginMapConversion(const WadFile& mapWad, const WadFormat sourceFormat) noexcept {
    /* Reset state */
    mCurrentFormat = sourceFormat;
    mConvertedLumps.clear();
    
    if (needsConversion()) {
        /* TODO: Pre-scan lumps or prepare texture registry here */
        buildTextureRegistry();
    }
}

/*

         getConvertedSize()
           ---
           Returns the size required for the lump in PSX format.

*/   

int32_t WadCompatibilityLayer::getConvertedSize(const char* const lumpName, const int32_t sourceSize) const noexcept {
    if (!needsConversion()) {
        return sourceSize;
    }

    /* Important: names in WadFile are UPPERCASE by default usually, but we check specific names */
    /* WadFile::getLumpName returns 8 chars. We should be careful about null termination if using strcmp */
    
    /* However, for standard lumps VERTEXES etc, length is checked. */
    if (std::strncmp(lumpName, "VERTEXES", 8) == 0) {
        /* PC vertices are 4 bytes, PSX are 8 bytes */
        const int32_t numVerts = sourceSize / sizeof(mapvertex_pc_t);
        return numVerts * sizeof(mapvertex_t);
    }

    if (std::strncmp(lumpName, "SECTORS", 7) == 0) {
        /* PC sectors are 26 bytes, PSX are 28 bytes */
        /* Note: comparing 7 chars effectively matches SECTORS */
        const int32_t numSectors = sourceSize / sizeof(mapsector_pc_t);
        return numSectors * sizeof(mapsector_t); // 28 bytes
    }
    
    return sourceSize;
}

/*

         convertMapLump()
           ---
           Main entry point for converting map data.
           If the lump needs conversion (based on name and format), it processes it.
           Otherwise, it does a raw copy.

*/   

int32_t WadCompatibilityLayer::convertMapLump(
    const char* const lumpName,
    const void* const pSourceData,
    const int32_t sourceSize,
    void* const pDestBuffer,
    const int32_t destBufferSize
) noexcept {
    
    if (!needsConversion()) {
        /* No conversion needed, just copy if destination is provided */
        if (pDestBuffer && pSourceData) {
            std::memcpy(pDestBuffer, pSourceData, sourceSize);
        }
        return sourceSize;
    }

    if (std::strncmp(lumpName, "VERTEXES", 8) == 0) {
        int32_t numVerts = sourceSize / sizeof(mapvertex_pc_t);
        int32_t neededSize = numVerts * sizeof(mapvertex_t);
        
        if (pDestBuffer) {
            ASSERT(destBufferSize >= neededSize);
            convertVertices_PCToPSX(pSourceData, pDestBuffer, numVerts);
        }
        return neededSize;
    }

    if (std::strncmp(lumpName, "SECTORS", 7) == 0) {
        int32_t numSectors = sourceSize / sizeof(mapsector_pc_t);
        int32_t neededSize = numSectors * sizeof(mapsector_t);

        if (pDestBuffer) {
            ASSERT(destBufferSize >= neededSize);
            convertSectors_PCToPSX(pSourceData, pDestBuffer, numSectors);
        }
        return neededSize;
    }
    
    if (std::strncmp(lumpName, "LINEDEFS", 8) == 0) {
        /* Linedefs are same size but flags need checking if strictly following struct */
        /* Currently using pass-through as sizes match */
        const int32_t numLines = sourceSize / sizeof(maplinedef_pc_t);
        const int32_t neededSize = sourceSize; // Same size

        if (pDestBuffer) {
            convertLinedefs_PCToPSX(pSourceData, pDestBuffer, numLines);
        }
        return neededSize;
    }

    if (std::strncmp(lumpName, "SIDEDEFS", 8) == 0) {
        /* Sidedefs are same size */
        const int32_t numSides = sourceSize / sizeof(mapsidedef_pc_t);
        const int32_t neededSize = sourceSize; // Same size

        if (pDestBuffer) {
            convertSidedefs_PCToPSX(pSourceData, pDestBuffer, numSides);
        }
        return neededSize;
    }

    /* Default: No conversion */
    if (pDestBuffer && pSourceData) {
        std::memcpy(pDestBuffer, pSourceData, sourceSize);
    }
    return sourceSize;
}

/*

         endMapConversion()
           ---
           Cleans up after map loading is complete.

*/   

void WadCompatibilityLayer::endMapConversion() noexcept {
    mConvertedLumps.clear();
    mCurrentFormat = WadFormat::Unknown;
}

/*

         buildTextureRegistry()
           ---
           Scans TEXTURE1/TEXTURE2 lumps to build a mapping from
           texture names to PSX-style indices.

*/   

void WadCompatibilityLayer::buildTextureRegistry() noexcept {
    /* TODO: Implement reading TEXTURE1/2 from the main WAD or map WAD */
}

/*

         resolveTextureName()
           ---
           Finds the PSX texture index for a given name.

*/   

int32_t WadCompatibilityLayer::resolveTextureName(const char name[8]) const noexcept {
    /* TODO: Implement lookup */
    return -1;
}

/*

         convertVertices_PCToPSX()
           ---
           Converts 16-bit integer PC vertices to 16.16 fixed-point PSX vertices.

*/   

void WadCompatibilityLayer::convertVertices_PCToPSX(const void* const pSource, void* const pDest, const int32_t count) noexcept {
    const mapvertex_pc_t* src = (const mapvertex_pc_t*)pSource;
    mapvertex_t* dst = (mapvertex_t*)pDest;
    
    for (int32_t i = 0; i < count; i++) {
        dst[i].x = d_int_to_fixed(src[i].x);
        dst[i].y = d_int_to_fixed(src[i].y);
    }
}

/*

         convertSectors_PCToPSX()
           ---
           Converts PC sectors to PSX format. Handles size difference and flags initialization.

*/   

void WadCompatibilityLayer::convertSectors_PCToPSX(const void* const pSource, void* const pDest, const int32_t count) noexcept {
    const mapsector_pc_t* src = (const mapsector_pc_t*)pSource;
    mapsector_t* dst = (mapsector_t*)pDest;
    
    for (int32_t i = 0; i < count; i++) {
        dst[i].floorheight   = src[i].floorheight;
        dst[i].ceilingheight = src[i].ceilingheight;
        
        std::memcpy(dst[i].floorpic, src[i].floorpic, 8);
        std::memcpy(dst[i].ceilingpic, src[i].ceilingpic, 8);
        
        /* PC: lightlevel is int16. PSX: lightlevel is uint8, colorid is uint8. */
        /* Since both are little endian and PC lightlevel is usually <= 255: */
        /* src->lightlevel = 0xYYXX (XX=level, YY=0). */
        /* dst->lightlevel = XX, dst->colorid = YY (0). */
        /* We can do this explicitly to be safe, or just copy the 16-bits. */
        /* Let's be explicit for safety. */
        
        dst[i].lightlevel = (uint8_t)(src[i].lightlevel & 0xFF);
        dst[i].colorid    = 0; /* Default color */
        
        dst[i].special = src[i].special;
        dst[i].tag     = src[i].tag;
        
        /* Initialize new PSX fields */
        dst[i].flags       = 0;
        dst[i].ceilColorid = 0; /* Use floor color (if PSYDOOM_MODS logic applies) / or just 0 */
    }
}

/*

         convertSidedefs_PCToPSX()
           ---
           Passthrough for sidedefs (structures match for now).

*/   

void WadCompatibilityLayer::convertSidedefs_PCToPSX(const void* const pSource, void* const pDest, const int32_t count) noexcept {
    /* PC and PSX (Doom format) sidedefs are identical in size and layout */
    /* 30 bytes: 2+2+8+8+8+2 */
    std::memcpy(pDest, pSource, count * sizeof(mapsidedef_pc_t));
}

/*

         convertLinedefs_PCToPSX()
           ---
           Passthrough for linedefs (flags mostly compatible).

*/   

void WadCompatibilityLayer::convertLinedefs_PCToPSX(const void* const pSource, void* const pDest, const int32_t count) noexcept {
    /* PC and PSX linedefs are identical in size and layout */
    /* 14 bytes */
    std::memcpy(pDest, pSource, count * sizeof(maplinedef_pc_t));
}

/*

         loadPCTextureDefinitions()
           ---
           Logic to load PC TEXTURE1/2 and PNAMES lumps.
           Populates `mPCTextureDefs` and `mPatchLumpIndices`.

*/   

bool WadCompatibilityLayer::loadPCTextureDefinitions() noexcept {
    /* 1. Read PNAMES */
    const int32_t pnamesLump = W_GetNumForName("PNAMES");
    if (pnamesLump < 0) return false;

    {
        /* Read PNAMES data */
        const int32_t size = W_LumpLength(pnamesLump);
        std::byte* pData = (std::byte*)Z_Malloc(size, PU_STATIC, nullptr);
        W_ReadLump(pnamesLump, pData, true);

        const int32_t numPatches = Endian::littleToHost(*(int32_t*)pData);
        const char* pNames = (const char*)(pData + 4);

        mPatchLumpIndices.clear();
        mPatchLumpIndices.reserve(numPatches);

        for (int32_t i = 0; i < numPatches; i++) {
            char name[9] = {0};
            std::memcpy(name, pNames + (i * 8), 8);
            WadUtils::toUpper(name);
            
            const int32_t idx = W_GetNumForName(name);
            mPatchLumpIndices.push_back(idx);
        }

        Z_Free(pData);
    }

    /* 2. Read TEXTURE1 (and TEXTURE2 if exists) */
    const char* texLumps[] = { "TEXTURE1", "TEXTURE2", nullptr };
    
    for (int t = 0; texLumps[t]; t++) {
        const int32_t texLumpIdx = W_CheckNumForName(texLumps[t]);
        if (texLumpIdx < 0) continue;

        const int32_t size = W_LumpLength(texLumpIdx);
        std::byte* pData = (std::byte*)Z_Malloc(size, PU_STATIC, nullptr);
        W_ReadLump(texLumpIdx, pData, true);

        const int32_t numTextures = Endian::littleToHost(*(int32_t*)pData);
        const int32_t* pOffsets = (const int32_t*)(pData + 4);

        for (int32_t i = 0; i < numTextures; i++) {
            const int32_t offset = Endian::littleToHost(pOffsets[i]);
            if (offset < 0 || offset >= size) continue;

            const maptexture_pc_t* pTex = (const maptexture_pc_t*)(pData + offset);
            
            PCTextureDef def;
            std::memcpy(def.name, pTex->name, 8);
            def.width = Endian::littleToHost(pTex->width);
            def.height = Endian::littleToHost(pTex->height);
            
            const int16_t patchCount = Endian::littleToHost(pTex->patchcount);
            def.patches.reserve(patchCount);

            for (int p = 0; p < patchCount; p++) {
                const auto& srcPatch = pTex->patches[p];
                TexturePatch patch;
                patch.originX = Endian::littleToHost(srcPatch.originx);
                patch.originY = Endian::littleToHost(srcPatch.originy);
                
                const int16_t patchIdx = Endian::littleToHost(srcPatch.patch);
                if (patchIdx >= 0 && patchIdx < (int32_t)mPatchLumpIndices.size()) {
                    patch.patchLumpIdx = mPatchLumpIndices[patchIdx];
                } else {
                    patch.patchLumpIdx = -1;
                }
                
                def.patches.push_back(patch);
            }
            
            mPCTextureDefs.push_back(def);
        }

        Z_Free(pData);
    }

    return !mPCTextureDefs.empty();
}

/*

         generateTexturePixels()
           ---
           Composites patches into a pixel buffer.

*/   

int32_t WadCompatibilityLayer::generateTexturePixels(const int32_t pcTextureIdx, void* const pDestBuffer) noexcept {
    if (pcTextureIdx < 0 || pcTextureIdx >= (int32_t)mPCTextureDefs.size()) return 0;

    const PCTextureDef& def = mPCTextureDefs[pcTextureIdx];
    
    // PSX Header
    struct texlump_header_t {
         int16_t     offsetX;
         int16_t     offsetY;
         int16_t     width;
         int16_t     height;
    } header;

    header.offsetX = 0;
    header.offsetY = 0;
    header.width = def.width;
    header.height = def.height;
    
    // Write header
    std::memcpy(pDestBuffer, &header, sizeof(header));
    
    uint8_t* pPixels = (uint8_t*)pDestBuffer + sizeof(header);
    std::memset(pPixels, 0, def.width * def.height); // Fill with index 0 (transparency usually)

    // TODO: Implement actual patch composition (column to row conversion)
    // For now, return blank/black texture to prevent crash
    
    return sizeof(header) + (def.width * def.height);
}

/*

         getCompatLayer()
           ---
           Returns the global singleton instance.

*/   

WadCompatibilityLayer& getCompatLayer() noexcept {
    return gCompatLayer;
}

END_NAMESPACE(WadCompat)

/* 
    ==================================
             --- EOF ---
    ==================================
*/
