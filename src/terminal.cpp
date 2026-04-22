#include "zep/terminal.h"
#include "zep/editor.h"
#include "zep/mcommon/logger.h"
#include "zep/mcommon/string/stringutils.h"
#include "zep/mode.h"
#include "zep/splits.h"
#include "zep/theme.h"

#include <algorithm>
#include <cctype>
#include <sstream>

// Undefine possible macro conflicts from Windows headers
#undef ERROR

namespace Zep
{

// ANSI color palette (16 colors, 8 normal + 8 bright)
static const struct
{
    uint8_t r, g, b;
} ansiColors[16] = {
    { 0, 0, 0 }, // 0: Black
    { 170, 0, 0 }, // 1: Red
    { 0, 170, 0 }, // 2: Green
    { 170, 85, 0 }, // 3: Yellow
    { 0, 0, 170 }, // 4: Blue
    { 170, 0, 170 }, // 5: Magenta
    { 0, 170, 170 }, // 6: Cyan
    { 170, 170, 170 }, // 7: White (normal)
    { 85, 85, 85 }, // 8: Bright Black (Gray)
    { 255, 85, 85 }, // 9: Bright Red
    { 85, 255, 85 }, // 10: Bright Green
    { 255, 255, 85 }, // 11: Bright Yellow
    { 85, 85, 255 }, // 12: Bright Blue
    { 255, 85, 255 }, // 13: Bright Magenta
    { 85, 255, 255 }, // 14: Bright Cyan
    { 255, 255, 255 }, // 15: Bright White
};

ZepTerminal::ZepTerminal(ZepEditor& editor, ZepBuffer* pBuffer)
    : ZepComponent(editor)
    , m_pBuffer(pBuffer)
    , m_state()
{
    // Initialize terminal state
    m_state.rows = 24;
    m_state.cols = 80;
    m_state.cursorRow = 0;
    m_state.cursorCol = 0;
    m_state.defaultFg = { 255, 255, 255, false, false, false, false };
    m_state.defaultBg = { 0, 0, 0, false, false, false, false };
    m_state.currentFg = m_state.defaultFg;
    m_state.currentBg = m_state.defaultBg;

    // Initialize screen buffer
    std::vector<TerminalCell> blankLine(m_state.cols);
    for (auto& cell : blankLine)
    {
        cell.codepoint = ' ';
        cell.fg = m_state.defaultFg;
        cell.bg = m_state.defaultBg;
    }
    m_state.screen.push_back(blankLine);

    ZLOG(INFO, "ZepTerminal created");
}

ZepTerminal::~ZepTerminal()
{
    Stop();
}

bool ZepTerminal::Start(const std::string& shell)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_running)
    {
        return true;
    }

    if (!StartProcess(shell))
    {
        ZLOG(ERROR, "Failed to start terminal process");
        return false;
    }

    m_running = true;
    m_readThreadRunning = true;

    // Start async read thread using editor's thread pool
    m_readFuture = GetEditor().GetThreadPool().enqueue([this]() {
        ReadOutputLoop();
    });

    ZLOG(INFO, "Terminal started successfully");
    return true;
}

void ZepTerminal::Stop()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_running)
    {
        return;
    }

    m_running = false;
    m_readThreadRunning = false;

    StopProcess();

    if (m_readFuture.valid())
    {
        m_readFuture.wait();
    }

    ZLOG(INFO, "Terminal stopped");
}

void ZepTerminal::SendInput(const std::string& data)
{
    if (!m_running)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

#ifdef _WIN32
    DWORD written = 0;
    WriteFile(m_hPipeInWrite, data.c_str(), (DWORD)data.size(), &written, nullptr);
#else
    if (m_masterFd >= 0)
    {
        write(m_masterFd, data.c_str(), data.size());
    }
#endif
}

