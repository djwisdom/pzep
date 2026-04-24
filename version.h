#pragma once

#include <string>

namespace Zep {

/**
 * @brief Version information for pZep
 */
struct VersionInfo {
    static constexpr int MAJOR = 0;
    static constexpr int MINOR = 5;
    static constexpr int PATCH = 23;
    
    // Full version string with git hash: 0.5.5+abc123
    static constexpr const char* FULL = "0.5.23+495c9bfb";
    
    // Git commit hash
    static constexpr const char* GIT_COMMIT = "495c9bfb";
    
    // Git commit count
    static constexpr int GIT_COMMIT_COUNT = 48;
    
    // Git branch
    static constexpr const char* GIT_BRANCH = "main";
    
    // Build timestamp
    static constexpr const char* BUILD_DATE = "";
    
    // Compiler info
    static constexpr const char* COMPILER = "MSVC 19.51.36231.0";
    
    /**
     * @brief Get version as string "major.minor.patch"
     */
    static std::string GetVersionString() {
        return std::to_string(MAJOR) + "." + 
               std::to_string(MINOR) + "." + 
               std::to_string(PATCH);
    }
    
    /**
     * @brief Get full version with git hash
     */
    static std::string GetFullVersion() {
        return GetVersionString() + "+" + GIT_COMMIT;
    }
};

} // namespace Zep
