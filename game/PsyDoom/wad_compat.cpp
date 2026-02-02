#include "PsyDoom/wad_compat.h"
#include "Doom/Base/w_wad.h"
#include "Doom/Base/i_system.h" 
#include "Doom/Base/z_zone.h"
#include "Doom/Base/doomdata.h"
#include "baselib/Asserts.h"
#include "baselib/Macros.h"
#include "baselib/Endian.h"
#include <cstring>
#include <algorithm>
#include <vector>
#include <unordered_map>
#include <memory>

// Forward declaration of engine function to resolve texture names
extern "C" int R_TextureNumForName(const char* name);

BEGIN_NAMESPACE(WadCompat)

//------------------------------------------------------------------------------------------------------------------------------------------
// PC WAD Structure Definitions
//------------------------------------------------------------------------------------------------------------------------------------------
struct mapvertex_pc_t {
    int16_t x;
    int16_t y;
};

struct mapsector_pc_t {
    int16_t floorheight;
    int16_t ceilingheight;
    char    floorpic[8];
    char    ceilingpic[8];
    int16_t lightlevel;
    int16_t special;
    int16_t tag;
};

struct mapsidedef_pc_t {
    int16_t textureoffset;
    int16_t rowoffset;
    char    toptexture[8];
    char    bottomtexture[8];
    char    midtexture[8];
    int16_t sector;
};

struct maplinedef_pc_t {
    int16_t v1;
    int16_t v2;
    int16_t flags;
    int16_t special;
    int16_t tag;
    int16_t sidenum[2];
};

struct maptopatch_pc_t {
    int16_t originx;
    int16_t originy;
    int16_t patch;
    int16_t stepdir;
    int16_t colormap;
};

struct maptexture_pc_t {
    char     name[8];
    int32_t  masked;
    int16_t  width;
    int16_t  height;
    int32_t  columndirectory;
    uint16_t patchcount;
};

//------------------------------------------------------------------------------------------------------------------------------------------
// Global singleton instance
//------------------------------------------------------------------------------------------------------------------------------------------
static WadCompatibilityLayer gCompatLayer;

WadCompatibilityLayer& getCompatLayer() noexcept {
    return gCompatLayer;
}

WadCompatibilityLayer::WadCompatibilityLayer() noexcept
    : mCurrentFormat(WadFormat::Unknown)
    , mMainWad(nullptr)
{
}

WadCompatibilityLayer::~WadCompatibilityLayer() noexcept {
}

WadFormatInfo WadCompatibilityLayer::detectWadFormat(const WadFile& wadFile) noexcept {
    WadFormatInfo info = {};
    info.format = WadFormat::Unknown;
    info.numMaps = 0;

    // Strict detection: Look for standard PC Doom lumps
    bool hasPNames = (wadFile.findLump("PNAMES") >= 0);
    bool hasTexture1 = (wadFile.findLump("TEXTURE1") >= 0);
    
    // Check for E1M1 or MAP01
    bool hasMap01 = (wadFile.findLump("MAP01") >= 0);
    bool hasE1M1 = (wadFile.findLump("E1M1") >= 0);

    if (hasPNames && hasTexture1 && (hasMap01 || hasE1M1)) {
        info.format = WadFormat::PC_Doom;
    }
    
    return info;
}

WadFormat WadCompatibilityLayer::detectMapFormat(const WadFile& wadFile, const char* const mapName) noexcept {
    // If the WAD itself was detected as PC, assume all maps are PC
    if (gCompatLayer.mCurrentFormat == WadFormat::PC_Doom) {
        return WadFormat::PC_Doom;
    }
    return WadFormat::PSX_Doom;
}

void WadCompatibilityLayer::beginMapConversion(const WadFile& mapWad, const WadFormat sourceFormat) noexcept {
    mCurrentFormat = sourceFormat;
    mConvertedLumps.clear();
}