void ZepTerminal::SendKey(uint32_t key, uint32_t modifier)
{
    // Translate Zep key codes to terminal sequences
    // This is simplified; will need more complete mapping
    std::string seq;

    switch (key)
    {
    case ExtKeys::RETURN:
        seq = "\r";
        break;
    case ExtKeys::ESCAPE:
        seq = "\x1b";
        break;
    case ExtKeys::BACKSPACE:
        seq = "\x7f"; // DEL
        break;
    case ExtKeys::TAB:
        seq = "\t";
        break;
    case ExtKeys::UP:
        seq = "\x1b[A";
        break;
    case ExtKeys::DOWN:
        seq = "\x1b[B";
        break;
    case ExtKeys::RIGHT:
        seq = "\x1b[C";
        break;
    case ExtKeys::LEFT:
        seq = "\x1b[D";
        break;
    case ExtKeys::HOME:
        seq = "\x1b[H";
        break;
    case ExtKeys::END:
        seq = "\x1b[F";
        break;
    default:
        // For regular characters, convert from key code
        if (key >= 32 && key <= 126)
        {
            seq = char(key);
        }
        break;
    }

    SendInput(seq);
}

void ZepTerminal::SetSize(int rows, int cols)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (rows <= 0 || cols <= 0)
    {
        return;
    }

    m_state.rows = rows;
    m_state.cols = cols;

#ifdef _WIN32
    // Resize ConPTY
    if (m_hPty)
    {
        COORD size;
        size.X = (SHORT)cols;
        size.Y = (SHORT)rows;
        SetConsoleScreenBufferSize(m_hPty, size);
    }
#else
    struct winsize ws;
    ws.ws_row = rows;
    ws.ws_col = cols;
    ws.ws_xpixel = 0;
    ws.ws_ypixel = 0;
    if (m_masterFd >= 0)
    {
        ioctl(m_masterFd, TIOCSWINSZ, &ws);
        kill(-m_pid, SIGWINCH);
    }
#endif
}

void ZepTerminal::GetSize(int& rows, int& cols) const
{
    rows = m_state.rows;
    cols = m_state.cols;
}

void ZepTerminal::ReadOutputLoop()
{
    char buffer[4096];
    std::string partial;

    while (m_readThreadRunning)
    {
        int bytesRead = 0;

#ifdef _WIN32
        DWORD read = 0;
        if (!ReadFile(m_hPipeOutRead, buffer, sizeof(buffer) - 1, &read, nullptr))
        {
            break;
        }
        bytesRead = read;
#else
        bytesRead = read(m_masterFd, buffer, sizeof(buffer) - 1);
        if (bytesRead <= 0)
        {
            break;
        }
#endif

        if (bytesRead > 0)
        {
            buffer[bytesRead] = 0;
            partial.append(buffer, bytesRead);

            // Process complete characters
            size_t pos = 0;
            while (pos < partial.size())
            {
                char ch = partial[pos++];

                if (m_ansiState == AnsiState::Ground)
                {
                    if (ch == '\x1b')
                    {
                        m_ansiState = AnsiState::Escape;
                        m_ansiBuffer.clear();
                    }
                    else
                    {
                        // Normal character
                        OutputChar(ch);
                    }
                }
                else if (m_ansiState == AnsiState::Escape)
                {
                    if (ch == '[')
                    {
                        m_ansiState = AnsiState::CSI;
                        m_ansiBuffer.clear();
                        m_csiParams.clear();
                        m_csiPrivate = false;
                    }
                    else if (ch == ']')
                    {
                        m_ansiState = AnsiState::OSC;
                        m_ansiBuffer.clear();
                    }
                    else
                    {
                        // Other escape sequences (simplified)
                        m_ansiState = AnsiState::Ground;
                    }
                }
                else if (m_ansiState == AnsiState::CSI)
                {
                    m_ansiBuffer += ch;
                    if (ch >= 0x40 && ch <= 0x7E)
                    {
                        ProcessCSI(m_ansiBuffer);
                        m_ansiState = AnsiState::Ground;
                    }
                }
                else if (m_ansiState == AnsiState::OSC)
                {
                    m_ansiBuffer += ch;
                    // OSC ends with BEL (\x07) or ST (\x1b\x5c)
                    if (ch == '\x07' || (m_ansiBuffer.size() >= 2 && m_ansiBuffer[m_ansiBuffer.size() - 2] == '\x1b' && m_ansiBuffer.back() == '\\'))
                    {
                        ProcessOSC(m_ansiBuffer);
                        m_ansiState = AnsiState::Ground;
                    }
                }
            }

            // Remove processed bytes
            partial.erase(0, pos);
        }
    }
}

