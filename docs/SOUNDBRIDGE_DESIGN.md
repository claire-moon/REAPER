# SoundBridge: REAPER Audio Architecture

## Overview

The **SoundBridge** is REAPER's solution for loading custom audio assets (WAV, OGG) while preserving the authentic PlayStation SPU sound characteristics. It acts as an adapter between modern audio formats and the PSX hardware emulation layer.

## Design Goals

1. **Asset Flexibility**: Support standard WAV and OGG Vorbis files instead of hardcoded VAG (PSX ADPCM) files
2. **Hardware Preservation**: Maintain all PSX SPU features (pitch shifting, reverb, ADSR envelopes)
3. **Runtime Conversion**: Encode PCM → ADPCM at runtime, eliminating external tools
4. **Designer Control**: Metadata sidecar files for loop points, pitch randomization, etc.
5. **Minimal Code Impact**: Integrate cleanly with existing WESS driver architecture

## Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                        REAPER SOUND FLOW                        │
└─────────────────────────────────────────────────────────────────┘

  ┌──────────┐      ┌──────────┐      ┌──────────┐
  │ WAV File │      │ OGG File │      │ sounds.  │
  │ (Custom) │      │ (Custom) │      │   def    │
  └─────┬────┘      └─────┬────┘      └────┬─────┘
        │                 │                 │
        └─────────┬───────┴─────────────────┘
                  ▼
        ┌─────────────────────┐
        │    SOUNDBRIDGE      │
        │  (C++ Adapter)      │
        └──────────┬──────────┘
                   │
        ┌──────────┴──────────┐
        │ 1. DECODE PCM       │  ← Standard audio decoding
        │ 2. RESAMPLE 44.1khz │  ← Match SPU rate
        │ 3. STEREO → MONO    │  ← SPU limitation
        │ 4. ENCODE ADPCM     │  ← PSX format
        │ 5. APPLY LOOP FLAGS │  ← From metadata
        └──────────┬──────────┘
                   ▼
        ┌─────────────────────┐
        │      SPU RAM        │
        │  (ADPCM Blocks)     │
        └──────────┬──────────┘
                   │
        ┌──────────┴──────────┐
        │                     │
        ▼                     ▼
  ┌──────────┐        ┌────────────┐
  │   WESS   │        │  PSX SPU   │
  │  Driver  │        │  Emulator  │
  └────┬─────┘        └──────┬─────┘
       │                     │
       └──────────┬──────────┘
                  ▼
           ┌──────────────┐
           │ Audio Output │
           │  (SDL/OS)    │
           └──────────────┘
```

## Component Breakdown

### 1. Asset Loading

**Input**: Standard audio files (WAV, OGG Vorbis)

**Process**:
- Load file from filesystem using standard libraries
- Decode to raw PCM (16-bit signed integers)
- Extract format metadata (sample rate, channels, bit depth)

**Implementation Notes**:
- WAV: Parse RIFF header, handle common formats (PCM, IEEE float)
- OGG: Use `stb_vorbis.h` single-header library (already used in some projects)
- Error handling for corrupted files

### 2. Sample Rate Conversion

**Goal**: Ensure all audio is 44,100 Hz (PSX CD-quality)

**Algorithm**: Linear Interpolation
```c
// Simple but effective for game audio
for (uint32_t i = 0; i < output_samples; i++) {
    float src_pos = (i * input_rate) / 44100.0f;
    uint32_t index = (uint32_t)src_pos;
    float frac = src_pos - index;
    
    output[i] = (int16_t)(
        input[index] * (1.0f - frac) +
        input[index + 1] * frac
    );
}
```

**Why Not Sinc?**: PSX audio was "lo-fi" by design. High-quality resampling would lose the vintage character.

### 3. Stereo to Mono Conversion

**PSX Limitation**: SPU voices are mono; stereo is achieved by panning two voices.

**Conversion**:
```c
// Average left and right channels
mono_sample = (left_sample + right_sample) / 2;
```

**Future Enhancement**: Optionally load stereo sounds as two separate mono assets with auto-panning.

### 4. ADPCM Encoding

**Core Technology**: PSX ADPCM is a 4-bit adaptive differential encoding.

#### Block Structure
Each ADPCM block = 16 bytes:
- Byte 0: `[shift:4][filter:3][reserved:1]`
- Byte 1: `[flags:8]` (loop markers)
- Bytes 2-15: `[28 4-bit samples packed]`

#### Encoding Algorithm

**Step 1: Choose Filter**
PSX ADPCM uses 5 filters (prediction filters):
```c
static const int16_t FILTER_COEF_POS[5] = { 0, 60, 115,  98, 122 };
static const int16_t FILTER_COEF_NEG[5] = { 0,  0, -52, -55, -60 };

predicted_sample = 
    (prev_1 * FILTER_COEF_POS[filter] + 
     prev_2 * FILTER_COEF_NEG[filter]) / 64;
