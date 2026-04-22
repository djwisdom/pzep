#pragma once

#include "zep/buffer.h"
#include "zep/mcommon/threadpool.h"
#include <atomic>
#include <deque>
#include <future>
#include <mutex>
#include <string>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <processthreadsapi.h>
#include <windows.h>
// ConPTY requires Windows 10 1809+
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#ifndef ENABLE_PROCESSED_OUTPUT
#define ENABLE_PROCESSED_OUTPUT 0x0001
#endif
// For ConPTY
#include <consoleapi2.h>
#include <wincon.h>
#else
#include <pty.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#endif

namespace Zep
{

// Forward declarations
struct TerminalColor
{
    uint8_t r, g, b;
    bool bold = false;
    bool underline = false;
    bool reverse = false;
    bool dim = false;
};

struct TerminalCell
{
    char codepoint;
    TerminalColor fg;
    TerminalColor bg;
    bool bold = false;
    bool underline = false;
};

struct TerminalState
{
    int rows = 24;
    int cols = 80;
    int cursorRow = 0;
    int cursorCol = 0;
    bool insertMode = false;
    bool wrapMode = true;

    TerminalColor defaultFg = { 255, 255, 255, false, false, false, false };
    TerminalColor defaultBg = { 0, 0, 0, false, false, false, false };
    TerminalColor currentFg;
    TerminalColor currentBg;

    std::deque<std::vector<TerminalCell>> screen; // Current screen (rows x cols)
    std::deque<std::vector<TerminalCell>> scrollback; // Scrollback buffer

    bool alternateScreen = false;
    std::deque<std::vector<TerminalCell>> altScreen; // Alternate screen buffer
};

class ZepTerminal : public ZepComponent
{
public:
    ZepTerminal(ZepEditor& editor, ZepBuffer* pBuffer);
    virtual ~ZepTerminal();

    // Start the terminal shell
    bool Start(const std::string& shell = "");

    // Stop the terminal
    void Stop();

    // Send input to terminal
    void SendInput(const std::string& data);
    void SendKey(uint32_t key, uint32_t modifier = 0);

    // Get the associated buffer
    ZepBuffer* GetBuffer() const
    {
        return m_pBuffer;
    }

    // Terminal state access
    TerminalState& GetState()
    {
        return m_state;
    }
    const TerminalState& GetState() const
    {
        return m_state;
    }

    // Get the styled text for rendering (converts TerminalCells to buffer text with colors)
    // The buffer will be updated with the terminal contents
    void UpdateBuffer();

    // Is the terminal running?
    bool IsRunning() const
    {
        return m_running;
    }

    // Get terminal dimensions
    void SetSize(int rows, int cols);
    void GetSize(int& rows, int& cols) const;

private:
    // Platform-specific process handling
    bool StartProcess(const std::string& shell);
    void StopProcess();
    void ReadOutputLoop();
    void WriteInputLoop();

    // ANSI escape sequence parser
    void ProcessANSI(const std::string& seq);
    void ProcessCSI(const std::string& seq);
    void ProcessOSC(const std::string& seq);

    // Terminal operations
    void HandleCR();
    void HandleLF();
    void HandleBS();
    void HandleTab();
    void HandleBell();
    void OutputChar(char ch);
    void OutputString(const std::string& str);

    // ANSI state management
    void SetFGColor(int color);
    void SetBGColor(int color);
    void SetBold(bool enable);
    void SetUnderline(bool enable);
    void SetReverse(bool enable);
    void SetDim(bool enable);
    void ResetAttributes();

    // Cursor movement
    void CursorUp(int n = 1);
    void CursorDown(int n = 1);
    void CursorForward(int n = 1);
    void CursorBack(int n = 1);
    void CursorSetRow(int row);
    void CursorSetCol(int col);
    void CursorSet(int row, int col);
    void SaveCursor();
    void RestoreCursor();

    // Screen manipulation
    void EraseDisplay(int mode);
    void EraseLine(int mode);
    void ScrollUp(int n = 1);
    void ScrollDown(int n = 1);
    void InsertLines(int n = 1);
    void DeleteLines(int n = 1);
    void SwitchToAltScreen();
    void SwitchFromAltScreen();

    // Convert terminal color to theme color index
    int MapColorToTheme(const TerminalColor& color) const;

private:
    ZepBuffer* m_pBuffer = nullptr;
    std::atomic<bool> m_running{ false };
    std::atomic<bool> m_readThreadRunning{ false };
    std::atomic<bool> m_writeThreadRunning{ false };

#ifdef _WIN32
    HANDLE m_hProcess = nullptr;
    HANDLE m_hPipeInRead = nullptr;
    HANDLE m_hPipeInWrite = nullptr;
    HANDLE m_hPipeOutRead = nullptr;
    HANDLE m_hPipeOutWrite = nullptr;
    HANDLE m_hPty = nullptr;
#else
    pid_t m_pid = -1;
    int m_masterFd = -1;
#endif

    TerminalState m_state;

    // ANSI parsing state
    enum class AnsiState
    {
        Ground,
        Escape,
        CSI,
        OSC,
        Param
    };
    AnsiState m_ansiState = AnsiState::Ground;
    std::string m_ansiBuffer;
    std::vector<int> m_csiParams;
    bool m_csiPrivate = false;
    bool m_csiFinal = false;

    // Cursor save/restore
    TerminalState m_savedState;

    // Thread handling
    std::future<void> m_readFuture;
    std::unique_ptr<ThreadPool> m_writeThreadPool;

    // Mutex for thread safety
    std::mutex m_mutex;
};

} // namespace Zep
