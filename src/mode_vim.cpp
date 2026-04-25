#include <cctype>
#include <memory>
#include <sstream>

#include "zep/buffer.h"
#include "zep/commands_font.h"
#include "zep/commands_terminal.h"
#include "zep/commands_tutor.h"
#include "zep/editor.h"
#include "zep/filesystem.h"
#include "zep/git.h"
#include "zep/keymap.h"
#include "zep/mode_search.h"
#include "zep/mode_vim.h"
#include "zep/tab_window.h"
#include "zep/theme.h"
#include "zep/window.h"

#include "zep/mcommon/animation/timer.h"
#include "zep/mcommon/logger.h"
#include "zep/mcommon/string/stringutils.h"

// Note:
// This is a very basic implementation of the common Vim commands that I use: the bare minimum I can live with.
// I do use more, and depending on how much pain I suffer, will add them over time.
// My aim is to make it easy to add commands, so if you want to put something in, please send a PR.
// The buffer/display search and find support makes it easy to gather the info you need, and the basic insert/delete undo redo commands
// make it easy to find the locations in the buffer
// Important to note: I'm not trying to beat/better Vim here.  Just make an editor I can use in a viewport without feeling pain.
// See further down for what is implemented, and what's on my todo list

// IMPLEMENTED VIM:
// Command counts
// hjkl Motions
// . dot command
// TAB
// w,W,e,E,ge,gE,b,B WORD motions
// u,CTRL+r  Undo, Redo
// i,I,a,A Insert mode (pending undo/redo fix)
// DELETE/BACKSPACE in insert and normal mode; match vim
// Command status bar
// Arrow keys
// '$'
// 'jk' to insert mode
// 'gg' Jump to end
// 'G' Jump to beginning
// CTRL+F/B/D/U page and have page moves
// 'J' join
// D
// dd,d$,x  Delete line, to end of line, chars
// 'v' + 'x'/'d'
// 'y'
// 'p'/'P'
// a-z&a-Z, 0->9, _ " registers
// r Replace with char
// '$'
// 'yy'
// cc
// c$  Change to end of line
// C
// S, s, with visual modes
// '^'
// 'O', 'o'
// 'V' (linewise v)
// Y, D, linewise yank/paste
// d[a]<count>w/e  Delete words
// di[({})]"'
// c[a]<count>w/e  Change word
// ci[({})]"'
// ct[char]/dt[char] Change to and delete to
// vi[Ww], va[Ww] Visual inner and word selections
// f[char] find on line
// /[string] find in file, 'n' find next

