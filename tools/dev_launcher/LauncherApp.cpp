#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Text_Buffer.H>

#include "ProcessUtilities.h"
#include "baselib/FileUtils.h"
#include "baselib/JsonUtils.h"

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>

#include <thread>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <regex>

namespace fs = std::filesystem;

class LauncherWindow : public Fl_Window {
    Fl_Text_Buffer* log_buff;
    Fl_Text_Display* log_disp;
    Fl_Button* btn_build;
    Fl_Button* btn_launch;
    Fl_Input* input_warp;
    Fl_Check_Button* chk_nomonsters;
    Fl_Check_Button* chk_pistolstart;
    Fl_Check_Button* chk_devparm;
    
    std::string project_root;
    std::string build_dir;
    std::string exe_path;
    const std::string config_file = "launcher_config.json";
    
    std::atomic<bool> is_building;

public:
    LauncherWindow() : Fl_Window(600, 450, "PsyDoom Dev Launcher") {
        is_building = false;
        
        // --- Setup UI ---
        int y = 20;
        
        // Header
        Fl_Box* title = new Fl_Box(20, y, w()-40, 30, "Developer Tools");
        title->labelfont(FL_BOLD);
        title->labelsize(20);
        title->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
        y += 40;

        // Build Section
        Fl_Group* build_grp = new Fl_Group(20, y, w()-40, 60, "Build Control");
        build_grp->box(FL_ENGRAVED_BOX);
        btn_build = new Fl_Button(30, y+10, 120, 40, "Build Game");
        btn_build->callback(BuildCB, this);
        
        Fl_Button* btn_clean = new Fl_Button(160, y+10, 120, 40, "Clean && Build");
        btn_clean->callback(CleanCB, this);
        build_grp->end();
        y += 70;

        // Launch Options
        Fl_Group* launch_grp = new Fl_Group(20, y, w()-40, 100, "Launch Options");
        launch_grp->box(FL_ENGRAVED_BOX);
        
        int ly = y + 10;
        input_warp = new Fl_Input(80, ly, 60, 25, "Warp:");
        input_warp->value("01");
        
        chk_nomonsters  = new Fl_Check_Button(160, ly, 100, 25, "No Monsters");
        chk_pistolstart = new Fl_Check_Button(270, ly, 100, 25, "Pistol Start");
        chk_devparm     = new Fl_Check_Button(380, ly, 100, 25, "-devparm");
        chk_devparm->value(1); // Default on

        ly += 35;
        btn_launch = new Fl_Button(30, ly, w()-60, 40, "LAUNCH GAME");
        btn_launch->color(FL_GREEN);
        btn_launch->callback(LaunchCB, this);
        launch_grp->end();
        y += 110;

        // Log Output
        log_buff = new Fl_Text_Buffer();
        log_disp = new Fl_Text_Display(20, y, w()-40, h()-y-20);
        log_disp->buffer(log_buff);
        log_disp->textfont(FL_COURIER);
        log_disp->textsize(12);

        // --- Init Logic ---
        FindPaths();
        Log("Launcher started. Root: " + project_root);
        Log("Build Dir: " + build_dir);
        if(!exe_path.empty()) Log("Exe found: " + exe_path);
        else Log("!! Executable not found. Please Build.");

        LoadConfig();
        
        // Hook close event to save config
        callback(CloseCB, this);
    }

    void Log(const std::string& msg) {
        log_buff->append(msg.c_str());
        log_buff->append("\n");
        log_disp->scroll(log_buff->length(), 0);
    }

    // Attempt to locate repo root and build dir
    void FindPaths() {
        // Assume we run from build/bin or similar, walk up to find CMakeLists.txt
        fs::path p = fs::current_path();
        for(int i=0; i<5; ++i) {
            if(fs::exists(p / "CMakeLists.txt") && fs::exists(p / "game")) {
                project_root = p.string();
                build_dir = (p / "build").string(); // default assumption
                
                // If we are IN the build dir, use current
                if (fs::exists(p / "CMakeCache.txt")) {
                    build_dir = p.string();
                }
                break;
            }
            p = p.parent_path();
        }

        if (project_root.empty()) project_root = "."; // Fallback

        // Find Exe
        const char* exeName = "PsyDoom";
#ifdef _WIN32
        std::string exe = std::string(exeName) + ".exe";
#else
        std::string exe = exeName;
#endif
        
        // Check common locations
        std::vector<std::string> candidates = {
            build_dir + "/game/" + exe,
            build_dir + "/game/Debug/" + exe,
            build_dir + "/game/Release/" + exe,
            build_dir + "/game/RelWithDebInfo/" + exe
        };

        for(const auto& s : candidates) {
            if(fs::exists(s)) {
                exe_path = s;
                break;
            }
        }
    }