void ZepTerminal::ProcessCSI(const std::string& seq)
{
    // CSI structure: [private?]param...intermediate?final
    // Example: "\x1b[32m" = set fg green
    // "\x1b[2J" = erase display

    if (seq.size() < 2)
        return;

    char finalChar = seq.back();
    std::string body = seq.substr(1, seq.size() - 2);

    // Check for private marker
    size_t start = 0;
    if (!body.empty() && body[0] == '?')
    {
        m_csiPrivate = true;
        start = 1;
    }

    // Parse parameters (semicolon-separated)
    m_csiParams.clear();
    std::stringstream ss(body.substr(start));
    std::string token;
    while (std::getline(ss, token, ';'))
    {
        if (token.empty())
        {
            m_csiParams.push_back(0);
        }
        else
        {
            m_csiParams.push_back(std::stoi(token));
        }
    }
    if (m_csiParams.empty())
    {
        m_csiParams.push_back(0);
    }

    // Execute final command
    switch (finalChar)
    {
    case 'm':
    { // SGR - Select Graphic Rendition
        if (m_csiParams[0] == 0)
        {
            ResetAttributes();
        }
        for (size_t i = 0; i < m_csiParams.size(); i++)
        {
            int param = m_csiParams[i];
            if (param >= 30 && param <= 37)
            {
                SetFGColor(param - 30);
            }
            else if (param >= 40 && param <= 47)
            {
                SetBGColor(param - 40);
            }
            else if (param >= 90 && param <= 97)
            {
                SetFGColor(param - 90 + 8);
            }
            else if (param >= 100 && param <= 107)
            {
                SetBGColor(param - 100 + 8);
            }
            else if (param == 1)
            {
                SetBold(true);
            }
            else if (param == 4)
            {
                SetUnderline(true);
            }
            else if (param == 7)
            {
                SetReverse(true);
            }
            else if (param == 22)
            {
                SetBold(false);
            }
            else if (param == 24)
            {
                SetUnderline(false);
            }
            else if (param == 27)
            {
                SetReverse(false);
            }
        }
        break;
    }
    case 'J':
    { // Erase in Display
        int mode = m_csiParams[0];
        if (mode == 0)
        {
            EraseDisplay(0); // Clear from cursor to end
        }
        else if (mode == 1)
        {
            EraseDisplay(1); // Clear from beginning to cursor
        }
        else if (mode == 2)
        {
            EraseDisplay(2); // Clear entire screen
        }
        break;
    }
    case 'K':
    { // Erase in Line
        int mode = m_csiParams[0];
        EraseLine(mode);
        break;
    }
    case 'H': // Cursor Position
    case 'f':
    {
        int row = m_csiParams[0] ? m_csiParams[0] - 1 : 0;
        int col = m_csiParams.size() > 1 ? m_csiParams[1] - 1 : 0;
        CursorSet(row, col);
        break;
    }
    case 'A':
        CursorUp(m_csiParams[0]);
        break;
    case 'B':
        CursorDown(m_csiParams[0]);
        break;
    case 'C':
        CursorForward(m_csiParams[0]);
        break;
    case 'D':
        CursorBack(m_csiParams[0]);
        break;
    case 's':
        SaveCursor();
        break;
    case 'u':
        RestoreCursor();
        break;
    case 'L':
        InsertLines(m_csiParams[0]);
        break;
    case 'M':
        DeleteLines(m_csiParams[0]);
        break;
    default:
        // Unhandled CSI
        break;
    }
}

void ZepTerminal::ProcessOSC(const std::string& seq)
{
    // OSC handling (window title etc.)
    // Simplified - just ignore for now
}