namespace Zep
{

ZepMode_Vim::ZepMode_Vim(ZepEditor& editor)
    : ZepMode(editor)
{
}

ZepMode_Vim::~ZepMode_Vim()
{
}

void ZepMode_Vim::AddOverStrikeMaps()
{
    AddKeyMapWithCountRegisters({ &m_normalMap, &m_visualMap }, { "r<.>" }, id_Replace);
}

void ZepMode_Vim::AddCopyMaps()
{
    AddKeyMapWithCountRegisters({ &m_normalMap }, { "v" }, id_VisualMode);
    AddKeyMapWithCountRegisters({ &m_normalMap }, { "V" }, id_VisualLineMode);
    AddKeyMapWithCountRegisters({ &m_visualMap }, { "y" }, id_Yank);
    AddKeyMapWithCountRegisters({ &m_normalMap, &m_visualMap }, { "Y" }, id_YankLine);
    AddKeyMapWithCountRegisters({ &m_normalMap }, { "yy" }, id_YankLine);

    // Visual mode
    AddKeyMapWithCountRegisters({ &m_visualMap }, { "aW" }, id_VisualSelectAWORD);
    AddKeyMapWithCountRegisters({ &m_visualMap }, { "aw" }, id_VisualSelectAWord);
    AddKeyMapWithCountRegisters({ &m_visualMap }, { "iW" }, id_VisualSelectInnerWORD);
    AddKeyMapWithCountRegisters({ &m_visualMap }, { "iw" }, id_VisualSelectInnerWord);
}

void ZepMode_Vim::AddPasteMaps()
{
}

void RegisterVimExCommands(ZepEditor& editor);

void ZepMode_Vim::Init()
{
    for (int i = 0; i <= 9; i++)
    {
        GetEditor().SetRegister('0' + (const char)i, "");
    }
    GetEditor().SetRegister('"', "");

    SetupKeyMaps();

    RegisterVimExCommands(GetEditor());
    RegisterTerminalCommands(GetEditor());

    // Apply relative number setting from config
    SetUseRelativeLineNumbers(GetEditor().GetConfig().relativeNumber);
}

void ZepMode_Vim::AddNavigationKeyMaps(bool allowInVisualMode)
{
    std::vector<KeyMap*> navigationMaps = { &m_normalMap };
    if (allowInVisualMode)
    {
        navigationMaps.push_back(&m_visualMap);
    }

    // Up/Down/Left/Right
    AddKeyMapWithCountRegisters(navigationMaps, { "j", "<Down>" }, id_MotionDown);
    AddKeyMapWithCountRegisters(navigationMaps, { "k", "<Up>" }, id_MotionUp);
    AddKeyMapWithCountRegisters(navigationMaps, { "l", "<Right>" }, id_MotionRight);
    AddKeyMapWithCountRegisters(navigationMaps, { "h", "<Left>" }, id_MotionLeft);

    // Page Motions
    AddKeyMapWithCountRegisters(navigationMaps, { "<C-f>", "<PageDown>" }, id_MotionPageForward);
    AddKeyMapWithCountRegisters(navigationMaps, { "<C-b>", "<PageUp>" }, id_MotionPageBackward);
    AddKeyMapWithCountRegisters(navigationMaps, { "<C-d>" }, id_MotionHalfPageForward);
    AddKeyMapWithCountRegisters(navigationMaps, { "<C-u>" }, id_MotionHalfPageBackward);
    AddKeyMapWithCountRegisters(navigationMaps, { "G" }, id_MotionGotoLine);

    // Screen Redraw Motions (z-prefixed)
    AddKeyMapWithCountRegisters(navigationMaps, { "z<CR>", "z<Return>", "zt" }, id_MotionScreenRedrawTop);
    AddKeyMapWithCountRegisters(navigationMaps, { "z-", "zb" }, id_MotionScreenRedrawBottom);
    AddKeyMapWithCountRegisters(navigationMaps, { "zz" }, id_MotionScreenRedrawCenter);

    // Horizontal Scroll (z-prefixed)
    AddKeyMapWithCountRegisters(navigationMaps, { "zH" }, id_MotionScreenScrollHalfLeft);
    AddKeyMapWithCountRegisters(navigationMaps, { "zL" }, id_MotionScreenScrollHalfRight);
    AddKeyMapWithCountRegisters(navigationMaps, { "zh" }, id_MotionScreenScrollLeft);
    AddKeyMapWithCountRegisters(navigationMaps, { "zl" }, id_MotionScreenScrollRight);

    // Line Motions
    AddKeyMapWithCountRegisters(navigationMaps, { "$", "<End>" }, id_MotionLineEnd);
    AddKeyMapWithCountRegisters(navigationMaps, { "^" }, id_MotionLineFirstChar);
    keymap_add(navigationMaps, { "0", "<Home>" }, id_MotionLineBegin);

    // Word motions
    AddKeyMapWithCountRegisters(navigationMaps, { "w" }, id_MotionWord);
    AddKeyMapWithCountRegisters(navigationMaps, { "b" }, id_MotionBackWord);
    AddKeyMapWithCountRegisters(navigationMaps, { "W" }, id_MotionWORD);
    AddKeyMapWithCountRegisters(navigationMaps, { "B" }, id_MotionBackWORD);
    AddKeyMapWithCountRegisters(navigationMaps, { "e" }, id_MotionEndWord);
    AddKeyMapWithCountRegisters(navigationMaps, { "E" }, id_MotionEndWORD);
    AddKeyMapWithCountRegisters(navigationMaps, { "ge" }, id_MotionBackEndWord);
    AddKeyMapWithCountRegisters(navigationMaps, { "gE" }, id_MotionBackEndWORD);
    AddKeyMapWithCountRegisters(navigationMaps, { "gg" }, id_MotionGotoBeginning);

    // Navigate between splits
    keymap_add(navigationMaps, { "<C-j>" }, id_MotionDownSplit);
    keymap_add(navigationMaps, { "<C-l>" }, id_MotionRightSplit);
    keymap_add(navigationMaps, { "<C-k>" }, id_MotionUpSplit);
    keymap_add(navigationMaps, { "<C-h>" }, id_MotionLeftSplit);

    // Arrows always navigate in insert mode
    keymap_add({ &m_insertMap }, { "<Down>" }, id_MotionDown);
    keymap_add({ &m_insertMap }, { "<Up>" }, id_MotionUp);
    keymap_add({ &m_insertMap }, { "<Right>" }, id_MotionRight);
    keymap_add({ &m_insertMap }, { "<Left>" }, id_MotionLeft);

    keymap_add({ &m_insertMap }, { "<End>" }, id_MotionLineBeyondEnd);
    keymap_add({ &m_insertMap }, { "<Home>" }, id_MotionLineBegin);
}

void ZepMode_Vim::AddSearchKeyMaps()
{
    // Normal mode searching
    AddKeyMapWithCountRegisters({ &m_normalMap }, { "f<.>" }, id_Find);
    AddKeyMapWithCountRegisters({ &m_normalMap }, { "F<.>" }, id_FindBackwards);
    AddKeyMapWithCountRegisters({ &m_normalMap }, { ";" }, id_FindNext);
    AddKeyMapWithCountRegisters({ &m_normalMap }, { "%" }, id_FindNextDelimiter);
    AddKeyMapWithCountRegisters({ &m_normalMap }, { "n" }, id_MotionNextSearch);
    AddKeyMapWithCountRegisters({ &m_normalMap }, { "N" }, id_MotionPreviousSearch);
    keymap_add({ &m_normalMap }, { "<F8>" }, id_MotionNextMarker);
    keymap_add({ &m_normalMap }, { "<S-F8>" }, id_MotionPreviousMarker);
}

void ZepMode_Vim::AddGlobalKeyMaps()
{
    // Global bits
    keymap_add({ &m_normalMap, &m_insertMap }, { "<C-p>", "<C-,>" }, id_QuickSearch);
    keymap_add({ &m_normalMap }, { ":", "/", "?" }, id_ExMode);
    keymap_add({ &m_normalMap }, { "H" }, id_PreviousTabWindow);
    keymap_add({ &m_normalMap }, { "L" }, id_NextTabWindow);
    keymap_add({ &m_normalMap }, { "<C-i><C-o>" }, id_SwitchToAlternateFile);
    keymap_add({ &m_normalMap }, { "+" }, id_FontBigger);
    keymap_add({ &m_normalMap }, { "-" }, id_FontSmaller);

    // CTRL + =/- to zoom fonts while in non-normal mode
    keymap_add({ &m_normalMap, &m_visualMap, &m_insertMap }, { "<C-=>" }, id_FontBigger);
    keymap_add({ &m_normalMap, &m_visualMap, &m_insertMap }, { "<C-->" }, id_FontSmaller);

    // Standard clipboard shortcuts
    keymap_add({ &m_normalMap, &m_visualMap, &m_insertMap }, { "<C-c>" }, id_StandardCopy);
    keymap_add({ &m_normalMap, &m_visualMap, &m_insertMap }, { "<C-x>" }, id_StandardCut);
    keymap_add({ &m_normalMap, &m_visualMap, &m_insertMap }, { "<C-v>", "<S-Insert>" }, id_StandardPaste);
}

void ZepMode_Vim::SetupKeyMaps()
{
    // Standard choices
    AddGlobalKeyMaps();
    AddNavigationKeyMaps(true);
    AddSearchKeyMaps();
    AddCopyMaps();
    AddPasteMaps();
    AddOverStrikeMaps();

    // Mode switching
    AddKeyMapWithCountRegisters({ &m_normalMap, &m_visualMap }, { "<Escape>" }, id_NormalMode);
    keymap_add({ &m_insertMap }, { "jk" }, id_NormalMode);
    keymap_add({ &m_insertMap }, { "<Escape>" }, id_NormalMode);

    AddKeyMapWithCountRegisters({ &m_normalMap }, { "i" }, id_InsertMode);
    AddKeyMapWithCountRegisters({ &m_normalMap }, { "S" }, id_SubstituteLine);
    AddKeyMapWithCountRegisters({ &m_normalMap }, { "s" }, id_Substitute);
    AddKeyMapWithCountRegisters({ &m_normalMap }, { "A" }, id_AppendToLine);
    AddKeyMapWithCountRegisters({ &m_normalMap }, { "a" }, id_Append);
    AddKeyMapWithCountRegisters({ &m_normalMap }, { "I" }, id_InsertAtFirstChar);
    AddKeyMapWithCountRegisters({ &m_visualMap }, { ":", "/", "?" }, id_ExMode);

    // Copy/paste
    AddKeyMapWithCountRegisters({ &m_normalMap, &m_visualMap }, { "p" }, id_PasteAfter);
    AddKeyMapWithCountRegisters({ &m_normalMap, &m_visualMap }, { "P" }, id_PasteBefore);
    AddKeyMapWithCountRegisters({ &m_normalMap, &m_visualMap }, { "x", "<Del>" }, id_Delete);

    // Visual changes
    AddKeyMapWithCountRegisters({ &m_visualMap }, { "d" }, id_VisualDelete);
    AddKeyMapWithCountRegisters({ &m_visualMap }, { "c" }, id_VisualChange);
    AddKeyMapWithCountRegisters({ &m_visualMap }, { "s" }, id_VisualSubstitute);

    // Line modifications
    AddKeyMapWithCountRegisters({ &m_normalMap, &m_visualMap }, { "J" }, id_JoinLines);
    AddKeyMapWithCountRegisters({ &m_visualMap }, { "C" }, id_ChangeLine);
    AddKeyMapWithCountRegisters({ &m_normalMap }, { "o" }, id_OpenLineBelow);
    AddKeyMapWithCountRegisters({ &m_normalMap }, { "O" }, id_OpenLineAbove);

    // Word modification/text
    AddKeyMapWithCountRegisters({ &m_normalMap }, { "d<D>w", "dw" }, id_DeleteWord);
    AddKeyMapWithCountRegisters({ &m_normalMap }, { "dW" }, id_DeleteWORD);
    AddKeyMapWithCountRegisters({ &m_normalMap }, { "daw" }, id_DeleteAWord);
    AddKeyMapWithCountRegisters({ &m_normalMap }, { "daW" }, id_DeleteAWORD);
    AddKeyMapWithCountRegisters({ &m_normalMap }, { "diw" }, id_DeleteInnerWord);
    AddKeyMapWithCountRegisters({ &m_normalMap }, { "diW" }, id_DeleteInnerWORD);
    AddKeyMapWithCountRegisters({ &m_normalMap }, { "D", "d$" }, id_DeleteToLineEnd);
    AddKeyMapWithCountRegisters({ &m_normalMap }, { "d<D>d", "dd" }, id_DeleteLine);
    AddKeyMapWithCountRegisters({ &m_normalMap }, { "dt<.>" }, id_DeleteToChar);

    AddKeyMapWithCountRegisters({ &m_normalMap }, { "cw" }, id_ChangeWord);
    AddKeyMapWithCountRegisters({ &m_normalMap }, { "cW" }, id_ChangeWORD);
    AddKeyMapWithCountRegisters({ &m_normalMap }, { "ciw" }, id_ChangeInnerWord);
    AddKeyMapWithCountRegisters({ &m_normalMap }, { "ciW" }, id_ChangeInnerWORD);
    AddKeyMapWithCountRegisters({ &m_normalMap }, { "ci<.>" }, id_ChangeIn);
    AddKeyMapWithCountRegisters({ &m_normalMap }, { "caw" }, id_ChangeAWord);
    AddKeyMapWithCountRegisters({ &m_normalMap }, { "caW" }, id_ChangeAWORD);
    AddKeyMapWithCountRegisters({ &m_normalMap }, { "C", "c$" }, id_ChangeToLineEnd);
    AddKeyMapWithCountRegisters({ &m_normalMap }, { "cc" }, id_ChangeLine);
    AddKeyMapWithCountRegisters({ &m_normalMap }, { "ct<.>" }, id_ChangeToChar);

    AddKeyMapWithCountRegisters({ &m_normalMap }, { "<Return>" }, id_MotionNextFirstChar);

    AddKeyMapWithCountRegisters({ &m_normalMap, &m_visualMap }, { "<C-r>" }, id_Redo);
    AddKeyMapWithCountRegisters({ &m_normalMap, &m_visualMap }, { "<C-z>", "u" }, id_Undo);

    keymap_add({ &m_normalMap }, { "<Backspace>" }, id_MotionStandardLeft);

    // No count allowed on backspace in insert mode, or that would interfere with text.
    keymap_add({ &m_insertMap }, { "<Backspace>" }, id_Backspace);
    keymap_add({ &m_insertMap }, { "<Del>" }, id_Delete);

    keymap_add({ &m_insertMap }, { "<Return>" }, id_InsertCarriageReturn);
    keymap_add({ &m_insertMap }, { "<Tab>" }, id_InsertTab);
    keymap_add({ &m_insertMap }, { "<S-Tab>" }, id_Dedent);

    // Macro recording and playback
    AddKeyMapWithCountRegisters({ &m_normalMap }, { "q<.>" }, id_MacroRecord);
    keymap_add({ &m_normalMap }, { "q" }, id_MacroRecord);
    AddKeyMapWithCountRegisters({ &m_normalMap }, { "@<.>" }, id_MacroPlay);

    // Folding
    AddKeyMapWithCountRegisters({ &m_normalMap, &m_visualMap }, { "zf" }, id_FoldCreate);
    AddKeyMapWithCountRegisters({ &m_normalMap }, { "zd" }, id_FoldDelete);
    AddKeyMapWithCountRegisters({ &m_normalMap }, { "zD" }, id_FoldDeleteAll);
    AddKeyMapWithCountRegisters({ &m_normalMap }, { "zo" }, id_FoldOpen);
    AddKeyMapWithCountRegisters({ &m_normalMap }, { "zO" }, id_FoldOpenAll);
    AddKeyMapWithCountRegisters({ &m_normalMap }, { "zc" }, id_FoldClose);
    AddKeyMapWithCountRegisters({ &m_normalMap }, { "zC" }, id_FoldCloseAll);
    AddKeyMapWithCountRegisters({ &m_normalMap }, { "zR" }, id_FoldOpenAll);
    AddKeyMapWithCountRegisters({ &m_normalMap }, { "zM" }, id_FoldCloseAll);
    AddKeyMapWithCountRegisters({ &m_normalMap }, { "za" }, id_FoldToggle);

    // Multi-cursor keys - Ctrl+d adds cursor at next word, Ctrl+k skips
    AddKeyMapWithCountRegisters({ &m_normalMap }, { "<C-d>", "<C-d>" }, id_MultiCursorAdd);
    AddKeyMapWithCountRegisters({ &m_normalMap }, { "<C-k>" }, id_MultiCursorSkip);
    AddKeyMapWithCountRegisters({ &m_normalMap }, { "<C-S-d>" }, id_MultiCursorSelectAll);
}

void ZepMode_Vim::Begin(ZepWindow* pWindow)
{
    ZepMode::Begin(pWindow);

    GetEditor().SetCommandText(m_currentCommand);
    m_currentMode = EditorMode::Normal;
    m_currentCommand.clear();
    m_dotCommand.clear();
}

void ZepMode_Vim::PreDisplay(ZepWindow& window)
{
    // After .25 seconds of not pressing the 'k' escape code after j,
    // put the j in.
    // We can do better than this and fix the keymapper to handle timed key events.
    // This is an easier fix for now
    if (timer_get_elapsed_seconds(m_lastKeyPressTimer) > .25f && m_currentMode == EditorMode::Insert && m_currentCommand == "j")
    {
        auto cmd = std::make_shared<ZepCommand_Insert>(
            window.GetBuffer(),
            window.GetBufferCursor(),
            m_currentCommand);
        AddCommand(cmd);

        m_currentCommand = "";
    }
}

class ZepExCommand_Substitute : public ZepExCommand
{
public:
    ZepExCommand_Substitute(ZepEditor& editor)
        : ZepExCommand(editor)
    {
    }