    void RunBuild(bool clean) {
        if(is_building) return;
        is_building = true;
        btn_build->deactivate();
        btn_launch->deactivate();

        // Run in thread to keep UI responsive
        std::thread([this, clean]() {
            auto streamData = [this](std::string s) {
                Fl::lock();
                log_buff->append(s.c_str());
                log_disp->scroll(log_buff->length(), 0);
                Fl::unlock();
                Fl::awake();
            };

            if(clean) {
                Fl::lock(); Log("Cleaning build dir..."); Fl::unlock();
                 ProcessUtilities::RunAndCapture("cmake --build . --target clean", build_dir, streamData);
            }

            Fl::lock(); Log("Starting Build..."); Fl::unlock();
            
            // Invoke CMake
            int ret = ProcessUtilities::RunAndCapture("cmake --build . --config RelWithDebInfo", build_dir, streamData);

            Fl::lock(); 
            if(ret == 0) {
                Log("Build SUCCESS!");
                FindPaths(); // Re-scan for exe
            } else {
                Log("Build FAILED! Code: " + std::to_string(ret));
            }
            
            is_building = false;
            btn_build->activate();
            btn_launch->activate();
            Fl::awake(); // Wake up main loop
            Fl::unlock();
        }).detach();
    }

    void LaunchGame() {
        if(exe_path.empty() || !fs::exists(exe_path)) {
            Log("Error: Executable not found. Build first.");
            return;
        }

        std::vector<std::string> args;
        if(chk_devparm->value()) args.push_back("-devparm");
        if(chk_nomonsters->value()) args.push_back("-nomonsters");
        if(chk_pistolstart->value()) args.push_back("-pistolstart");
        
        std::string w = input_warp->value();
        if(!w.empty()) {
            args.push_back("-warp");
            args.push_back(w);
        }

        Log("Launching: " + exe_path);
        fs::path exe(exe_path);
        ProcessUtilities::SpawnAsync(exe_path, args, exe.parent_path().string());
    }

    void LoadConfig() {
        if (!FileUtils::fileExists(config_file.c_str())) return;

        Log("Loading config...");
        auto data = FileUtils::getContentsOfFile(config_file.c_str(), 1); // +1 for null terminator
        if (!data.bytes) return;
        
        // Ensure null termination for parsing
        data.bytes[data.size] = std::byte(0);

        rapidjson::Document doc;
        doc.Parse(reinterpret_cast<const char*>(data.bytes.get()));

        if (doc.HasParseError() || !doc.IsObject()) {
            Log("Failed to parse config file.");
            return;
        }

        if (doc.HasMember("warp") && doc["warp"].IsString())
            input_warp->value(doc["warp"].GetString());
        
        if (doc.HasMember("nomonsters") && doc["nomonsters"].IsBool())
            chk_nomonsters->value(doc["nomonsters"].GetBool() ? 1 : 0);
            
        if (doc.HasMember("pistolstart") && doc["pistolstart"].IsBool())
            chk_pistolstart->value(doc["pistolstart"].GetBool() ? 1 : 0);

        if (doc.HasMember("devparm") && doc["devparm"].IsBool())
            chk_devparm->value(doc["devparm"].GetBool() ? 1 : 0);
    }

    void SaveConfig() {
        Log("Saving config...");
        rapidjson::StringBuffer sb;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);

        writer.StartObject();
        
        writer.Key("warp");
        writer.String(input_warp->value());

        writer.Key("nomonsters");
        writer.Bool(chk_nomonsters->value() != 0);

        writer.Key("pistolstart");
        writer.Bool(chk_pistolstart->value() != 0);

        writer.Key("devparm");
        writer.Bool(chk_devparm->value() != 0);

        writer.EndObject();

        FileUtils::writeDataToFile(config_file.c_str(), sb.GetString(), sb.GetSize());
    }

    static void BuildCB(Fl_Widget*, void* v) { ((LauncherWindow*)v)->RunBuild(false); }
    static void CleanCB(Fl_Widget*, void* v) { ((LauncherWindow*)v)->RunBuild(true); }
    static void LaunchCB(Fl_Widget*, void* v) { ((LauncherWindow*)v)->LaunchGame(); }
    static void CloseCB(Fl_Widget*, void* v) { 
        LauncherWindow* win = (LauncherWindow*)v;
        win->SaveConfig();
        win->hide();
    }
};

int main(int argc, char **argv) {
    Fl::lock(); // Enable multi-threading support
    Fl::scheme("gtk+");
    
    LauncherWindow *win = new LauncherWindow();
    win->show(argc, argv);
    
    return Fl::run();
}