```

**Step 2: Calculate Residual**
```c
residual = current_sample - predicted_sample;
```

**Step 3: Quantize to 4 bits**
```c
shift = find_best_shift(residual_block);  // 0-15
quantized = (residual >> shift) & 0x0F;   // 4-bit value
```

**Step 4: Pack into Block**
```c
adpcm_block[2 + i/2] = 
    (sample[i] & 0x0F) |       // Lower nibble
    ((sample[i+1] & 0x0F) << 4); // Upper nibble
```

#### Optimal Parameter Selection

The encoder tests all combinations of shift (0-15) and filter (0-4) to minimize error:

```c
void find_best_adpcm_params(
    const int16_t* pcm_samples,
    uint8_t& out_shift,
    uint8_t& out_filter
) {
    int32_t min_error = INT32_MAX;
    
    for (uint8_t filter = 0; filter < 5; filter++) {
        for (uint8_t shift = 0; shift < 16; shift++) {
            int32_t error = calculate_encoding_error(
                pcm_samples, shift, filter
            );
            
            if (error < min_error) {
                min_error = error;
                out_shift = shift;
                out_filter = filter;
            }
        }
    }
}
```

This brute-force approach is fast enough (80 tests per block) and ensures optimal quality.

### 5. Loop Point Handling

**Problem**: Loop points must align to ADPCM block boundaries (28 samples each).

**Solution**: Snap loop points to nearest block:
```c
loop_start_block = loop_start_sample / 28;
loop_end_block = (loop_end_sample + 27) / 28;  // Round up
```

**Flag Encoding**:
- `LOOP_START` (0x04): Set at loop_start_block
- `LOOP_END` (0x01): Set at loop_end_block
- `REPEAT` (0x02): Set at loop_end_block (for looping sounds)

```c
// Example: looping ambient sound
adpcm_blocks[loop_start_block][1] |= 0x04;  // LOOP_START
adpcm_blocks[loop_end_block][1] |= 0x03;    // LOOP_END | REPEAT
```

**Non-Looping Sounds**:
```c
adpcm_blocks[last_block][1] |= 0x01;  // LOOP_END only
// SPU will silence the voice (no REPEAT flag)
```

## Metadata System: sounds.def

### File Format

Simple INI-style text format for artist/designer control:

```ini
# REAPER Sound Definitions
# Format: [sound_name]
#         property = value

[sfx_pistol]
loop_start = 0          # Start of loop (sample offset, 0 = no loop)
loop_end = 0            # End of loop (sample offset, 0 = end of file)
pitch_variance = 50     # Random pitch shift ±50 cents
reverb_depth = 255      # Use map default reverb (0-127 = override)
priority = 10           # Voice stealing priority

[sfx_sawidl]
loop_start = 4410       # Loop after 0.1 seconds
loop_end = 0            # Loop to end
pitch_variance = 0      # No randomization (consistent engine sound)
reverb_depth = 64       # Half reverb depth
priority = 15

[music_01]
loop_start = 441000     # 10 seconds in
loop_end = 0
pitch_variance = 0
reverb_depth = 255
priority = 20           # Music has high priority
```

### Parser Implementation

```c
bool SoundBridge::load_metadata(const char* def_file_path) {
    FILE* f = fopen(def_file_path, "r");
    if (!f) return false;
    
    char line[256];
    char current_sound[MAX_SOUND_NAME_LEN] = {0};
    SoundMetadata current_meta = {};
    
    while (fgets(line, sizeof(line), f)) {
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\n') continue;
        
        // Section header: [sound_name]
        if (line[0] == '[') {
            // Save previous sound if any
            if (current_sound[0]) {
                save_metadata(current_sound, current_meta);
            }
            
            // Parse new sound name
            sscanf(line, "[%31[^]]]", current_sound);
            memset(&current_meta, 0, sizeof(current_meta));
            strncpy(current_meta.name, current_sound, MAX_SOUND_NAME_LEN);
            current_meta.reverb_depth = 255;  // Default = use map
        }
        
        // Property: key = value
        else {
            char key[64], value[64];
            if (sscanf(line, "%63s = %63s", key, value) == 2) {
                if (strcmp(key, "loop_start") == 0)
                    current_meta.loop_start_sample = atoi(value);
                else if (strcmp(key, "loop_end") == 0)
                    current_meta.loop_end_sample = atoi(value);
                else if (strcmp(key, "pitch_variance") == 0)
                    current_meta.pitch_variance = atoi(value);
                else if (strcmp(key, "reverb_depth") == 0)
                    current_meta.reverb_depth = atoi(value);
                else if (strcmp(key, "priority") == 0)
                    current_meta.priority = atoi(value);
            }
        }
    }
    
    // Save last sound
    if (current_sound[0]) {
        save_metadata(current_sound, current_meta);
    }
    
    fclose(f);
    return true;
}
```

## Integration with WESS Driver

### Replacing LCD Loader

**Old Flow** (Original Doom):
```
psxcd_read("MAP01.LCD")
  ↓
wess_dig_lcd_data_read(sector_data, spu_addr, &sample_block)
  ↓