    const char* ExCommandName() const override
    {
        return "s";
    }

    void Run(const std::vector<std::string>& args) override
    {
        if (args.size() < 2)
        {
            GetEditor().SetCommandText("Usage: :s/pattern/replacement/[flags]");
            return;
        }

        ZepBuffer& buffer = GetEditor().GetActiveWindow()->GetBuffer();
        auto cursor = GetEditor().GetActiveWindow()->GetBufferCursor();
        long currentLine = buffer.GetBufferLine(cursor);

        std::string cmdArgs = args[1];

        bool globalAll = false;
        long lineCount = 1;
        long startLine = currentLine;

        if (cmdArgs == "%")
        {
            globalAll = true;
            startLine = 0;
            lineCount = buffer.GetLineCount();
        }
        else if (cmdArgs.find('%') == 0)
        {
            globalAll = true;
            cmdArgs = cmdArgs.substr(1);
            startLine = 0;
            lineCount = buffer.GetLineCount();
        }
        else if (cmdArgs.find('%') != std::string::npos)
        {
            globalAll = true;
            cmdArgs = string_replace(cmdArgs, "%", "");
            startLine = 0;
            lineCount = buffer.GetLineCount();
        }
        else
        {
            long lineNum = 0;
            for (char c : cmdArgs)
            {
                if (isdigit(c))
                {
                    lineNum = lineNum * 10 + (c - '0');
                }
            }
            if (lineNum > 0)
            {
                startLine = lineNum - 1;
                lineCount = lineNum;
                cmdArgs = cmdArgs.substr(cmdArgs.find_first_not_of("0123456789"));
            }
        }

        if (cmdArgs.empty())
        {
            GetEditor().SetCommandText("Usage: :s/pattern/replacement/[flags]");
            return;
        }

        if (cmdArgs[0] == 's' && cmdArgs.length() > 1)
        {
            cmdArgs = cmdArgs.substr(1);
        }

        if (cmdArgs.empty() || cmdArgs[0] != '/')
        {
            GetEditor().SetCommandText("Usage: :s/pattern/replacement/[flags]");
            return;
        }

        size_t firstSlash = 1;
        size_t secondSlash = std::string::npos;
        for (size_t i = 1; i < cmdArgs.length(); i++)
        {
            if (cmdArgs[i] == '/' && (i == 1 || cmdArgs[i - 1] != '\\'))
            {
                secondSlash = i;
                break;
            }
        }

        if (secondSlash == std::string::npos)
        {
            GetEditor().SetCommandText("Invalid format. Use :s/pattern/replacement/[flags]");
            return;
        }

        std::string searchPattern = cmdArgs.substr(firstSlash, secondSlash - firstSlash);
        std::string replacement = cmdArgs.substr(secondSlash + 1);
        std::string flags;

        size_t flagStart = secondSlash + 1;
        if (flagStart < cmdArgs.length())
        {
            flags = cmdArgs.substr(flagStart);
        }

        bool doGlobal = false;
        bool doConfirm = false;
        bool ignoreCase = false;

        for (char f : flags)
        {
            if (f == 'g')
                doGlobal = true;
            else if (f == 'c')
                doConfirm = true;
            else if (f == 'i')
                ignoreCase = true;
        }

        if (ignoreCase)
        {
            searchPattern = string_tolower(searchPattern);
        }

        long substitutions = 0;

        for (long line = startLine; line < startLine + lineCount && line < buffer.GetLineCount(); line++)
        {
            ByteRange range;
            if (!buffer.GetLineOffsets(line, range))
            {
                continue;
            }

            GlyphIterator lineStart(&buffer, range.first);
            GlyphIterator lineEnd(&buffer, range.second);

            std::string lineText = buffer.GetBufferText(lineStart, lineEnd);
            std::string searchText = lineText;
            if (ignoreCase)
            {
                searchText = string_tolower(searchText);
            }

            size_t pos = 0;

            while (true)
            {
                pos = searchText.find(searchPattern, pos);
                if (pos == std::string::npos)
                {
                    break;
                }

                if (doConfirm)
                {
                    GetEditor().SetCommandText("Replace at " + std::to_string(line + 1) + "? [y/n/a/q]");
                }

                GlyphIterator replaceStart(&buffer, range.first + pos);
                GlyphIterator replaceEnd(&buffer, range.first + pos + searchPattern.length());

                if (doConfirm)
                {
                    GetEditor().SetCommandText("Replace (y/n/a/q)? ");
                }
                else
                {
                    ChangeRecord changeRecord;
                    buffer.Replace(replaceStart, replaceEnd, replacement, ReplaceRangeMode::Replace, changeRecord);
                    substitutions++;
                }

                if (!doGlobal)
                {
                    break;
                }

                pos += replacement.length();
                if (pos >= searchText.length())
                {
                    break;
                }

                if (!buffer.GetLineOffsets(line, range))
                {
                    break;
                }

                lineStart = GlyphIterator(&buffer, range.first);
                lineEnd = GlyphIterator(&buffer, range.second);
                searchText = buffer.GetBufferText(lineStart, lineEnd);
                if (ignoreCase)
                {
                    searchText = string_tolower(searchText);
                }
            }
        }

        GetEditor().SetCommandText(std::to_string(substitutions) + " substitution(s)");

        if (substitutions > 0)
        {
            buffer.SetFileFlags((uint32_t)FileFlags::Dirty, true);
        }
    }
};

class ZepExCommand_Quit : public ZepExCommand
{
public:
    ZepExCommand_Quit(ZepEditor& editor)
        : ZepExCommand(editor)
    {
    }