void ZepTerminal::OutputChar(char ch)
{
    switch (ch)
    {
    case '\r':
        HandleCR();
        break;
    case '\n':
        HandleLF();
        break;
    case '\b':
    case '\x7f':
        HandleBS();
        break;
    case '\t':
        HandleTab();
        break;
    case '\x07': // BEL
        HandleBell();
        break;
    default:
        // Insert character at cursor position
        if (m_state.cursorRow >= m_state.screen.size())
        {
            // Should not happen, but add lines if needed
            while (m_state.screen.size() <= (size_t)m_state.cursorRow)
            {
                std::vector<TerminalCell> line(m_state.cols);
                for (auto& cell : line)
                {
                    cell.codepoint = ' ';
                    cell.fg = m_state.currentFg;
                    cell.bg = m_state.currentBg;
                }
                m_state.screen.push_back(line);
            }
        }

        auto& line = m_state.screen[m_state.cursorRow];
        if (m_state.cursorCol < m_state.cols)
        {
            line[m_state.cursorCol].codepoint = ch;
            line[m_state.cursorCol].fg = m_state.currentFg;
            line[m_state.cursorCol].bg = m_state.currentBg;
            line[m_state.cursorCol].bold = m_state.currentFg.bold;
            line[m_state.cursorCol].underline = m_state.currentFg.underline;
        }

        m_state.cursorCol++;
        if (m_state.cursorCol >= m_state.cols)
        {
            m_state.cursorCol = 0;
            HandleLF();
        }
        break;
    }

    // Mark buffer for update
    // Will need to call UpdateBuffer from main thread
}

void ZepTerminal::HandleCR()
{
    m_state.cursorCol = 0;
}

void ZepTerminal::HandleLF()
{
    m_state.cursorRow++;
    if (m_state.cursorRow >= m_state.rows)
    {
        ScrollUp(1);
        m_state.cursorRow = m_state.rows - 1;
    }
    // Some terminals also do CR on LF
    // m_state.cursorCol = 0; // Optional, depends on mode
}

void ZepTerminal::HandleBS()
{
    if (m_state.cursorCol > 0)
    {
        m_state.cursorCol--;
    }
}

void ZepTerminal::HandleTab()
{
    m_state.cursorCol = (m_state.cursorCol + 8) & ~7;
    if (m_state.cursorCol >= m_state.cols)
    {
        HandleLF();
        m_state.cursorCol = 0;
    }
}

void ZepTerminal::HandleBell()
{
    // Visual bell - could flash or beep
}

void ZepTerminal::SetFGColor(int color)
{
    if (color < 16)
    {
        m_state.currentFg.r = ansiColors[color].r;
        m_state.currentFg.g = ansiColors[color].g;
        m_state.currentFg.b = ansiColors[color].b;
    }
}

void ZepTerminal::SetBGColor(int color)
{
    if (color < 16)
    {
        m_state.currentBg.r = ansiColors[color].r;
        m_state.currentBg.g = ansiColors[color].g;
        m_state.currentBg.b = ansiColors[color].b;
    }
}

void ZepTerminal::SetBold(bool enable)
{
    m_state.currentFg.bold = enable;
}

void ZepTerminal::SetUnderline(bool enable)
{
    m_state.currentFg.underline = enable;
}

void ZepTerminal::SetReverse(bool enable)
{
    m_state.currentFg.reverse = enable; // Note: we'll swap on render
}

void ZepTerminal::SetDim(bool enable)
{
    m_state.currentFg.dim = enable;
}

void ZepTerminal::ResetAttributes()
{
    m_state.currentFg = m_state.defaultFg;
    m_state.currentBg = m_state.defaultBg;
}

void ZepTerminal::CursorUp(int n)
{
    m_state.cursorRow = std::max(0, m_state.cursorRow - n);
}

void ZepTerminal::CursorDown(int n)
{
    m_state.cursorRow += n;
    while (m_state.cursorRow >= m_state.rows)
    {
        ScrollUp(1);
        m_state.cursorRow--;
    }
}

void ZepTerminal::CursorForward(int n)
{
    m_state.cursorCol += n;
    if (m_state.cursorCol >= m_state.cols)
    {
        m_state.cursorCol = m_state.cols - 1;
    }
}

void ZepTerminal::CursorBack(int n)
{
    m_state.cursorCol = std::max(0, m_state.cursorCol - n);
}

void ZepTerminal::CursorSetRow(int row)
{
    m_state.cursorRow = std::min(m_state.rows - 1, std::max(0, row));
}

void ZepTerminal::CursorSetCol(int col)
{
    m_state.cursorCol = std::min(m_state.cols - 1, std::max(0, col));
}

void ZepTerminal::CursorSet(int row, int col)
{
    CursorSetRow(row);
    CursorSetCol(col);
}

void ZepTerminal::SaveCursor()
{
    m_savedState = m_state;
}