Upload VAG samples to SPU RAM
  ↓
Update patch_sample addresses in WESS
```

**New Flow** (REAPER with SoundBridge):
```
soundbridge_load_wav("assets/sounds/pistol.wav", "sfx_pistol")
  ↓
soundbridge_encode_to_adpcm(pcm_data, adpcm_data, metadata)
  ↓
soundbridge_upload_to_spu(spu_start_addr)
  ↓
Update patch_sample addresses in WESS
```

### Modified s_sound.cpp Integration

```c
void S_LoadMapSoundAndMusic(const int32_t mapNum) {
    // ... existing setup code ...
    
    #if REAPER_SOUNDBRIDGE
        // Use SoundBridge instead of LCD loading
        uint32_t destSpuAddr = gSound_MapLcdSpuStartAddr;
        
        // Load all sounds for this map from metadata
        for (const char* sound_name : get_map_sounds(mapNum)) {
            char wav_path[128];
            sprintf(wav_path, "sounds/%s.wav", sound_name);
            soundbridge_load_wav(wav_path, sound_name);
        }
        
        // Upload to SPU
        destSpuAddr += soundbridge_upload_to_spu(destSpuAddr);
        
        // Update WESS patch table
        for (uint32_t i = 0; i < gMapSndBlock.num_samples; i++) {
            const char* sound_name = get_sound_name_for_patch(i);
            uint32_t spu_addr = soundbridge_get_address(sound_name);
            wess_dig_set_sample_position(i, spu_addr);
        }
    #else
        // Original LCD loading path
        destSpuAddr += wess_dig_lcd_load(lcdFileId, destSpuAddr, &gMapSndBlock, false);
    #endif
}
```

## Performance Considerations

### Encoding Speed

On modern hardware (2015+ CPUs):
- WAV decode: ~1ms per second of audio
- Resample: ~2ms per second of audio
- ADPCM encode: ~5ms per second of audio
- **Total**: ~8ms per second of audio

For a typical 30-second music track: **~240ms encoding time**

This is acceptable for level loading (which already takes 1-3 seconds).

### Memory Usage

**Per Sound**:
- WAV file: ~88 KB per second (16-bit, 44.1kHz)
- PCM buffer: Same as WAV (temporary)
- ADPCM buffer: ~22 KB per second (4:1 compression)

**Example**:
- 50 SFX @ 2 seconds each = 2.2 MB
- 1 music track @ 120 seconds = 2.6 MB
- **Total SPU RAM usage**: ~5 MB

PSX had 512 KB. REAPER can configure larger SPU RAM (PSYDOOM_LIMIT_REMOVING).

### Optimization Strategies

1. **Cache Encoded ADPCM**: Save .adpcm files next to .wav files for instant loading
2. **Thread Encoding**: Encode sounds on background thread during level load
3. **Stream Music**: Don't load entire music track; stream and encode on-the-fly
4. **Lazy Loading**: Only encode sounds when first requested

## Testing Strategy

### Unit Tests

1. **ADPCM Roundtrip**: Encode PCM → ADPCM → Decode → Verify error < threshold
2. **Loop Points**: Verify flags are set correctly at block boundaries
3. **Resampling**: Test 22050Hz → 44100Hz for accuracy
4. **Metadata Parsing**: Test all .def file edge cases

### Integration Tests

1. **SPU Upload**: Verify sounds play through emulated SPU
2. **WESS Compatibility**: Ensure patch_sample addresses work correctly
3. **Reverb/Pitch**: Test SPU effects still apply to encoded sounds

### Quality Assurance

1. **Listening Tests**: A/B compare original VAG vs. SoundBridge WAV
2. **Loop Smoothness**: Verify loops are seamless (no clicks)
3. **Memory Leaks**: Run Valgrind during level load/unload cycles

## Future Enhancements

### Phase 2: OGG Vorbis Support

Add `load_ogg_sound()` using `stb_vorbis.h`:
```c
int channels, sample_rate;
short* pcm_data = stb_vorbis_decode_filename(
    ogg_path, &channels, &sample_rate, &num_samples
);
// Feed into existing pipeline
```

### Phase 3: Streaming Audio

For long music tracks:
```c
class StreamingSound {
    FILE* wav_file;
    AdpcmEncoder encoder;
    CircularBuffer spu_buffer;
    
    void update() {
        if (spu_buffer.space_available() > BLOCK_SIZE) {
            read_next_pcm_block();
            encode_to_adpcm();
            upload_to_spu();
        }
    }
};
```

### Phase 4: Real-Time Effects

Apply DSP before encoding:
- Normalize volume
- Apply EQ/filters
- Dynamic range compression
- Fade in/out

## Conclusion

The SoundBridge provides REAPER with modern audio asset flexibility while preserving the authentic PSX sound experience. By encoding at runtime, we eliminate tool dependencies and give designers full control via simple text files.

The implementation is modular, testable, and maintains clean separation from the existing WESS/SPU architecture.
