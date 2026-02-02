# REAPER

REAPER is a standalone first-person shooter built on the **PsyDoom** engine, a reverse-engineered port of PlayStation Doom. While PsyDoom faithfully recreates the original PSX Doom experience, REAPER is a completely new game featuring 100% custom assets including sprites, textures, music, and sound effects.

## Project Goals

REAPER embraces the technical limitations and aesthetic of PSX hardware to create an authentic retro experience:
- **Custom Content**: All-new sprites, textures, levels, and audio designed specifically for REAPER
- **PSX Aesthetic**: Authentic "wobbly" PSX look with hardware constraints (VRAM limits, geometry precision)
- **Modern Features**: Vulkan/Metal renderer supporting widescreen and high resolutions
- **Hybrid Architecture**: Classic C game logic (Doom-style) with modern C++ rendering backend

## Built on PsyDoom

REAPER is built on the [PsyDoom](https://github.com/BodbDearg/PsyDoom) engine, which reverse-engineers PlayStation Doom to run natively on modern systems. The engine code is derived directly from the original PlayStation machine code and has been converted from emulated MIPS instructions to structured C++ code.

The engine emulates PSX GPU (for the classic renderer) and SPU specifics to maintain authenticity, while also offering a modern Vulkan renderer. Special thanks to [Erick Vásquez García (Erick194)](https://github.com/Erick194) and the [PSXDOOM-RE](https://github.com/Erick194/PSXDOOM-RE) project, which helped accelerate PsyDoom's development.

## README contents:
- [Development Status](#Development-status)
- [Requirements](#Requirements)
- [How to build](#How-to-build)
- [Command line arguments](#Command-line-arguments)
- [Multiplayer/link-cable emulation](#Multiplayerlink-cable-emulation)

## Other documents
- [License](LICENSE)
- [PsyDoom Changelist](CHANGELIST.md) - History of the base engine
- [PsyDoom Contributors](CONTRIBUTORS.md) - Contributors to the base engine

## Development Status

**REAPER is currently in early development.** The project is focused on:
1. Asset creation and replacement (sprites, textures, audio)
2. Level design and gameplay mechanics
3. Engine modifications and enhancements specific to REAPER's vision

This repository is actively being developed. Check the [docs/TODO.TXT](docs/TODO.TXT) file for current tasks and progress.

## Requirements
- **Build Requirements**: CMake 3.13.4 or higher
- **Compiler Support**:
    - Windows: Visual Studio 2019 or later (64-bit)
    - macOS: Xcode 11 or later
    - Linux: GCC 8 or later (Debian 'Buster' based distros or later recommended)
- **Vulkan/Metal Support**:
    - Vulkan 1.0 capable GPU for Vulkan renderer (Windows/Linux)
    - Metal capable GPU for macOS
    - On macOS, you must download and install the Vulkan SDK to build with the Vulkan renderer enabled
- **Runtime Requirements**:
    - Windows: Latest Visual C++ redistributable ([download here](https://support.microsoft.com/en-us/help/2977003/the-latest-supported-visual-c-downloads))
    - macOS: Must manually allow the .app through Gatekeeper (code is not signed)
    - Linux: PulseAudio required for sound
    - For best audio quality, set your audio device's sample rate to 44.1 kHz (CD Quality)

## How to build
1. Ensure you have CMake 3.13.4 or higher installed
2. Clone this repository:
   ```bash
   git clone https://github.com/claire-moon/REAPER.git
   cd REAPER
   ```
3. Generate build files:
   ```bash
   mkdir build
   cd build
   cmake ..
   ```
4. Build the project:
   - **Windows**: Open the generated Visual Studio solution and build
   - **macOS**: Open the generated Xcode project and build
   - **Linux**: Run `make` in the build directory

**Note**: On macOS, you must download and install the Vulkan SDK before building if you want Vulkan renderer support.

## Command line arguments

REAPER supports various command line arguments inherited from the PsyDoom engine:

### General Arguments
- `-datadir <MY_DIRECTORY_PATH>` - Specify a custom data directory
- `-cue <CUE_FILE_PATH>` - Manually specify the .cue file to use (for PsyDoom compatibility during development)
- `-file <WAD_FILE_PATH>` - Add extra IWADs to the game
- `-nolauncher` - Skip showing the launcher on startup

### Gameplay Cheats & Options
- `-nomonsters` - Enable 'no monsters' cheat (similar to PC Doom)
- `-nmbossfixup` - Trigger boss-related special actions when using no monsters cheat
- `-pistolstart` - Force pistol starts on all levels
- `-turbo` - Enable turbo mode (2x movement/fire speed, 2x door/platform speed)

### Warp & Demo Options
- `-warp <MAP_NUMBER>` - Warp directly to a specified map on startup
- `-skill <SKILL_NUMBER>` - Specify skill level (0-4) when warping: 0='I am a Wimp', 4='Nightmare!'
- `-playdemo <DEMO_LUMP_FILE_PATH>` - Play a demo lump file and exit
- `-saveresult <RESULT_FILE_PATH>` - Save demo playback results to a .json file
- `-checkresult <RESULT_FILE_PATH>` - Verify demo playback matches expected result (returns 0 on success)
- `-record` - Record demos for each map played (saved as `DEMO_MAP??.LMP`)
- `-headless` - Run in headless mode (for demo playback only)

### Multiplayer Arguments
- `-server [LISTEN_PORT]` - Run as server (default ports: 666 on Windows/macOS, 1666 on Linux)
- `-client [SERVER_HOST_NAME_AND_PORT]` - Connect to server (e.g., `-client 192.168.0.2:12345`)

## Multiplayer/link-cable emulation

REAPER inherits PsyDoom's emulation of the original PSX 'Link Cable' multiplayer functionality over TCP:

- **Network Requirements**: VERY low latency connection required (Ethernet or very fast WiFi recommended)
    - The protocol is synchronous and not lag-tolerant
- **Setup**:
    - Player 1 acts as the 'server' and listens for connections
    - Player 2 acts as the 'client' and connects to the server
    - Default ports: `666` (Windows/macOS) or `1666` (Linux)
- **Compatibility**: Both server and client must run compatible versions
- **Game Rules**: Server decides all game rules and overrides client settings

---

## Credits & Acknowledgments

REAPER is built on the foundation of the **PsyDoom** project by [BodbDearg](https://github.com/BodbDearg). 

Special thanks to:
- **[Erick Vásquez García (Erick194)](https://github.com/Erick194)** - Creator of [PSXDOOM-RE](https://github.com/Erick194/PSXDOOM-RE), which was instrumental in PsyDoom's development
- **All PsyDoom contributors** - See [CONTRIBUTORS.md](CONTRIBUTORS.md) for the full list

For the complete history of the PsyDoom engine, see [CHANGELIST.md](CHANGELIST.md).

## License

See [LICENSE](LICENSE) for license information.