void ZepTerminal::RestoreCursor()
{
    m_state = m_savedState;
}

void ZepTerminal::EraseDisplay(int mode)
{
    if (mode == 0 || mode == 1 || mode == 2)
    {
        // Erase as needed
        if (mode == 0)
        {
            // Clear from cursor to end of screen
            for (int r = m_state.cursorRow; r < m_state.screen.size(); r++)
            {
                for (auto& cell : m_state.screen[r])
                {
                    cell.codepoint = ' ';
                    cell.fg = m_state.currentFg;
                    cell.bg = m_state.currentBg;
                }
            }
        }
        else if (mode == 1)
        {
            // Clear from beginning to cursor
            for (int r = 0; r <= m_state.cursorRow && r < m_state.screen.size(); r++)
            {
                auto limit = (r == m_state.cursorRow) ? m_state.cursorCol : m_state.cols;
                for (int c = 0; c < limit; c++)
                {
                    m_state.screen[r][c].codepoint = ' ';
                    m_state.screen[r][c].fg = m_state.currentFg;
                    m_state.screen[r][c].bg = m_state.currentBg;
                }
            }
        }
        else if (mode == 2)
        {
            // Clear entire screen
            for (auto& line : m_state.screen)
            {
                for (auto& cell : line)
                {
                    cell.codepoint = ' ';
                    cell.fg = m_state.currentFg;
                    cell.bg = m_state.currentBg;
                }
            }
        }
    }
}

void ZepTerminal::EraseLine(int mode)
{
    if (mode == 0 || mode == 1 || mode == 2)
    {
        auto& line = m_state.screen[m_state.cursorRow];
        if (mode == 0)
        {
            // Clear from cursor to end of line
            for (int c = m_state.cursorCol; c < m_state.cols; c++)
            {
                line[c].codepoint = ' ';
                line[c].fg = m_state.currentFg;
                line[c].bg = m_state.currentBg;
            }
        }
        else if (mode == 1)
        {
            // Clear from beginning to cursor
            for (int c = 0; c <= m_state.cursorCol && c < m_state.cols; c++)
            {
                line[c].codepoint = ' ';
                line[c].fg = m_state.currentFg;
                line[c].bg = m_state.currentBg;
            }
        }
        else if (mode == 2)
        {
            // Clear entire line
            for (auto& cell : line)
            {
                cell.codepoint = ' ';
                cell.fg = m_state.currentFg;
                cell.bg = m_state.currentBg;
            }
        }
    }
}

void ZepTerminal::ScrollUp(int n)
{
    for (int i = 0; i < n && !m_state.screen.empty(); i++)
    {
        // Save the top line to scrollback
        // (Could implement scrollback separately)
        // Remove first line
        m_state.screen.pop_front();
        // Add blank line at bottom
        std::vector<TerminalCell> blankLine(m_state.cols);
        for (auto& cell : blankLine)
        {
            cell.codepoint = ' ';
            cell.fg = m_state.currentFg;
            cell.bg = m_state.currentBg;
        }
        m_state.screen.push_back(blankLine);
    }
}

void ZepTerminal::ScrollDown(int n)
{
    for (int i = 0; i < n && !m_state.screen.empty(); i++)
    {
        // Remove last line
        m_state.screen.pop_back();
        // Add blank line at top
        std::vector<TerminalCell> blankLine(m_state.cols);
        for (auto& cell : blankLine)
        {
            cell.codepoint = ' ';
            cell.fg = m_state.defaultFg;
            cell.bg = m_state.defaultBg;
        }
        m_state.screen.push_front(blankLine);
    }
}

void ZepTerminal::InsertLines(int n)
{
    for (int i = 0; i < n && m_state.cursorRow < m_state.screen.size(); i++)
    {
        // Insert blank lines at cursor position
        m_state.screen.insert(m_state.screen.begin() + m_state.cursorRow,
            std::vector<TerminalCell>(m_state.cols));
        // Keep screen size
        if (m_state.screen.size() > (size_t)m_state.rows)
        {
            m_state.screen.pop_back();
        }
    }
}