void WadCompatibilityLayer::endMapConversion() noexcept {
    // Nothing to cleanup yet
}

int32_t WadCompatibilityLayer::convertMapLump(
    const char* const lumpName,
    const void* const pSourceData,
    const int32_t sourceSize,
    void* const pDestBuffer,
    const int32_t destBufferSize
) noexcept {
    if (!needsConversion()) return 0;

    int32_t neededSize = 0;

    if (strnicmp(lumpName, "VERTEXES", 8) == 0) {
        int32_t count = sourceSize / sizeof(mapvertex_pc_t);
        neededSize = count * sizeof(mapvertex_t);
        
        if (pDestBuffer) {
            ASSERT(destBufferSize >= neededSize);
            convertVertices_PCToPSX(pSourceData, pDestBuffer, count);
        }
    }
    else if (strnicmp(lumpName, "SECTORS", 8) == 0) {
        int32_t count = sourceSize / sizeof(mapsector_pc_t);
        neededSize = count * sizeof(mapsector_t);
        
        if (pDestBuffer) {
            ASSERT(destBufferSize >= neededSize);
            convertSectors_PCToPSX(pSourceData, pDestBuffer, count);
        }
    }
    else if (strnicmp(lumpName, "SIDEDEFS", 8) == 0) {
        int32_t count = sourceSize / sizeof(mapsidedef_pc_t);
        neededSize = count * sizeof(mapsidedef_t);
        
        if (pDestBuffer) {
            ASSERT(destBufferSize >= neededSize);
            convertSidedefs_PCToPSX(pSourceData, pDestBuffer, count);
        }
    }
    else if (strnicmp(lumpName, "LINEDEFS", 8) == 0) {
        int32_t count = sourceSize / sizeof(maplinedef_pc_t);
        neededSize = count * sizeof(maplinedef_t);
        
        if (pDestBuffer) {
            ASSERT(destBufferSize >= neededSize);
            convertLinedefs_PCToPSX(pSourceData, pDestBuffer, count);
        }
    }

    return neededSize;
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Geometry Conversion Implementations
//------------------------------------------------------------------------------------------------------------------------------------------

void WadCompatibilityLayer::convertVertices_PCToPSX(const void* const pSource, void* const pDest, const int32_t count) noexcept {
    const mapvertex_pc_t* src = (const mapvertex_pc_t*)pSource;
    mapvertex_t* dst = (mapvertex_t*)pDest;

    for (int32_t i = 0; i < count; ++i) {
        dst[i].x = (fixed_t)little16(src[i].x) << FRACBITS;
        dst[i].y = (fixed_t)little16(src[i].y) << FRACBITS;
    }
}

void WadCompatibilityLayer::convertSectors_PCToPSX(const void* const pSource, void* const pDest, const int32_t count) noexcept {
    const mapsector_pc_t* src = (const mapsector_pc_t*)pSource;
    mapsector_t* dst = (mapsector_t*)pDest;

    for (int32_t i = 0; i < count; ++i) {
        dst[i].floorheight = little16(src[i].floorheight) << FRACBITS;
        dst[i].ceilingheight = little16(src[i].ceilingheight) << FRACBITS;
        
        // Flats: Need proper resolution. For now, 0.
        // TODO: Implement R_FlatNumForName or similar logic if flats are distinct from textures
        dst[i].floorpic = 0; 
        dst[i].ceilingpic = 0;

        dst[i].lightlevel = little16(src[i].lightlevel);
        dst[i].special = little16(src[i].special);
        dst[i].tag = little16(src[i].tag);
    }
}

void WadCompatibilityLayer::convertSidedefs_PCToPSX(const void* const pSource, void* const pDest, const int32_t count) noexcept {
    const mapsidedef_pc_t* src = (const mapsidedef_pc_t*)pSource;
    mapsidedef_t* dst = (mapsidedef_t*)pDest;

    for (int32_t i = 0; i < count; ++i) {
        dst[i].textureoffset = little16(src[i].textureoffset) << FRACBITS;
        dst[i].rowoffset = little16(src[i].rowoffset) << FRACBITS;
        
        dst[i].toptexture = resolveTextureName(src[i].toptexture);
        dst[i].bottomtexture = resolveTextureName(src[i].bottomtexture);
        dst[i].midtexture = resolveTextureName(src[i].midtexture);
        
        dst[i].sector = little16(src[i].sector);
    }
}

void WadCompatibilityLayer::convertLinedefs_PCToPSX(const void* const pSource, void* const pDest, const int32_t count) noexcept {
    const maplinedef_pc_t* src = (const maplinedef_pc_t*)pSource;
    maplinedef_t* dst = (maplinedef_t*)pDest;

    for (int32_t i = 0; i < count; ++i) {
        dst[i].v1 = little16(src[i].v1);
        dst[i].v2 = little16(src[i].v2);
        dst[i].flags = little16(src[i].flags);
        dst[i].special = little16(src[i].special);
        dst[i].tag = little16(src[i].tag);
        dst[i].sidenum[0] = little16(src[i].sidenum[0]);
        dst[i].sidenum[1] = little16(src[i].sidenum[1]);
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Texture Management
//------------------------------------------------------------------------------------------------------------------------------------------

void WadCompatibilityLayer::loadPCTextureDefinitions(const WadFile& mainWad) noexcept {
    mMainWad = &mainWad;
    mPCPatchNames.clear();
    mPCTextures.clear();

    // 1. Load PNAMES
    int32_t pnamesIdx = mainWad.findLump("PNAMES");
    if (pnamesIdx >= 0) {
        const char* data = (const char*)mainWad.getLumpData(pnamesIdx);
        int32_t count = little32(*(int32_t*)data);
        const char* pList = data + 4;
        
        for (int32_t i = 0; i < count; ++i) {
            char name[9] = {0};
            strncpy(name, pList + i * 8, 8);
            mPCPatchNames.emplace_back(name);
        }
    }

    // 2. Load TEXTURE1/TEXTURE2
    const char* textureLumps[] = { "TEXTURE1", "TEXTURE2" };
    
    for (const char* lumpName : textureLumps) {
        int32_t texIdx = mainWad.findLump(lumpName);
        if (texIdx < 0) continue;

        const char* data = (const char*)mainWad.getLumpData(texIdx);
        int32_t count = little32(*(int32_t*)data);
        const int32_t* offsets = (const int32_t*)(data + 4);

        for (int32_t i = 0; i < count; ++i) {
            int32_t offset = little32(offsets[i]);
            const maptexture_pc_t* pTex = (const maptexture_pc_t*)(data + offset);
            
            PCTextureDef def;
            strncpy(def.name, pTex->name, 8);
            def.masked = (little32(pTex->masked) > 0);
            def.width = little16(pTex->width);
            def.height = little16(pTex->height);
            def.columndirectory = 0; // Unused
            def.patchCount = little16(pTex->patchcount);

            const maptopatch_pc_t* pPatches = (const maptopatch_pc_t*)((const char*)pTex + sizeof(maptexture_pc_t));
            
            for (int k = 0; k < def.patchCount; ++k) {
                TexturePatch patch;
                patch.originX = little16(pPatches[k].originx);
                patch.originY = little16(pPatches[k].originy);
                patch.patchIndex = little16(pPatches[k].patch);
                patch.stepDir = little16(pPatches[k].stepdir);
                patch.colormap = little16(pPatches[k].colormap);
                def.patches.push_back(patch);
            }
            
            mPCTextures.push_back(def);
        }
    }
}

int32_t WadCompatibilityLayer::resolveTextureName(const char name[8]) const noexcept {
    // Delegate to engine's lookup. 
    // This assumes R_InitTextures has already run and registered our PC textures.
    // If not, this might return 0 ("-") or fail.
    char safeName[9] = {0};
    strncpy(safeName, name, 8);
    return R_TextureNumForName(safeName);
}

void WadCompatibilityLayer::buildTextureRegistry() noexcept {
    // Unused for now
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Texture Conversion
//------------------------------------------------------------------------------------------------------------------------------------------

namespace {
    // Transparent color index (Cyan/Mask in Doom palette usually maps to index 0 for transparency in PSX)
    constexpr uint8_t TEX_TRANSPARENT_IDX = 0;

    // Helper for caching patch locations to avoid repeated lookups and IO
    class TexturePatchCache {
    public:
        TexturePatchCache(const WadFile& wad) : mWad(wad) {
        }
        
        // Resolve all PNAME indices to Wad Lump Indices once
        void preloadPNames(const char* pnamesData, int32_t numPatches) {
            mPatchLumpIndices.resize(numPatches, -1);
            
            for (int32_t i = 0; i < numPatches; ++i) {
                char patchName[9] = { 0 };
                std::memcpy(patchName, pnamesData + (i * 8), 8);
                // No need to uppercase here if we assume PNAMES is consistent
                mPatchLumpIndices[i] = mWad.checkForLump(patchName);
            }
        }
        
        const uint8_t* getPatchData(int32_t patchIdx, int32_t& outSize) {
            if (patchIdx < 0 || patchIdx >= (int32_t)mPatchLumpIndices.size()) {
                outSize = 0;
                return nullptr;
            }
            
            const int32_t lumpIdx = mPatchLumpIndices[patchIdx];
            if (lumpIdx < 0) {
                outSize = 0;
                return nullptr;
            }
            
            // Check cache
            auto it = mDataCache.find(lumpIdx);
            if (it != mDataCache.end()) {
                outSize = (int32_t)it->second.size();
                return it->second.data();
            }
            
            // Load
            const int32_t size = mWad.getRawSize(lumpIdx);
            if (size <= 0) {
                outSize = 0;
                return nullptr;
            }

            std::vector<uint8_t>& buf = mDataCache[lumpIdx];
            buf.resize(size);
            mWad.readLump(lumpIdx, buf.data(), false);
            
            outSize = size;
            return buf.data();
        }

    private:
        const WadFile& mWad;
        std::vector<int32_t> mPatchLumpIndices;
        std::unordered_map<int32_t, std::vector<uint8_t>> mDataCache;
    };
}

void WadCompatibilityLayer::ConvertPCTexturesToPSX(const WadFile& wadFile) noexcept {
    // Only proceed if we are in PC conversion mode
    if (!needsConversion()) {
        return;
    }

    // Locate the required lumps
    const int32_t pnamesLumpIdx = wadFile.checkForLump("PNAMES");
    const int32_t tex1LumpIdx   = wadFile.checkForLump("TEXTURE1");

    if (pnamesLumpIdx < 0 || tex1LumpIdx < 0) {
        return; // Missing essential lumps
    }

    // 1. Read PNAMES lump
    const int32_t pnamesSize = wadFile.getRawSize(pnamesLumpIdx);
    std::vector<uint8_t> pnamesData(pnamesSize);
    wadFile.readLump(pnamesLumpIdx, pnamesData.data(), false);

    if (pnamesSize < 4) return;
    
    const int32_t numPatches = Endian::littleToHost(*(int32_t*)pnamesData.data());
    if (pnamesSize < 4 + (numPatches * 8)) return;
    
    // Initialize patch cache and resolution
    TexturePatchCache patchCache(wadFile);
    patchCache.preloadPNames((const char*)(pnamesData.data() + 4), numPatches);

    // 2. Read TEXTURE1 lump
    const int32_t tex1Size = wadFile.getRawSize(tex1LumpIdx);
    std::vector<uint8_t> tex1Data(tex1Size);
    wadFile.readLump(tex1LumpIdx, tex1Data.data(), false);

    if (tex1Size < 4) return;

    const int32_t numTextures = Endian::littleToHost(*(int32_t*)tex1Data.data());
    const int32_t* const pTexOffsets = (const int32_t*)(tex1Data.data() + 4);

    if ((tex1Size < 4 + (numTextures * 4))) return;

    // 3. Process each texture
    for (int32_t i = 0; i < numTextures; ++i) {
        const int32_t offset = Endian::littleToHost(pTexOffsets[i]);
        if (offset < 0 || offset + (int32_t)sizeof(maptexture_pc_t) > tex1Size) continue;

        const maptexture_pc_t* const pTex = (const maptexture_pc_t*)(tex1Data.data() + offset);
        
        const int16_t texWidth     = Endian::littleToHost(pTex->width);
        const int16_t texHeight    = Endian::littleToHost(pTex->height);
        const uint16_t patchCount  = Endian::littleToHost(pTex->patchcount);
        
        // Sanity check dimensions
        if (texWidth <= 0 || texHeight <= 0) continue;

        // Ensure we fit strictly within offset array bounds
        if (offset + sizeof(maptexture_pc_t) + (patchCount * sizeof(maptopatch_pc_t)) > (size_t)tex1Size) continue;

        // Allocate the destination lump: Header (8 bytes) + Pixels (W*H)
        const int32_t dataSize = texWidth * texHeight;
        const int32_t lumpSize = 8 + dataSize;
        
        ConvertedLump constLump;
        constLump.size = lumpSize;
        constLump.ownsData = true;
        constLump.data = std::make_unique<std::byte[]>(lumpSize);
        
        // Write the header (standard 8-byte PSX texture header format: X, Y, W, H)
        int16_t* pHeader = reinterpret_cast<int16_t*>(constLump.data.get());
        pHeader[0] = 0; // offset x
        pHeader[1] = 0; // offset y
        pHeader[2] = Endian::hostToLittle(texWidth);
        pHeader[3] = Endian::hostToLittle(texHeight);

        // Fill with transparent color (cyan/index 0)
        uint8_t* const pDstPixels = reinterpret_cast<uint8_t*>(constLump.data.get()) + 8;
        std::memset(pDstPixels, TEX_TRANSPARENT_IDX, dataSize);

        // Compose all patches onto the texture buffer
        const maptopatch_pc_t* const pPatches = (const maptopatch_pc_t*)((const uint8_t*)pTex + sizeof(maptexture_pc_t));

        for (int p = 0; p < patchCount; ++p) {
            const maptopatch_pc_t& patchDef = pPatches[p];
            const int16_t originX = Endian::littleToHost(patchDef.originx);
            const int16_t originY = Endian::littleToHost(patchDef.originy);
            const int16_t patchIdx = Endian::littleToHost(patchDef.patch);

            if (patchIdx < 0) continue;

            // Retrieve patch data from cache
            int32_t patchSize = 0;
            const uint8_t* pPatchData = patchCache.getPatchData(patchIdx, patchSize);
            
            if (!pPatchData || patchSize < 8) continue; // Minimum header

            const int16_t patchW = Endian::littleToHost(((const int16_t*)pPatchData)[0]);
            // const int16_t patchH = Endian::littleToHost(((const int16_t*)pPatchData)[1]); // Unused
            const int32_t* const pColOffsets = (const int32_t*)(pPatchData + 8);
            
            if (patchSize < 8 + (patchW * 4)) continue; // Check header + offsets size

            // Draw patch columns
            for (int col = 0; col < patchW; ++col) {
                const int drawX = originX + col;
                if (drawX < 0 || drawX >= texWidth) continue;

                const int32_t colOffset = Endian::littleToHost(pColOffsets[col]);
                if (colOffset < 0 || colOffset >= patchSize) continue;

                const uint8_t* pPost = pPatchData + colOffset;

                // Process posts
                while ((pPost - pPatchData) + 4 <= patchSize) { // Ensure at least header read
                    const uint8_t topDelta = pPost[0];
                    if (topDelta == 0xFF) break; // End of column

                    const uint8_t length = pPost[1];
                    const uint8_t* const pSrcData = pPost + 3; // +3 to skip Top, Length, Padding
                    
                    // Validation
                    if ((pSrcData - pPatchData) + length > patchSize) break; 

                    for (int py = 0; py < length; ++py) {
                        const int drawY = originY + topDelta + py;
                        if (drawY >= 0 && drawY < texHeight) {
                            pDstPixels[drawY * texWidth + drawX] = pSrcData[py];
                        }
                    }

                    // Advance to next post: top(1) + len(1) + pad(1) + data(len) + pad(1) = 4 + len
                    pPost += (length + 4);
                }
            }
        }

        // Store the completed texture in our lookup map
        // Names in TEXTURE1 are usually uppercase, ensure valid C string
        char storeName[9] = { 0 };
        std::memcpy(storeName, pTex->name, 8);
        
        // Manual toUpper
        for (int c = 0; c < 8; ++c) {
            if (storeName[c] >= 'a' && storeName[c] <= 'z') {
                storeName[c] -= 32;
            }
        }

        mConvertedLumps[storeName] = std::move(constLump);
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------
// Texture Generation
//------------------------------------------------------------------------------------------------------------------------------------------

void WadCompatibilityLayer::generateTexturePixels(const int32_t pcTextureIndex, uint8_t* const pPixels) noexcept {
    if (pcTextureIndex < 0 || pcTextureIndex >= (int32_t)mPCTextures.size()) return;
    if (!mMainWad) return;

    const PCTextureDef& texDef = mPCTextures[pcTextureIndex];
    int32_t width = texDef.width;
    int32_t height = texDef.height;

    // Clear to transparent (0)
    std::memset(pPixels, 0, width * height);

    for (const TexturePatch& patchRef : texDef.patches) {
        if (patchRef.patchIndex < 0 || patchRef.patchIndex >= (int32_t)mPCPatchNames.size()) continue;

        const std::string& patchName = mPCPatchNames[patchRef.patchIndex];
        int32_t lumpNum = mMainWad->findLump(patchName.c_str());
        if (lumpNum < 0) continue;

        const uint8_t* pPatchData = (const uint8_t*)mMainWad->getLumpData(lumpNum);
        
        int16_t pWidth = little16(*(int16_t*)&pPatchData[0]);
        int16_t pHeight = little16(*(int16_t*)&pPatchData[2]);
        // Offsets 4 (left) and 6 (top) in header are for sprite offsets, usually ignored for wall textures 
        // OR used to offset the texture? Doom wall textures usually align top-left of patch to (originX, originY).
        
        int32_t originX = patchRef.originX;
        int32_t originY = patchRef.originY;

        const int32_t* columnOffsets = (const int32_t*)(pPatchData + 8);

        for (int col = 0; col < pWidth; ++col) {
            int32_t texX = originX + col;
            if (texX < 0 || texX >= width) continue;

            int32_t colOffset = little32(columnOffsets[col]);
            const uint8_t* pPost = pPatchData + colOffset;

            while (true) {
                uint8_t topDelta = pPost[0];
                if (topDelta == 0xFF) break;

                uint8_t length = pPost[1];
                // pPost[2] is padding
                const uint8_t* pData = pPost + 3;

                for (int i = 0; i < length; ++i) {
                    int32_t texY = originY + topDelta + i;
                    if (texY >= 0 && texY < height) {
                         pPixels[texY * width + texX] = pData[i];
                    }
                }
                pPost += 3 + length + 1; // +1 for final padding byte
            }
        }
    }
}

END_NAMESPACE(WadCompat)