    const char* ExCommandName() const override
    {
        return "q";
    }

    void Run(const std::vector<std::string>& args) override
    {
        auto pWindow = GetEditor().GetActiveWindow();
        if (!pWindow)
            return;

        if (args.size() > 1 && args[1] == "a")
        {
            auto pTab = GetEditor().GetActiveTabWindow();
            while (pTab->GetWindows().size() > 1)
            {
                pTab->CloseActiveWindow();
            }
            if (!pTab->GetWindows().empty())
            {
                pTab->CloseActiveWindow();
            }
        }
        else if (args.size() > 1 && args[1] == "!")
        {
            pWindow->GetBuffer().ClearFileFlags(FileFlags::Dirty);
            pWindow->GetTabWindow().CloseActiveWindow();
        }
        else
        {
            ZepBuffer& buffer = pWindow->GetBuffer();
            if (buffer.HasFileFlags(FileFlags::Dirty))
            {
                GetEditor().SetCommandText("E37: No write since last change (add ! to override)");
                return;
            }
            pWindow->GetTabWindow().CloseActiveWindow();
        }
    }
};

class ZepExCommand_Write : public ZepExCommand
{
public:
    ZepExCommand_Write(ZepEditor& editor)
        : ZepExCommand(editor)
    {
    }

    const char* ExCommandName() const override
    {
        return "w";
    }

    void Run(const std::vector<std::string>& args) override
    {
        auto pWindow = GetEditor().GetActiveWindow();
        if (!pWindow)
            return;

        ZepBuffer& buffer = pWindow->GetBuffer();

        if (args.size() > 1 && args[1] != "q")
        {
            fs::path filePath = args[1];
            GetEditor().SaveBufferAs(buffer, filePath);
            GetEditor().SetCommandText("'" + filePath.filename().string() + "' " + std::to_string(buffer.GetLineCount()) + "L written");
        }
        else
        {
            GetEditor().SaveBuffer(buffer);
            GetEditor().SetCommandText("'" + buffer.GetDisplayName() + "' " + std::to_string(buffer.GetLineCount()) + "L written");
        }
    }
};

class ZepExCommand_WriteQuit : public ZepExCommand
{
public:
    ZepExCommand_WriteQuit(ZepEditor& editor)
        : ZepExCommand(editor)
    {
    }

    const char* ExCommandName() const override
    {
        return "wq";
    }

    void Run(const std::vector<std::string>& args) override
    {
        auto pWindow = GetEditor().GetActiveWindow();
        if (!pWindow)
            return;

        ZepBuffer& buffer = pWindow->GetBuffer();

        if (args.size() > 1 && args[1] != "q")
        {
            fs::path filePath = args[1];
            GetEditor().SaveBufferAs(buffer, filePath);
        }
        else
        {
            GetEditor().SaveBuffer(buffer);
        }
        pWindow->GetTabWindow().CloseActiveWindow();
    }
};

class ZepExCommand_ListBuffers : public ZepExCommand
{
public:
    ZepExCommand_ListBuffers(ZepEditor& editor)
        : ZepExCommand(editor)
    {
    }

    const char* ExCommandName() const override
    {
        return "buffers";
    }

    void Run(const std::vector<std::string>& args) override
    {
        std::ostringstream str;
        auto buffers = GetEditor().GetBuffers();
        auto pActiveBuffer = GetEditor().GetActiveBuffer();

        int idx = 0;
        for (auto& buff : buffers)
        {
            str << (buff.get() == pActiveBuffer ? "%" : " ");
            str << "  " << ++idx << " ";

            if (buff->HasFileFlags(FileFlags::Dirty))
                str << "+";
            else
                str << " ";

            str << " \"" << buff->GetDisplayName() << "\"" << "\n";
        }

        GetEditor().SetCommandText(str.str());
    }
};

class ZepExCommand_Ls : public ZepExCommand
{
public:
    ZepExCommand_Ls(ZepEditor& editor)
        : ZepExCommand(editor)
    {
    }

    const char* ExCommandName() const override
    {
        return "ls";
    }

    void Run(const std::vector<std::string>& args) override
    {
        ZepExCommand_ListBuffers cmd(GetEditor());
        cmd.Run(args);
    }
};

class ZepExCommand_Global : public ZepExCommand
{
public:
    ZepExCommand_Global(ZepEditor& editor)
        : ZepExCommand(editor)
    {
    }

    const char* ExCommandName() const override
    {
        return "g";
    }

