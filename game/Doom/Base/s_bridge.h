/*
 * SPDX-License-Identifier: ACSL-1.4 OR FAFOL-0.1 OR Hippocratic-3.0
 * Multi-licensed under ACSL-1.4, FAFOL-0.1, and Hippocratic-3.0
 * See LICENSE.txt for full license texts
 */

#pragma once

/*
   ===================================
   S O U N D B R I D G E
   REAPER AUDIO ARCHITECTURE
   ELASTIC SOFTWORKS 2025
   ===================================
*/

/*
                    --- MODULE ETHOS ---

        the SoundBridge is the adapter layer between modern
        audio formats (WAV, OGG) and the PSX SPU hardware.
        
        it preserves the signature "wobbly" PSX sound by
        converting PCM audio to SPU-compatible ADPCM at
        runtime, while maintaining hardware effects like
        pitch shifting, reverb, and envelope control.
        
        this module allows REAPER to use custom assets
        without requiring external conversion tools or
        hardcoded asset pipelines.
*/

#include <cstdint>
#include <memory>
#include <vector>
#include <string>

/* 
    ==================================
             --- CONSTANTS ---
    ==================================
*/

/* 
 * psx spu operates at 44.1khz sample rate,
 * matching the original cd-audio standard.
 */
static constexpr uint32_t SPU_SAMPLE_RATE = 44100;

/*
 * psx adpcm block format matches the hardware
 * expectations of the spu. each block encodes
 * 28 samples in 16 bytes.
 */
static constexpr uint32_t ADPCM_BLOCK_SIZE_BYTES = 16;
static constexpr uint32_t ADPCM_SAMPLES_PER_BLOCK = 28;

/*
 * maximum sound name length for metadata definitions.
 * kept short to match psx memory constraints.
 */
static constexpr uint32_t MAX_SOUND_NAME_LEN = 32;

/* 
    ==================================
          --- DATA STRUCTURES ---
    ==================================
*/

/*
 * sound metadata: defines properties for a single
 * audio asset that cannot be derived from the
 * waveform alone.
 * 
 * this allows game designers to control:
 *  - loop behavior for ambient sounds
 *  - pitch randomization for variety
 *  - reverb depth for spatial presence
 */
struct SoundMetadata 
{
    char        name[MAX_SOUND_NAME_LEN];   /* identifier matching sfxenum_t */
    uint32_t    loop_start_sample;          /* sample offset where loop begins (0 = no loop) */
    uint32_t    loop_end_sample;            /* sample offset where loop ends (0 = end of file) */
    int16_t     pitch_variance;             /* random pitch shift range in cents (+/- value) */
    uint8_t     reverb_depth;               /* override reverb amount (0-127, 255 = use map default) */
    uint8_t     priority;                   /* voice stealing priority (higher = more important) */
};

/*
 * pcm audio buffer: holds decoded audio data
 * in standard 16-bit signed mono format before
 * conversion to adpcm.
 */
struct PcmAudioBuffer
{
    std::vector<int16_t>    samples;        /* raw pcm samples at 44.1khz */
    uint32_t                sample_rate;    /* original sample rate (for resampling if needed) */
    uint8_t                 channels;       /* 1 = mono, 2 = stereo */
};

/*
 * adpcm audio buffer: holds encoded audio data
 * ready for upload to spu ram. includes the
 * header flags needed for loop control.
 */
struct AdpcmAudioBuffer
{
    std::vector<uint8_t>    blocks;         /* adpcm blocks (16 bytes each) */
    uint32_t                num_blocks;     /* total number of blocks */
    uint32_t                loop_start_block; /* which block contains loop start */
    uint32_t                loop_end_block;   /* which block contains loop end */
};

/*
 * sound asset: complete in-memory representation
 * of a loaded sound, including both the waveform
 * data and its metadata properties.
 */
struct SoundAsset
{
    SoundMetadata           metadata;       /* designer-defined properties */
    PcmAudioBuffer          pcm_data;       /* decoded pcm (cached for debug) */
    AdpcmAudioBuffer        adpcm_data;     /* encoded spu format (uploaded to ram) */
    uint32_t                spu_address;    /* where in spu ram this lives (0 = not uploaded) */
};

/* 
    ==================================
        --- SOUNDBRIDGE CLASS ---
    ==================================
*/

/*
 * the soundbridge manages the complete audio
 * asset pipeline:
 * 
 *  1. loads wav/ogg files from disk
 *  2. decodes to pcm format
 *  3. resamples to 44.1khz if needed
 *  4. converts stereo to mono (spu limitation)
 *  5. encodes to psx adpcm format
 *  6. applies loop points from metadata
 *  7. uploads to spu ram
 *  8. integrates with wess sound driver
 */
class SoundBridge
{
public:
    /* 
     * ===============================
     *     INITIALIZATION
     * ===============================
     */
    
    /*
     * constructs the bridge and loads the metadata
     * definition file (e.g. "sounds.def") which
     * contains loop points and effect parameters
     * for all custom sounds.
     */
    SoundBridge() noexcept;
    ~SoundBridge() noexcept;
    
