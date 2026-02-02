#pragma once

#include "Macros.h"
#include "PsyDoom/WadFile.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

BEGIN_NAMESPACE(WadCompat)

//------------------------------------------------------------------------------------------------------------------------------------------
// Identifies the detected WAD format so the map loader can decide whether conversion is required.
//------------------------------------------------------------------------------------------------------------------------------------------
enum class WadFormat : uint8_t {
    Unknown,
    PC_Doom,
    PC_Hexen,
    PSX_Doom,
    PSX_FinalDoom
};

//------------------------------------------------------------------------------------------------------------------------------------------
// Summary of the WAD format detection outcome.
//------------------------------------------------------------------------------------------------------------------------------------------
struct WadFormatInfo {
    WadFormat format;
    bool      hasCompressedLumps;
    bool      usesFixedPointVertices;
    bool      usesTextureIndices;
    int32_t   numMaps;
};

//------------------------------------------------------------------------------------------------------------------------------------------
// Represents a converted lump buffer owned by the compatibility layer.
//------------------------------------------------------------------------------------------------------------------------------------------
struct ConvertedLump {
    std::unique_ptr<std::byte[]> data;
    int32_t                      size;
    bool                         ownsData;
};

//------------------------------------------------------------------------------------------------------------------------------------------
// Acts as the middleware for converting PC Doom WAD data into PSX-friendly structures on demand.
//------------------------------------------------------------------------------------------------------------------------------------------

struct TexturePatch {
    int16_t originX;
    int16_t originY;
    int16_t patchLumpIdx;
};

struct PCTextureDef {
    char                    name[8];
    int16_t                 width;
    int16_t                 height;
    std::vector<TexturePatch> patches;
};

class WadCompatibilityLayer {
public:

    WadCompatibilityLayer() noexcept;
    ~WadCompatibilityLayer() noexcept;

    WadCompatibilityLayer(const WadCompatibilityLayer&) = delete;
    WadCompatibilityLayer& operator = (const WadCompatibilityLayer&) = delete;

    //--------------------------------------------------------------------------------------------------------------------------------------
    // Format detection helpers
    //--------------------------------------------------------------------------------------------------------------------------------------

    static WadFormatInfo detectWadFormat(const WadFile& wadFile) noexcept;
    static WadFormat detectMapFormat(const WadFile& wadFile, const char* const mapName) noexcept;

    //--------------------------------------------------------------------------------------------------------------------------------------
    // Map conversion lifecycle
    //--------------------------------------------------------------------------------------------------------------------------------------

    void beginMapConversion(const WadFile& mapWad, const WadFormat sourceFormat) noexcept;

    int32_t getConvertedSize(const char* const lumpName, const int32_t sourceSize) const noexcept;

    int32_t convertMapLump(
        const char* const lumpName,
        const void* const pSourceData,
        const int32_t sourceSize,
        void* const pDestBuffer,
        const int32_t destBufferSize
    ) noexcept;

    void endMapConversion() noexcept;

    //--------------------------------------------------------------------------------------------------------------------------------------
    // Texture registry utilities (PC name -> PSX texture index)
    //--------------------------------------------------------------------------------------------------------------------------------------

    void buildTextureRegistry() noexcept;
    int32_t resolveTextureName(const char name[8]) const noexcept;

    void loadPCTextureDefinitions(const WadFile& mainWad) noexcept;
    void generateTexturePixels(const int32_t pcTextureIndex, uint8_t* const pPixels) noexcept;

    //--------------------------------------------------------------------------------------------------------------------------------------
    // Texture System (Phase 1C)
    //--------------------------------------------------------------------------------------------------------------------------------------
    
    // Loads PC texture definitions from TEXTURE1/2 and PNAMES
    // Populates internal storage and returns true if successful.
    // Uses global W_ functions to access loaded WADs.
    bool loadPCTextureDefinitions() noexcept;
    
    // Generates pixel data for a PC texture (composites patches)
    // Returns the size of data written.
    int32_t generateTexturePixels(const int32_t pcTextureIdx, void* const pDestBuffer) noexcept;

    inline int32_t getNumPCTextures() const noexcept {
        return (int32_t)mPCTextureDefs.size();
    }

    inline const PCTextureDef& getPCTextureDef(const int32_t idx) const noexcept {
        return mPCTextureDefs[idx];
    }

    //--------------------------------------------------------------------------------------------------------------------------------------
    // Accessors
    //--------------------------------------------------------------------------------------------------------------------------------------

    inline WadFormat getCurrentFormat() const noexcept {
        return mCurrentFormat;
    }

    inline bool needsConversion() const noexcept {
        return (mCurrentFormat == WadFormat::PC_Doom);
    }

    inline int32_t getTextureCount() const noexcept {
        return (int32_t)mPCTextures.size();
    }
    
    inline const char* getTextureName(const int32_t index) const noexcept {
        if (index >= 0 && index < (int32_t)mPCTextures.size()) {
            return mPCTextures[index].name;
        }
        return "?";
    }

    inline int32_t getTextureWidth(const int32_t index) const noexcept {
        if (index >= 0 && index < (int32_t)mPCTextures.size()) {
            return mPCTextures[index].width;
        }
        return 0;
    }
    
    inline int32_t getTextureHeight(const int32_t index) const noexcept {
        if (index >= 0 && index < (int32_t)mPCTextures.size()) {
            return mPCTextures[index].height;
        }
        return 0;
    }

private:

    void convertVertices_PCToPSX(const void* const pSource, void* const pDest, const int32_t count) noexcept;
    void convertSectors_PCToPSX(const void* const pSource, void* const pDest, const int32_t count) noexcept;
    void convertSidedefs_PCToPSX(const void* const pSource, void* const pDest, const int32_t count) noexcept;
    void convertLinedefs_PCToPSX(const void* const pSource, void* const pDest, const int32_t count) noexcept;
    void ConvertPCTexturesToPSX(const WadFile& wadFile) noexcept;

    struct TextureEntry {
        char    name[8];
        int32_t psxIndex;
    };

    std::vector<PCTextureDef>                 mPCTextures;
    std::vector<std::string>                  mPCPatchNames;
    const WadFile*                            mMainWad; // Reference to WAD for loading patches

    std::vector<TextureEntry>                 mTextureRegistry;
    WadFormat                                 mCurrentFormat;
    std::unordered_map<std::string, ConvertedLump> mConvertedLumps;

    // PC Texture Data
    std::vector<PCTextureDef>                 mPCTextureDefs;
    std::vector<int32_t>                      mPatchLumpIndices; // From PNAMES
};

//------------------------------------------------------------------------------------------------------------------------------------------
// Singleton accessor
//------------------------------------------------------------------------------------------------------------------------------------------
WadCompatibilityLayer& getCompatLayer() noexcept;

END_NAMESPACE(WadCompat)