void ZepTerminal::DeleteLines(int n)
{
    for (int i = 0; i < n && m_state.cursorRow < m_state.screen.size(); i++)
    {
        // Remove line at cursor, shift up
        if (m_state.cursorRow + 1 < m_state.screen.size())
        {
            m_state.screen.erase(m_state.screen.begin() + m_state.cursorRow);
        }
        // Add blank at bottom
        std::vector<TerminalCell> blankLine(m_state.cols);
        for (auto& cell : blankLine)
        {
            cell.codepoint = ' ';
            cell.fg = m_state.defaultFg;
            cell.bg = m_state.defaultBg;
        }
        m_state.screen.push_back(blankLine);
    }
}

void ZepTerminal::SwitchToAltScreen()
{
    m_state.alternateScreen = true;
    m_state.altScreen = m_state.screen;
    // Switch to a new blank screen
    m_state.screen.clear();
    std::vector<TerminalCell> blankLine(m_state.cols);
    for (auto& cell : blankLine)
    {
        cell.codepoint = ' ';
        cell.fg = m_state.defaultFg;
        cell.bg = m_state.defaultBg;
    }
    for (int i = 0; i < m_state.rows; i++)
    {
        m_state.screen.push_back(blankLine);
    }
}

void ZepTerminal::SwitchFromAltScreen()
{
    if (m_state.alternateScreen)
    {
        m_state.alternateScreen = false;
        m_state.screen = m_state.altScreen;
    }
}

void ZepTerminal::UpdateBuffer()
{
    if (!m_pBuffer)
        return;

    // Convert terminal screen to text buffer
    std::ostringstream oss;
    for (const auto& line : m_state.screen)
    {
        for (const auto& cell : line)
        {
            oss << cell.codepoint;
        }
        oss << '\n';
    }

    m_pBuffer->SetText(oss.str());
}

// Platform-specific process creation
bool ZepTerminal::StartProcess(const std::string& shell)
{
#ifdef _WIN32
    // Create pipes for stdin/stdout
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };

    // Create pipe for child's stdout
    if (!CreatePipe(&m_hPipeOutRead, &m_hPipeOutWrite, &sa, 0))
    {
        return false;
    }

    // Create pipe for child's stdin
    if (!CreatePipe(&m_hPipeInRead, &m_hPipeInWrite, &sa, 0))
    {
        return false;
    }

    // Ensure the write handles are not inherited
    SetHandleInformation(m_hPipeOutWrite, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(m_hPipeInRead, HANDLE_FLAG_INHERIT, 0);

    // Try ConPTY first for proper terminal emulation
    HPCON hPC = nullptr;
    COORD size = { (SHORT)m_state.cols, (SHORT)m_state.rows };

    // Use legacy pipes if ConPTY fails
    bool useConpty = false;
    typedef HRESULT(WINAPI * CreatePseudoConsoleFn)(
        COORD size, HANDLE hInput, HANDLE hOutput, DWORD dwFlags, HPCON * phPC);
    typedef void(WINAPI * ClosePseudoConsoleFn)(HPCON hPC);

    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
    auto pCreatePseudoConsole = (CreatePseudoConsoleFn)GetProcAddress(hKernel32, "CreatePseudoConsole");
    auto pClosePseudoConsole = (ClosePseudoConsoleFn)GetProcAddress(hKernel32, "ClosePseudoConsole");

    if (pCreatePseudoConsole && pClosePseudoConsole)
    {
        HRESULT hr = pCreatePseudoConsole(size, m_hPipeInRead, m_hPipeOutWrite, 0, &hPC);
        if (SUCCEEDED(hr))
        {
            useConpty = true;
            m_hPty = hPC;
        }
    }

    STARTUPINFOEXW si = { sizeof(STARTUPINFOEXW) };
    PROCESS_INFORMATION pi = { 0 };

    if (useConpty)
    {
        // Use ConPTY
        si.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
        si.StartupInfo.hStdInput = nullptr;
        si.StartupInfo.hStdOutput = nullptr;
        si.StartupInfo.hStdError = nullptr;

        SIZE_T attrListSize = 0;
        InitializeProcThreadAttributeList(nullptr, 1, 0, &attrListSize);
        auto attrList = (LPPROC_THREAD_ATTRIBUTE_LIST)HeapAlloc(GetProcessHeap(), 0, attrListSize);
        InitializeProcThreadAttributeList(attrList, 1, 0, &attrListSize);
        UpdateProcThreadAttribute(attrList, 0, PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE,
            hPC, sizeof(HPCON), nullptr, nullptr);
        si.lpAttributeList = attrList;

        std::wstring cmdLine = L"cmd.exe";
        if (!shell.empty())
        {
            std::wstring wshell = std::wstring(shell.begin(), shell.end());
            cmdLine = wshell;
        }

        if (!CreateProcessW(nullptr, &cmdLine[0], nullptr, nullptr, FALSE,
                EXTENDED_STARTUPINFO_PRESENT, nullptr, nullptr,
                &si.StartupInfo, &pi))
        {
            pClosePseudoConsole(hPC);
            return false;
        }

        DeleteProcThreadAttributeList(attrList);
        HeapFree(GetProcessHeap(), 0, attrList);
    }
    else
    {
        // Legacy pipes
        si.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
        si.StartupInfo.hStdInput = m_hPipeInRead;
        si.StartupInfo.hStdOutput = m_hPipeOutWrite;
        si.StartupInfo.hStdError = m_hPipeOutWrite;

        std::wstring cmdLine = L"cmd.exe";
        if (!shell.empty())
        {
            std::wstring wshell = std::wstring(shell.begin(), shell.end());
            cmdLine = wshell;
        }

        if (!CreateProcessW(nullptr, &cmdLine[0], nullptr, nullptr, TRUE,
                0, nullptr, nullptr, &si.StartupInfo, &pi))
        {
            return false;
        }
    }

    m_hProcess = pi.hProcess;
    CloseHandle(pi.hThread);

    return true;
#else
    // Unix: use forkpty
    struct termios term;
    struct winsize ws;

    ws.ws_row = m_state.rows;
    ws.ws_col = m_state.cols;
    ws.ws_xpixel = 0;
    ws.ws_ypixel = 0;

    // Set terminal attributes for raw mode
    tcgetattr(STDIN_FILENO, &term);

    m_pid = forkpty(&m_masterFd, nullptr, &term, &ws);
    if (m_pid < 0)
    {
        return false;
    }

    if (m_pid == 0)
    {
        // Child process
        const char* shell_path = shell.empty() ? "/bin/bash" : shell.c_str();
        execlp(shell_path, shell_path, nullptr);
        _exit(1);
    }

    return true;
#endif
}

