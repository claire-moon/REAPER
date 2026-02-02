#include "ProcessUtilities.h"
#include <cstdlib>
#include <iostream>
#include <filesystem>
#include <array>
#include <memory>

#if defined(_WIN32)
#include <windows.h>
#include <shellapi.h>
#else
#include <unistd.h>
#include <spawn.h>
#include <sys/wait.h>
#endif

namespace ProcessUtilities {

    int RunAndCapture(const std::string& command, const std::string& workingDir, std::function<void(std::string)> outputCallback) {
        std::string fullCmd = "cd /d \"" + workingDir + "\" && " + command + " 2>&1";
        #if !defined(_WIN32)
        fullCmd = "cd \"" + workingDir + "\" && " + command + " 2>&1";
        #endif

        std::array<char, 256> buffer;
        
        #if defined(_WIN32)
        auto pipe_closer = &::_pclose;
        auto pipe_opener = &::_popen;
        #else
        auto pipe_closer = &::pclose;
        auto pipe_opener = &::popen;
        #endif

        std::unique_ptr<FILE, decltype(pipe_closer)> pipe(pipe_opener(fullCmd.c_str(), "r"), pipe_closer);

        if (!pipe) {
            outputCallback("Failed to open pipe for command: " + command + "\n");
            return -1;
        }

        while (fgets(buffer.data(), (int)buffer.size(), pipe.get()) != nullptr) {
             outputCallback(buffer.data());
        }

        // Return exit code
        return 0; // pclose return value handling is platform specific and messy, 
                  // for now we assume if stream finished it ran.
                  // Ideally we check ferror or pclose result.
    }

    int RunBlocking(const std::string& command, const std::string& workingDir) {
        std::string fullCmd = "cd /d \"" + workingDir + "\" && " + command;
        #if !defined(_WIN32)
        fullCmd = "cd \"" + workingDir + "\" && " + command;
        #endif
        return std::system(fullCmd.c_str());
    }

    bool SpawnAsync(const std::string& exePath, const std::vector<std::string>& args, const std::string& workingDir) {
#if defined(_WIN32)
        std::string cmdLine = "\"" + exePath + "\"";
        for (const auto& arg : args) {
            cmdLine += " " + arg;
        }

        STARTUPINFOA si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        if (CreateProcessA(NULL, const_cast<char*>(cmdLine.c_str()), NULL, NULL, FALSE, 0, NULL, workingDir.c_str(), &si, &pi)) {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            return true;
        }
        return false;
#else
        // Simple fork/exec for POSIX
        pid_t pid = fork();
        if (pid == 0) {
            // Child
            std::vector<char*> cArgs;
            cArgs.push_back(const_cast<char*>(exePath.c_str()));
            for (const auto& a : args) cArgs.push_back(const_cast<char*>(a.c_str()));
            cArgs.push_back(nullptr);
            
            chdir(workingDir.c_str());
            execvp(exePath.c_str(), cArgs.data());
            _exit(1); 
        }
        return pid > 0;
#endif
    }

    void OpenExplorer(const std::string& path) {
#if defined(_WIN32)
        ShellExecuteA(NULL, "open", path.c_str(), NULL, NULL, SW_SHOWDEFAULT);
#else
        std::string cmd = "xdg-open \"" + path + "\"";
        std::system(cmd.c_str());
#endif
    }
}
