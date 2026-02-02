#pragma once

#include <string>
#include <vector>
#include <functional>

namespace ProcessUtilities {

    // Returns exit code. Blocks execution.
    // 'outputCallback' is invoked with chunks of stdout/stderr text.
    int RunAndCapture(const std::string& command, const std::string& workingDir, std::function<void(std::string)> outputCallback);

    // Returns exit code. Blocks execution.
    int RunBlocking(const std::string& command, const std::string& workingDir = ".");

    // Launches and forgets (for running the game). Returns true if successful.
    bool SpawnAsync(const std::string& exePath, const std::vector<std::string>& args, const std::string& workingDir = ".");
    
    // Open a folder in system file explorer
    void OpenExplorer(const std::string& path);

}