void ZepTerminal::StopProcess()
{
#ifdef _WIN32
    if (m_hProcess)
    {
        TerminateProcess(m_hProcess, 0);
        CloseHandle(m_hProcess);
        m_hProcess = nullptr;
    }

    if (m_hPipeInRead)
    {
        CloseHandle(m_hPipeInRead);
        m_hPipeInRead = nullptr;
    }
    if (m_hPipeInWrite)
    {
        CloseHandle(m_hPipeInWrite);
        m_hPipeInWrite = nullptr;
    }
    if (m_hPipeOutRead)
    {
        CloseHandle(m_hPipeOutRead);
        m_hPipeOutRead = nullptr;
    }
    if (m_hPipeOutWrite)
    {
        CloseHandle(m_hPipeOutWrite);
        m_hPipeOutWrite = nullptr;
    }

    if (m_hPty)
    {
        typedef void(WINAPI * ClosePseudoConsoleFn)(HPCON);
        HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
        auto pClosePseudoConsole = (ClosePseudoConsoleFn)GetProcAddress(hKernel32, "ClosePseudoConsole");
        if (pClosePseudoConsole)
        {
            pClosePseudoConsole(m_hPty);
        }
        m_hPty = nullptr;
    }
#else
    if (m_masterFd >= 0)
    {
        close(m_masterFd);
        m_masterFd = -1;
    }
    if (m_pid > 0)
    {
        kill(m_pid, SIGTERM);
        waitpid(m_pid, nullptr, 0);
        m_pid = -1;
    }
#endif
}

int ZepTerminal::MapColorToTheme(const TerminalColor& color) const
{
    // Find nearest ANSI color or return custom
    // For now, map standard 16 colors directly
    for (int i = 0; i < 16; i++)
    {
        if (ansiColors[i].r == color.r && ansiColors[i].g == color.g && ansiColors[i].b == color.b)
        {
            return i;
        }
    }
    return 7; // Default white
}

} // namespace Zep
