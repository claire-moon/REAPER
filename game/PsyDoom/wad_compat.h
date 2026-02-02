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

    //--------------------------------------------------------------------------------------------------------------------------------------
    // Accessors
    //--------------------------------------------------------------------------------------------------------------------------------------

    inline WadFormat getCurrentFormat() const noexcept {
        return mCurrentFormat;
    }

    inline bool needsConversion() const noexcept {
        return (mCurrentFormat == WadFormat::PC_Doom);
    }

private:

    void convertVertices_PCToPSX(const void* const pSource, void* const pDest, const int32_t count) noexcept;
    void convertSectors_PCToPSX(const void* const pSource, void* const pDest, const int32_t count) noexcept;
    void convertSidedefs_PCToPSX(const void* const pSource, void* const pDest, const int32_t count) noexcept;
    void convertLinedefs_PCToPSX(const void* const pSource, void* const pDest, const int32_t count) noexcept;

    struct TextureEntry {
        char    name[8];
        int32_t psxIndex;
    };

    std::vector<TextureEntry>                 mTextureRegistry;
    WadFormat                                 mCurrentFormat;
    std::unordered_map<std::string, ConvertedLump> mConvertedLumps;
};

//------------------------------------------------------------------------------------------------------------------------------------------
// Singleton accessor
//------------------------------------------------------------------------------------------------------------------------------------------
WadCompatibilityLayer& getCompatLayer() noexcept;

END_NAMESPACE(WadCompat)