    void Run(const std::vector<std::string>& args) override
    {
        if (args.size() < 2)
        {
            GetEditor().SetCommandText("Usage: :g/pattern/command");
            return;
        }

        auto pWindow = GetEditor().GetActiveWindow();
        if (!pWindow)
            return;

        std::string cmdArgs = args[1];
        if (cmdArgs.empty() || cmdArgs[0] != '/')
        {
            GetEditor().SetCommandText("Usage: :g/pattern/command");
            return;
        }

        size_t secondSlash = std::string::npos;
        for (size_t i = 1; i < cmdArgs.length(); i++)
        {
            if (cmdArgs[i] == '/' && (i == 1 || cmdArgs[i - 1] != '\\'))
            {
                secondSlash = i;
                break;
            }
        }

        if (secondSlash == std::string::npos)
        {
            GetEditor().SetCommandText("Invalid pattern. Use :g/pattern/command");
            return;
        }

        std::string pattern = cmdArgs.substr(1, secondSlash - 1);
        std::string subCommand;
        if (secondSlash + 1 < cmdArgs.length())
        {
            subCommand = cmdArgs.substr(secondSlash + 1);
        }

        ZepBuffer& buffer = pWindow->GetBuffer();
        std::vector<long> matchingLines;

        bool ignoreCase = false;
        std::string searchPattern = pattern;
        if (!pattern.empty() && pattern[0] == '\\' && pattern.length() > 1)
        {
            if (pattern[1] == 'c')
            {
                ignoreCase = true;
                searchPattern = pattern.substr(2);
            }
        }

        if (ignoreCase)
        {
            searchPattern = string_tolower(searchPattern);
        }

        for (long line = 0; line < buffer.GetLineCount(); line++)
        {
            ByteRange range;
            if (!buffer.GetLineOffsets(line, range))
                continue;

            GlyphIterator lineStart(&buffer, range.first);
            GlyphIterator lineEnd(&buffer, range.second);
            std::string lineText = buffer.GetBufferText(lineStart, lineEnd);

            std::string searchText = lineText;
            if (ignoreCase)
                searchText = string_tolower(searchText);

            if (searchText.find(searchPattern) != std::string::npos)
            {
                matchingLines.push_back(line);
            }
        }

        long count = 0;
        if (subCommand == "d" || subCommand == "delete")
        {
            for (auto it = matchingLines.rbegin(); it != matchingLines.rend(); ++it)
            {
                long line = *it;
                ByteRange range;
                if (buffer.GetLineOffsets(line, range))
                {
                    GlyphIterator start(&buffer, range.first);
                    GlyphIterator end(&buffer, range.second);
                    ChangeRecord changeRecord;
                    buffer.Delete(start, end, changeRecord);
                    count++;
                }
            }
            GetEditor().SetCommandText(std::to_string(count) + " line(s) deleted");
        }
        else if (subCommand == "p" || subCommand == "print")
        {
            std::ostringstream str;
            for (long line : matchingLines)
            {
                ByteRange range;
                if (buffer.GetLineOffsets(line, range))
                {
                    GlyphIterator lineStart(&buffer, range.first);
                    GlyphIterator lineEnd(&buffer, range.second);
                    std::string lineText = buffer.GetBufferText(lineStart, lineEnd);
                    str << line + 1 << " " << lineText << "\n";
                }
            }
            GetEditor().SetCommandText(str.str());
        }
        else if (!subCommand.empty())
        {
            GetEditor().SetCommandText("Unsupported command: " + subCommand);
        }
        else
        {
            GetEditor().SetCommandText(std::to_string(matchingLines.size()) + " match(es) for pattern");
        }
    }
};

class ZepExCommand_Next : public ZepExCommand
{
public:
    ZepExCommand_Next(ZepEditor& editor)
        : ZepExCommand(editor)
    {
    }

    const char* ExCommandName() const override
    {
        return "n";
    }

    void Run(const std::vector<std::string>& args) override
    {
        auto buffers = GetEditor().GetBuffers();
        if (buffers.size() <= 1)
        {
            GetEditor().SetCommandText("No other file in buffer list");
            return;
        }

        auto pCurrentBuffer = GetEditor().GetActiveBuffer();
        auto itr = std::find_if(buffers.begin(), buffers.end(), [pCurrentBuffer](const std::shared_ptr<ZepBuffer>& buff) {
            return buff.get() == pCurrentBuffer;
        });

        if (itr != buffers.end())
        {
            ++itr;
            if (itr == buffers.end())
                itr = buffers.begin();
            GetEditor().EnsureWindow(**itr);
        }
    }
};

// ============================================================================
// :e[dit] {file} -- Edit file in current window
// Vim behavior: Load the file into the current window, replacing the current
// buffer's contents. If the file is already open in another buffer, switch
// to that buffer. Does NOT split the screen.
// ============================================================================
class ZepExCommand_Edit : public ZepExCommand
{
public:
    ZepExCommand_Edit(ZepEditor& editor)
        : ZepExCommand(editor)
    {
    }

    const char* ExCommandName() const override
    {
        return "e";
    }

    void Run(const std::vector<std::string>& args) override
    {
        if (args.size() < 2)
        {
            GetEditor().SetCommandText("E471: Argument required");
            return;
        }

        fs::path filePath = args[1];
        // Get or create the buffer for the file
        auto pBuffer = GetEditor().GetFileBuffer(filePath);
        if (!pBuffer)
        {
            GetEditor().SetCommandText("E484: Cannot open file");
            return;
        }

        // Switch the active window to this buffer (no split)
        auto pTab = GetEditor().GetActiveTabWindow();
        if (!pTab)
            return;

        auto pActiveWindow = pTab->GetActiveWindow();
        if (pActiveWindow)
        {
            pActiveWindow->SetBuffer(pBuffer);
        }
    }
};

class ZepExCommand_BufferNext : public ZepExCommand
{
public:
    ZepExCommand_BufferNext(ZepEditor& editor)
        : ZepExCommand(editor)
    {
    }

    const char* ExCommandName() const override
    {
        return "bn";
    }

    void Run(const std::vector<std::string>& args) override
    {
        auto buffers = GetEditor().GetBuffers();
        if (buffers.size() <= 1)
        {
            GetEditor().SetCommandText("E85: No next buffer");
            return;
        }

        auto pCurrentBuffer = GetEditor().GetActiveBuffer();
        auto itr = std::find_if(buffers.begin(), buffers.end(), [pCurrentBuffer](const std::shared_ptr<ZepBuffer>& buff) {
            return buff.get() == pCurrentBuffer;
        });

        if (itr != buffers.end())
        {
            ++itr;
            if (itr == buffers.end())
                itr = buffers.begin();
            GetEditor().EnsureWindow(**itr);
        }
    }
};

class ZepExCommand_BufferPrev : public ZepExCommand
{
public:
    ZepExCommand_BufferPrev(ZepEditor& editor)
        : ZepExCommand(editor)
    {
    }

    const char* ExCommandName() const override
    {
        return "bp";
    }

    void Run(const std::vector<std::string>& args) override
    {
        auto buffers = GetEditor().GetBuffers();
        if (buffers.size() <= 1)
        {
            GetEditor().SetCommandText("E85: No previous buffer");
            return;
        }

        auto pCurrentBuffer = GetEditor().GetActiveBuffer();
        auto itr = std::find_if(buffers.begin(), buffers.end(), [pCurrentBuffer](const std::shared_ptr<ZepBuffer>& buff) {
            return buff.get() == pCurrentBuffer;
        });

        if (itr != buffers.end())
        {
            if (itr == buffers.begin())
                itr = buffers.end();
            --itr;
            GetEditor().EnsureWindow(**itr);
        }
    }
};

class ZepExCommand_BufferGoto : public ZepExCommand
{
public:
    ZepExCommand_BufferGoto(ZepEditor& editor)
        : ZepExCommand(editor)
    {
    }

    const char* ExCommandName() const override
    {
        return "b";
    }

