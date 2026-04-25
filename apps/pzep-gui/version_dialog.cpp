#include <cstdio>
#include <sstream>
#include <string>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

// Version numbers are provided as preprocessor defines by the build system
#ifndef ZEP_VERSION_MAJOR
#define ZEP_VERSION_MAJOR 0
#define ZEP_VERSION_MINOR 0
#define ZEP_VERSION_PATCH 0
#endif

void ShowVersionDialog()
{
    std::ostringstream ss;
    ss << "pZep - Vim-like editor " << ZEP_VERSION_MAJOR << "." << ZEP_VERSION_MINOR << "." << ZEP_VERSION_PATCH
       << " (" << __DATE__ << ", compiled " << __TIME__ << ")\n";
#ifdef _WIN64
    ss << "\nMS-Windows 64-bit raylib version with plugin support";
#else
    ss << "\nPlatform raylib version with plugin support";
#endif
    ss << "\nCompiled by ";
#ifdef _MSC_VER
    ss << "MSVC " << _MSC_FULL_VER;
#else
    ss << "unknown";
#endif
    ss << "\n\nFeatures included (+) or not (-):";
    ss << "\n+vim-mode";
    ss << "\n+repl-support";
    ss << "\n+syntax-highlighting";
    ss << "\n+line-numbers";
    ss << "\n+statusline";
    ss << "\n+minimap";
    ss << "\n+git-integration";
    ss << "\n+plugin-support";
    ss << "\n+raylib-display";
    ss << "\n+multi-buffer";
    ss << "\n\nsystem pzeprc file: \"$PZEP\\pzeprc\"";
    ss << "\nuser pzeprc file: \"$PZEP\\_pzeprc\"";
    ss << "\n\nCompilation: cmake";
#ifdef _MSC_VER
    ss << " MSVC " << _MSC_FULL_VER;
#endif
    ss << " CXXFLAGS: /DWIN32 /D_WINDOWS /GR /EHsc";
    ss << "\nLinking: link /nologo /opt:ref /LTCG";
    ss << "\n";
#ifdef _WIN32
    MessageBoxA(nullptr, ss.str().c_str(), "pZep --version", MB_OK | MB_ICONINFORMATION);
#else
    // On non-Windows, just print to stdout
    printf("%s\n", ss.str().c_str());
#endif
}

void ShowHelpDialog()
{
    std::ostringstream ss;
    ss << "pZep - Vim-like editor\n";
    ss << "\nUsage: pzep-gui [OPTIONS] [FILE]\n";
    ss << "\nOptions:\n";
    ss << "  --version    Show version information\n";
    ss << "  --help       Show this help message\n";
    ss << "  --info       Show extended build info\n";
    ss << "\nVim-style commands:\n";
    ss << "  :q           Quit\n";
    ss << "  :q!          Quit without saving\n";
    ss << "  :e FILE      Edit file\n";
    ss << "  :sp FILE     Split horizontally\n";
    ss << "  :vsp FILE    Split vertically\n";
    ss << "  :r FILE      Read file into current buffer\n";
    ss << "  :w           Save\n";
    ss << "  :wq          Save and quit\n";
    ss << "  i            Insert mode\n";
    ss << "  Esc          Normal mode\n";
    ss << "\nKey mappings:\n";
    ss << "  F11          Toggle fullscreen\n";
    ss << "  Ctrl+Q       Quit (respects unsaved changes)\n";
    ss << "  Ctrl++/-     Adjust font size\n";
    ss << "  Ctrl+Wheel   Adjust font size\n";
#ifdef _WIN32
    MessageBoxA(nullptr, ss.str().c_str(), "pZep --help", MB_OK | MB_ICONINFORMATION);
#else
    printf("%s\n", ss.str().c_str());
#endif
}

void ShowInfoDialog()
{
    std::ostringstream ss;
    ss << "pZep - Detailed Build Information\n";
    ss << "\nVersion: " << ZEP_VERSION_MAJOR << "." << ZEP_VERSION_MINOR << "." << ZEP_VERSION_PATCH << "\n";
    ss << "\nBuild date: " << __DATE__ << " " << __TIME__ << "\n";
#ifdef _WIN64
    ss << "Platform: MS-Windows 64-bit\n";
#else
    ss << "Platform: Unknown\n";
#endif
#ifdef _MSC_VER
    ss << "Compiler: MSVC " << _MSC_FULL_VER << "\n";
#else
    ss << "Compiler: Unknown\n";
#endif
    ss << "\nFeatures:\n";
    ss << "  Vim mode: Enabled\n";
    ss << "  REPL support: Built-in\n";
    ss << "  Syntax highlighting: Enabled\n";
    ss << "  Line numbers: Enabled\n";
    ss << "  Status line: Enabled\n";
    ss << "  Minimap: Enabled\n";
    ss << "  Git integration: Enabled\n";
    ss << "  Plugin support: Enabled\n";
    ss << "  Raylib display: Enabled\n";
    ss << "  Multi-buffer: Enabled\n\n";
    ss << "Config paths:\n";
    ss << "  System: $PZEP\\pzeprc\n";
    ss << "  User: $PZEP\\_pzeprc\n";
    ss << "\nCompilation:\n";
    ss << "  Build system: CMake\n";
#ifdef _MSC_VER
    ss << "  CXXFLAGS: /DWIN32 /D_WINDOWS /GR /EHsc\n";
    ss << "  Linker: link /nologo /opt:ref /LTCG\n";
#endif
#ifdef _WIN32
    MessageBoxA(nullptr, ss.str().c_str(), "pZep --info", MB_OK | MB_ICONINFORMATION);
#else
    printf("%s\n", ss.str().c_str());
#endif
}