    /*
     * loads the sound metadata definition file.
     * this is a simple text format:
     * 
     * [sfx_pistol]
     * loop_start = 0
     * loop_end = 0
     * pitch_variance = 50
     * reverb_depth = 255
     * priority = 10
     * 
     * returns true on success, false on error.
     */
    bool load_metadata(const char* def_file_path) noexcept;
    
    /* 
     * ===============================
     *     ASSET LOADING
     * ===============================
     */
    
    /*
     * loads a wav file from the filesystem and
     * stores it as a soundasset ready for use.
     * 
     * the sound is automatically:
     *  - decoded to pcm
     *  - resampled to 44.1khz if needed
     *  - converted to mono if stereo
     *  - encoded to adpcm
     * 
     * returns true on success, false on error.
     */
    bool load_wav_sound(
        const char* file_path,
        const char* sound_name
    ) noexcept;
    
    /*
     * loads an ogg vorbis file (future extension).
     * same processing pipeline as wav loading.
     */
    bool load_ogg_sound(
        const char* file_path,
        const char* sound_name
    ) noexcept;
    
    /* 
     * ===============================
     *     SPU INTEGRATION
     * ===============================
     */
    
    /*
     * uploads all loaded sounds to spu ram starting
     * at the specified address. this must be called
     * after loading sounds and before playing them.
     * 
     * returns the total number of bytes uploaded.
     */
    uint32_t upload_to_spu(uint32_t start_address) noexcept;
    
    /*
     * retrieves the spu address for a named sound.
     * used by the wess driver to locate samples.
     * 
     * returns 0 if sound not found or not uploaded.
     */
    uint32_t get_spu_address(const char* sound_name) const noexcept;
    
    /*
     * retrieves the metadata for a named sound.
     * allows the game to query loop points, pitch
     * variance, etc. for dynamic behavior.
     * 
     * returns nullptr if sound not found.
     */
    const SoundMetadata* get_metadata(const char* sound_name) const noexcept;
    
    /* 
     * ===============================
     *     ENCODING PIPELINE
     * ===============================
     */
    
    /*
     * encodes pcm audio to psx adpcm format.
     * this is the core conversion function that
     * makes the bridge work.
     * 
     * the adpcm encoding uses the same algorithm
     * as the psx hardware, preserving loop points
     * and applying the proper block flags.
     */
    bool encode_pcm_to_adpcm(
        const PcmAudioBuffer& pcm,
        AdpcmAudioBuffer& adpcm,
        uint32_t loop_start_sample,
        uint32_t loop_end_sample
    ) noexcept;
    
    /* 
     * ===============================
     *     UTILITY FUNCTIONS
     * ===============================
     */
    
    /*
     * resamples pcm audio to 44.1khz using linear
     * interpolation. this ensures all sounds match
     * the spu's native sample rate.
     */
    static bool resample_to_44100(
        PcmAudioBuffer& buffer
    ) noexcept;
    
    /*
     * converts stereo pcm to mono by averaging
     * left and right channels. the psx spu only
     * supports mono samples (stereo is done via
     * panning multiple voices).
     */
    static bool stereo_to_mono(
        PcmAudioBuffer& buffer
    ) noexcept;
    
private:
    /* internal sound asset storage */
    std::vector<SoundAsset> m_sounds;
    
    /* lookup table: sound name -> asset index */
    std::unordered_map<std::string, uint32_t> m_sound_lookup;
    
    /* adpcm encoder state (preserves filter history between blocks) */
    struct AdpcmEncoderState
    {
        int16_t prev_sample_1;      /* n-1 sample for adaptive filter */
        int16_t prev_sample_2;      /* n-2 sample for adaptive filter */
    };
    
    /*
     * internal helper: encodes a single adpcm block
     * from 28 pcm samples. this is where the magic
     * happens - matching the psx hardware algorithm.
     */
    void encode_adpcm_block(
        const int16_t* pcm_samples,
        uint8_t* adpcm_block,
        AdpcmEncoderState& state,
        uint8_t flags
    ) noexcept;
    
    /*
     * internal helper: finds the best adpcm encoding
     * parameters (shift and filter) for a block of
     * samples. minimizes encoding error.
     */
    void find_best_adpcm_params(
        const int16_t* pcm_samples,
        uint8_t& out_shift,
        uint8_t& out_filter,
        const AdpcmEncoderState& state
    ) noexcept;
};

/* 
    ==================================
          --- C INTERFACE ---
    ==================================
*/

/*
 * c-compatible api for integration with the
 * existing doom c codebase. the c++ soundbridge
 * is wrapped by these functions.
 */

#ifdef __cplusplus
extern "C" {
#endif

/*
 * initializes the global soundbridge instance.
 * must be called during sound system startup.
 */
void soundbridge_init(void);

/*
 * shuts down the soundbridge and frees resources.
 */
void soundbridge_shutdown(void);

/*
 * loads the metadata definition file.
 */
int soundbridge_load_metadata(const char* path);

/*
 * loads a wav sound file.
 */
int soundbridge_load_wav(const char* path, const char* name);

/*
 * uploads all sounds to spu ram.
 * returns bytes uploaded.
 */
uint32_t soundbridge_upload_to_spu(uint32_t start_addr);

/*
 * gets the spu address for a sound by name.
 */
uint32_t soundbridge_get_address(const char* name);

#ifdef __cplusplus
}
#endif

#endif /* S_BRIDGE_H */