    void Run(const std::vector<std::string>& args) override
    {
        if (args.size() < 2)
        {
            GetEditor().SetCommandText("E86: Buffer number or name required");
            return;
        }

        auto buffers = GetEditor().GetBuffers();
        std::string arg = args[1];

        if (std::all_of(arg.begin(), arg.end(), ::isdigit))
        {
            int bufNum = std::stoi(arg);
            if (bufNum < 1 || bufNum > (int)buffers.size())
            {
                GetEditor().SetCommandText("E86: Invalid buffer number");
                return;
            }
            GetEditor().EnsureWindow(*buffers[bufNum - 1]);
        }
        else
        {
            auto itr = std::find_if(buffers.begin(), buffers.end(), [&arg](const std::shared_ptr<ZepBuffer>& buff) {
                return buff->GetDisplayName().find(arg) != std::string::npos;
            });

            if (itr != buffers.end())
            {
                GetEditor().EnsureWindow(**itr);
            }
            else
            {
                GetEditor().SetCommandText("E86: No buffer with name: " + arg);
            }
        }
    }
};

// ============================================================================
// :sp[lit] [file] -- Split window horizontally
// Vim behavior: Split current window horizontally. If [file] given, load it
// in the new window. Otherwise, show current buffer in the new window.
// ============================================================================
class ZepExCommand_Split : public ZepExCommand
{
public:
    ZepExCommand_Split(ZepEditor& editor)
        : ZepExCommand(editor)
    {
    }

    const char* ExCommandName() const override
    {
        return "sp";
    }

    void Run(const std::vector<std::string>& args) override
    {
        auto pTab = GetEditor().GetActiveTabWindow();
        if (!pTab)
            return;

        ZepBuffer* pBuffer = nullptr;
        if (args.size() >= 2)
        {
            fs::path filePath = args[1];
            pBuffer = GetEditor().GetFileBuffer(filePath);
        }
        else
        {
            // No filename: split and show current buffer in new window
            pBuffer = GetEditor().GetActiveBuffer();
            if (!pBuffer)
                return;
        }

        if (pBuffer)
        {
            // VBox = vertical box = children stacked vertically = horizontal split
            pTab->AddWindow(pBuffer, nullptr, RegionLayoutType::VBox);
        }
    }
};

// ============================================================================
// :vsp[lit] [file] -- Split window vertically (side-by-side)
// Vim behavior: Split current window vertically. If [file] given, load it
// in the new window. Otherwise, show current buffer in the new window.
// ============================================================================
class ZepExCommand_VSplit : public ZepExCommand
{
public:
    ZepExCommand_VSplit(ZepEditor& editor)
        : ZepExCommand(editor)
    {
    }

    const char* ExCommandName() const override
    {
        return "vsplit";
    }

    void Run(const std::vector<std::string>& args) override
    {
        auto pEditor = &GetEditor();
        auto pTab = pEditor->GetActiveTabWindow();
        if (!pTab)
            return;

        ZepBuffer* pBuffer = nullptr;
        if (args.size() >= 2)
        {
            fs::path filePath = args[1];
            pBuffer = pEditor->GetFileBuffer(filePath);
        }
        else
        {
            // No filename: split and show current buffer in new window
            pBuffer = pEditor->GetActiveBuffer();
            if (!pBuffer)
                return;
        }

        if (pBuffer)
        {
            // Use active window as parent to split it vertically (side-by-side)
            auto pParent = pTab->GetActiveWindow();
            pTab->AddWindow(pBuffer, pParent, RegionLayoutType::HBox);
        }
    }
};

// ============================================================================
// :r[ead] {file} -- Read file and insert after current line
// Vim behavior: Read the contents of {file} and insert them after the cursor
// line in the current buffer. If the file cannot be read, show an error.
// ============================================================================
// ============================================================================
// :r[ead] {file} -- Read file and insert after current line
// Vim behavior: Read the contents of {file} and insert them after the cursor
// line in the current buffer. If the file cannot be read, show an error.
// ============================================================================
// ============================================================================
// :r[ead] {file} -- Read file and insert after current line
// Vim behavior: Read the contents of {file} and insert them after the cursor
// line in the current buffer. If the file cannot be read, show an error.
// ============================================================================
class ZepExCommand_Read : public ZepExCommand
{
public:
    ZepExCommand_Read(ZepEditor& editor)
        : ZepExCommand(editor)
    {
    }

    const char* ExCommandName() const override
    {
        return "r";
    }

    void Run(const std::vector<std::string>& args) override
    {
        if (args.size() < 2)
        {
            GetEditor().SetCommandText("E484: No filename given");
            return;
        }

        fs::path filePath = args[1];
        auto& fs = GetEditor().GetFileSystem();

        // Check file exists
        if (!fs.Exists(filePath))
        {
            GetEditor().SetCommandText("E484: Cannot open file \"" + filePath.string() + "\"");
            return;
        }

        // Read file contents
        std::string content;
        try
        {
            content = fs.Read(filePath);
        }
        catch (...)
        {
            GetEditor().SetCommandText("E484: Cannot read file");
            return;
        }

        // Get active window and buffer
        auto pTab = GetEditor().GetActiveTabWindow();
        if (!pTab)
        {
            GetEditor().SetCommandText("E444: Cannot get active window");
            return;
        }
        auto pWindow = pTab->GetActiveWindow();
        if (!pWindow)
        {
            GetEditor().SetCommandText("E444: Cannot get active window");
            return;
        }
        ZepBuffer* pBuffer = &pWindow->GetBuffer();

        // Determine insertion point: after current line (like Vim)
        GlyphIterator cursor = pWindow->GetBufferCursor();
        long currentLine = pBuffer->GetBufferLine(cursor);
        long insertLine = currentLine + 1;

        GlyphIterator insertPos;
        if (insertLine >= pBuffer->GetLineCount())
        {
            insertPos = pBuffer->End();
        }
        else
        {
            ByteRange range;
            if (pBuffer->GetLineOffsets(insertLine, range))
            {
                insertPos = GlyphIterator(pBuffer, static_cast<unsigned long>(range.first));
            }
            else
            {
                insertPos = pBuffer->End();
            }
        }

        ChangeRecord record;
        pBuffer->Insert(insertPos, content, record);
        // Note: Insert updates buffer state; undo may not be implemented yet
        GetEditor().SetCommandText("Read " + filePath.string());
    }
};

class ZepExCommand_TabNew : public ZepExCommand
{
public:
    ZepExCommand_TabNew(ZepEditor& editor)
        : ZepExCommand(editor)
    {
    }

    const char* ExCommandName() const override
    {
        return "tabnew";
    }

    void Run(const std::vector<std::string>& args) override
    {
        ZEP_UNUSED(args);
        auto pEditor = &GetEditor();
        auto pNewTab = pEditor->AddTabWindow();
        // Optionally open a file in the new tab if argument provided
        if (args.size() >= 2)
        {
            fs::path filePath = args[1];
            auto pBuffer = pEditor->GetFileBuffer(filePath);
            if (pBuffer)
            {
                pNewTab->AddWindow(pBuffer, nullptr, RegionLayoutType::HBox);
            }
        }
        pEditor->SetCurrentTabWindow(pNewTab);
    }
};

class ZepExCommand_TabClose : public ZepExCommand
{
public:
    ZepExCommand_TabClose(ZepEditor& editor)
        : ZepExCommand(editor)
    {
    }

    const char* ExCommandName() const override
    {
        return "tabclose";
    }

    void Run(const std::vector<std::string>& args) override
    {
        ZEP_UNUSED(args);
        auto pTab = GetEditor().GetActiveTabWindow();
        if (!pTab)
            return;

        // Close the active window in this tab; if it's the last window, the tab will be removed
        // Actually to close the entire tab, we can directly remove it
        GetEditor().RemoveTabWindow(pTab);
    }
};

class ZepExCommand_Set : public ZepExCommand
{
public:
    ZepExCommand_Set(ZepEditor& editor)
        : ZepExCommand(editor)
    {
    }

    const char* ExCommandName() const override
    {
        return "set";
    }

    void Run(const std::vector<std::string>& args) override
    {
        if (args.size() < 2)
        {
            ShowAllOptions();
            return;
        }

        std::string opt = args[1];

        // Query option (ending with ?)
        if (!opt.empty() && opt.back() == '?')
        {
            opt.pop_back();
            QueryOption(opt);
            return;
        }

        // Set option
        SetOption(opt);
    }

private:
    void ShowAllOptions()
    {
        std::ostringstream str;
        str << "    number: " << (GetEditor().GetConfig().showLineNumbers ? "on" : "off") << "\n";
        str << "  relativenumber: " << (IsRelativeNumberEnabled() ? "on" : "off") << "\n";
        str << "        list: " << (IsFlagEnabled(WindowFlags::ShowWhiteSpace) ? "on" : "off") << "\n";
        str << "        wrap: " << (IsFlagEnabled(WindowFlags::WrapText) ? "on" : "off") << "\n";
        str << "  autoindent: " << (ZTestFlags(GetEditor().GetFlags(), ZepEditorFlags::AutoIndent) ? "on" : "off") << "\n";
        str << "   expandtab: " << (IsInsertTabsEnabled() ? "off" : "on") << "\n"; // insert spaces (no expandtab) -> InsertTabs = false -> expandtab
        str << "    tabstop: " << GetEditor().GetConfig().tabStop << "\n";
        str << "  shiftwidth: " << GetEditor().GetConfig().shiftWidth << "\n";
        str << "    minimap: " << (GetEditor().GetConfig().showMinimap ? "on" : "off") << "\n";
        str << "       font: " << GetEditor().GetDisplay().GetCurrentFontName() << "\n";

        GetEditor().SetCommandText(str.str());
    }

    void QueryOption(const std::string& opt)
    {
        std::ostringstream str;

        if (opt == "number")
        {
            str << (GetEditor().GetConfig().showLineNumbers ? "number" : "nonumber");
        }
        else if (opt == "relativenumber")
        {
            str << (IsRelativeNumberEnabled() ? "relativenumber" : "norelativenumber");
        }
        else if (opt == "list")
        {
            str << (IsFlagEnabled(WindowFlags::ShowWhiteSpace) ? "list" : "nolist");
        }
        else if (opt == "wrap")
        {
            str << (IsFlagEnabled(WindowFlags::WrapText) ? "wrap" : "nowrap");
        }
        else if (opt == "autoindent")
        {
            str << (ZTestFlags(GetEditor().GetFlags(), ZepEditorFlags::AutoIndent) ? "autoindent" : "noautoindent");
        }
        else if (opt == "expandtab")
        {
            str << (IsInsertTabsEnabled() ? "noexpandtab" : "expandtab");
        }
        else if (opt == "tabstop")
        {
            str << "tabstop=" << GetEditor().GetConfig().tabStop;
        }
        else if (opt == "shiftwidth")
        {
            str << "shiftwidth=" << GetEditor().GetConfig().shiftWidth;
        }
        else if (opt == "minimap")
        {
            str << (GetEditor().GetConfig().showMinimap ? "minimap" : "nominimap");
        }
        else if (opt == "font")
        {
            str << "font=" << GetEditor().GetDisplay().GetCurrentFontName();
        }
        else
        {
            str << "No such option: " << opt;
        }

        GetEditor().SetCommandText(str.str());
    }

    void SetOption(const std::string& opt)
    {
        if (opt == "number")
        {
            SetShowLineNumbers(true);
        }
        else if (opt == "nonumber")
        {
            SetShowLineNumbers(false);
        }
        else if (opt == "relativenumber")
        {
            SetRelativeNumber(true);
        }
        else if (opt == "norelativenumber")
        {
            SetRelativeNumber(false);
        }
        else if (opt == "list")
        {
            SetFlag(WindowFlags::ShowWhiteSpace, true);
        }
        else if (opt == "nolist")
        {
            SetFlag(WindowFlags::ShowWhiteSpace, false);
        }
        else if (opt == "wrap")
        {
            SetFlag(WindowFlags::WrapText, true);
        }
        else if (opt == "nowrap")
        {
            SetFlag(WindowFlags::WrapText, false);
        }
        else if (opt == "autoindent")
        {
            GetEditor().SetFlags(GetEditor().GetFlags() | ZepEditorFlags::AutoIndent);
        }
        else if (opt == "noautoindent")
        {
            GetEditor().SetFlags(GetEditor().GetFlags() & ~ZepEditorFlags::AutoIndent);
        }
        else if (opt == "expandtab")
        {
            SetInsertTabs(false); // expandtab = use spaces -> InsertTabs = false
        }
        else if (opt == "noexpandtab")
        {
            SetInsertTabs(true); // noexpandtab = use tabs -> InsertTabs = true
        }
        else if (opt == "minimap")
        {
            SetMinimap(true);
        }
        else if (opt == "nominimap")
        {
            SetMinimap(false);
        }
        else if (opt.find('=') != std::string::npos)
        {
            size_t pos = opt.find('=');
            std::string name = opt.substr(0, pos);
            std::string value = opt.substr(pos + 1);

            if (name == "tabstop")
            {
                SetTabStop(value);
            }
            else if (name == "shiftwidth")
            {
                SetShiftWidth(value);
            }
            else if (name == "font")
            {
                bool success = GetEditor().GetDisplay().SetFontByName(value);
                if (!success)
                {
                    GetEditor().SetCommandText("Failed to set font: " + value);
                }
            }
            else
            {
                GetEditor().SetCommandText("Not implemented: " + name);
            }
        }
        else if (opt == "all")
        {
            ShowAllOptions();
        }
        else
        {
            GetEditor().SetCommandText("Not implemented: " + opt);
        }
    }

    bool IsRelativeNumberEnabled() const
    {
        auto pMode = GetEditor().GetGlobalMode();
        return pMode && strcmp(pMode->Name(), "pZep") == 0
            ? static_cast<ZepMode_Vim*>(pMode)->UsesRelativeLines()
            : false;
    }

    bool IsFlagEnabled(uint32_t flag) const
    {
        auto pWindow = GetEditor().GetActiveWindow();
        return pWindow ? ZTestFlags(pWindow->GetWindowFlags(), flag) : false;
    }

    bool IsInsertTabsEnabled() const
    {
        auto pBuffer = GetEditor().GetActiveBuffer();
        return pBuffer ? pBuffer->HasFileFlags(FileFlags::InsertTabs) : false;
    }

    void SetShowLineNumbers(bool enable)
    {
        GetEditor().GetConfig().showLineNumbers = enable;
        auto pWindow = GetEditor().GetActiveWindow();
        if (pWindow)
        {
            uint32_t flags = pWindow->GetWindowFlags();
            if (enable)
                flags |= WindowFlags::ShowLineNumbers;
            else
                flags &= ~WindowFlags::ShowLineNumbers;
            pWindow->SetWindowFlags(flags);
        }
    }

    void SetRelativeNumber(bool enable)
    {
        auto pMode = GetEditor().GetGlobalMode();
        if (pMode && strcmp(pMode->Name(), "pZep") == 0)
        {
            static_cast<ZepMode_Vim*>(pMode)->SetUseRelativeLineNumbers(enable);
        }
    }

    void SetFlag(uint32_t flag, bool enable)
    {
        auto pWindow = GetEditor().GetActiveWindow();
        if (pWindow)
        {
            uint32_t flags = pWindow->GetWindowFlags();
            if (enable)
                flags |= flag;
            else
                flags &= ~flag;
            pWindow->SetWindowFlags(flags);
        }
    }

    void SetInsertTabs(bool enable)
    {
        auto pBuffer = GetEditor().GetActiveBuffer();
        if (pBuffer)
        {
            pBuffer->SetFileFlags(FileFlags::InsertTabs, enable);
        }
    }

    void SetTabStop(const std::string& value)
    {
        try
        {
            int val = std::stoi(value);
            if (val > 0)
            {
                GetEditor().GetConfig().tabStop = val;
            }
        }
        catch (...)
        {
        }
    }

    void SetShiftWidth(const std::string& value)
    {
        try
        {
            int val = std::stoi(value);
            if (val > 0)
            {
                GetEditor().GetConfig().shiftWidth = val;
            }
        }
        catch (...)
        {
        }
    }

    void SetMinimap(bool enable)
    {
        GetEditor().GetConfig().showMinimap = enable;
        GetEditor().GetDisplay().SetLayoutDirty(true);
    }
};

class ZepExCommand_Version : public ZepExCommand
{
public:
    ZepExCommand_Version(ZepEditor& editor)
        : ZepExCommand(editor)
    {
    }

    const char* ExCommandName() const override
    {
        return "version";
    }

    void Run(const std::vector<std::string>& args) override
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
        GetEditor().SetCommandText(ss.str());
    }
};

void RegisterVimExCommands(ZepEditor& editor)
{
    editor.RegisterExCommand(std::make_shared<ZepExCommand_Substitute>(editor));
    editor.RegisterExCommand(std::make_shared<ZepExCommand_Quit>(editor));
    editor.RegisterExCommand(std::make_shared<ZepExCommand_Write>(editor));
    editor.RegisterExCommand(std::make_shared<ZepExCommand_WriteQuit>(editor));
    editor.RegisterExCommand(std::make_shared<ZepExCommand_ListBuffers>(editor));
    editor.RegisterExCommand(std::make_shared<ZepExCommand_Ls>(editor));
    editor.RegisterExCommand(std::make_shared<ZepExCommand_Global>(editor));
    editor.RegisterExCommand(std::make_shared<ZepExCommand_Next>(editor));
    editor.RegisterExCommand(std::make_shared<ZepExCommand_Edit>(editor));
    editor.RegisterExCommand(std::make_shared<ZepExCommand_BufferNext>(editor));
    editor.RegisterExCommand(std::make_shared<ZepExCommand_BufferPrev>(editor));
    editor.RegisterExCommand(std::make_shared<ZepExCommand_BufferGoto>(editor));
    editor.RegisterExCommand(std::make_shared<ZepExCommand_Split>(editor));
    editor.RegisterExCommand(std::make_shared<ZepExCommand_VSplit>(editor));
    editor.RegisterExCommand(std::make_shared<ZepExCommand_Read>(editor));
    editor.RegisterExCommand(std::make_shared<ZepExCommand_TabNew>(editor));
    editor.RegisterExCommand(std::make_shared<ZepExCommand_TabClose>(editor));
    editor.RegisterExCommand(std::make_shared<ZepExCommand_Set>(editor));
    editor.RegisterExCommand(std::make_shared<ZepExCommand_Tutor>(editor));
    editor.RegisterExCommand(std::make_shared<ZepExCommand_Version>(editor));
    editor.RegisterExCommand(std::make_shared<ZepExCommand_GetFonts>(editor));

    // Git commands
    if (auto spGit = editor.GetGit())
    {
        editor.RegisterExCommand(std::make_shared<ZepExCommand_GitStatus>(editor, spGit));
        editor.RegisterExCommand(std::make_shared<ZepExCommand_GitDiff>(editor, spGit));
        editor.RegisterExCommand(std::make_shared<ZepExCommand_VGitDiff>(editor, spGit));
        editor.RegisterExCommand(std::make_shared<ZepExCommand_GitBlame>(editor, spGit));
        editor.RegisterExCommand(std::make_shared<ZepExCommand_GitCommit>(editor, spGit));
        editor.RegisterExCommand(std::make_shared<ZepExCommand_GitPush>(editor, spGit));
        editor.RegisterExCommand(std::make_shared<ZepExCommand_GitPull>(editor, spGit));
    }
}

bool ZepMode_Vim::GetCommand(CommandContext& context)
{
    auto mappedCommand = context.keymap.foundMapping;

    if (mappedCommand == id_MacroRecord)
    {
        if (m_recordingRegister != 0)
        {
            StopRecording();
        }
        else
        {
            if (!context.keymap.captureChars.empty())
            {
                char reg = context.keymap.captureChars[0];
                StartRecording(reg);
            }
        }
        context.commandResult.flags = CommandResultFlags::HandledCount;
        return true;
    }
    else if (mappedCommand == id_MacroPlay)
    {
        char reg = 0;
        if (!context.keymap.captureChars.empty())
        {
            reg = context.keymap.captureChars[0];
        }
        else if (m_lastPlaybackRegister != 0)
        {
            reg = m_lastPlaybackRegister;
        }

        if (reg != 0)
        {
            int count = context.keymap.TotalCount();
            PlayMacro(reg, count);
        }
        context.commandResult.flags = CommandResultFlags::HandledCount;
        return true;
    }

    return ZepMode::GetCommand(context);
}

void ZepMode_Vim::HandleExTabCompletion()
{
    std::string cmd = m_currentCommand;
    const std::string prefix1 = ":set font ";
    const std::string prefix2 = ":set font=";
    size_t pos = std::string::npos;
    std::string after;
    if (cmd.compare(0, prefix1.size(), prefix1) == 0)
    {
        pos = prefix1.size();
        after = cmd.substr(pos);
    }
    else if (cmd.compare(0, prefix2.size(), prefix2) == 0)
    {
        pos = prefix2.size();
        after = cmd.substr(pos);
    }
    else
    {
        return;
    }

    auto pDisplay = &GetEditor().GetDisplay();
    auto fonts = pDisplay->GetAvailableMonospaceFonts();
    std::vector<std::string> matches;
    for (auto& f : fonts)
    {
        if (f.find(after) == 0)
        {
            matches.push_back(f);
        }
    }

    if (matches.empty())
    {
        GetEditor().SetCommandText("No font matches");
        return;
    }

    if (matches.size() == 1)
    {
        std::string newCmd = ":set font ";
        // preserve equals if used
        if (cmd[pos - 1] == '=')
            newCmd += "=";
        newCmd += matches[0];
        m_currentCommand = newCmd;
        GetEditor().SetCommandText(newCmd);
        return;
    }

    // Multiple matches: show them
    std::ostringstream str;
    str << "Matches:";
    for (auto& m : matches)
        str << " " << m;
    GetEditor().SetCommandText(str.str());
}

bool ZepMode_Vim::IsValidRegister(char reg) const
{
    return ((reg >= 'a' && reg <= 'z') || (reg >= '0' && reg <= '9'));
}

void ZepMode_Vim::StartRecording(char reg)
{
    if (!IsValidRegister(reg))
    {
        GetEditor().SetCommandText("Invalid register");
        return;
    }

    m_recordingRegister = reg;
    m_macros[reg] = "";
    GetEditor().SetCommandText("recording @" + std::string(1, reg));
}

void ZepMode_Vim::StopRecording()
{
    if (m_recordingRegister != 0)
    {
        GetEditor().SetCommandText("recorded @" + std::string(1, m_recordingRegister));
        m_recordingRegister = 0;
    }
}

std::string ZepMode_Vim::GetRegisterValue(char reg) const
{
    auto it = m_macros.find(reg);
    if (it != m_macros.end())
    {
        return it->second;
    }
    return "";
}

void ZepMode_Vim::PlayMacro(char reg, int count)
{
    if (!IsValidRegister(reg))
    {
        GetEditor().SetCommandText("Invalid register");
        return;
    }

    std::string macro = GetRegisterValue(reg);
    if (macro.empty())
    {
        GetEditor().SetCommandText("Register @" + std::string(1, reg) + " is empty");
        return;
    }

    m_lastPlaybackRegister = reg;

    for (int i = 0; i < count; i++)
    {
        for (char key : macro)
        {
            HandleMappedInput(std::string(1, key));
        }
    }
}

void ZepMode_Vim::AddKeyPress(uint32_t key, uint32_t modifierKeys)
{
    if (m_recordingRegister != 0)
    {
        if (key == ExtKeys::ESCAPE)
        {
            StopRecording();
            return;
        }

        std::string input = ConvertInputToMapString(key, modifierKeys);
        if (!input.empty())
        {
            m_macros[m_recordingRegister] += input;
        }
        return;
    }

    ZepMode::AddKeyPress(key, modifierKeys);
}

} // namespace Zep
