# pZep User Guide

## Your New Favorite Editor

Alright, let's talk about pZep - the Vim-like editor that'll make you wonder why you ever struggled with anything else.

If you're new to Vim-style editors, this guide is for you. We're going to go from "what even is normal mode" to "yeah, I live in this editor now."

Let's get into it.

---

## What Even Is pZep?

pZep is a modern text editor that gives you the power of Vim keybindings without the learning curve (well, without *all* of it). Think of it as Vim made approachable.

Here's the thing about pZep: it has *modes*. This is the part that trips everyone up, but once it clicks, everything makes sense.

---

## The Two Modes You Actually Need

### Normal Mode - Your Home Base

This is where you start. When you launch pZep, you're in Normal mode. This is for navigating and running commands. You can't type text here - and that's a good thing.

Think of Normal mode as your command center. You're moving around, deleting, copying, searching. The world is your oyster.

### Insert Mode - Where You Type

This is what it sounds like. When you press `i`, you enter Insert mode and can actually type. It's like a normal text editor here.

Press `ESC` to go back to Normal mode.

That's it. That's the secret.

---

## Your First Session

### Opening pZep

```bash
pzep-gui myfile.cpp
```

Or just:

```bash
pzep-gui
```

And you'll get a blank untitled buffer.

### Let's Type Something

1. Press `i` - you're now in Insert mode
2. Type "Hello, world!"
3. Press `ESC` - back to Normal mode

Congratulations. You just used Vim-style editing.

### Saving Your Work

In Normal mode:

```
:w
```

That's it. Write (save) the file. If you want to quit *and* save:

```
:wq
```

Or if you're crazy and don't want to save:

```
:q!
```

---

## Navigation - The Fun Part

Here's where Vim-style editors shine. Once you learn these, you'll never go back to arrow keys.

### Basic Movement

| Key | What It Does |
|-----|-------------|
| `h` | Left |
| `j` | Down |
| `k` | Up |
| `l` | Right |

Yes, you keep your hands on the home row. Your fingers will thank you.

### Word Movement

| Key | What It Does |
|-----|-------------|
| `w` | Forward to next word |
| `b` | Back to previous word |
| `e` | End of word |

### Line Navigation

| Key | What It Does |
|-----|-------------|
| `0` | Start of line |
| `$` | End of line |
| `^` | First non-blank character |

### Big Leaps

| Key | What It Does |
|-----|-------------|
| `gg` | First line of file |
| `G` | Last line of file |
| `5G` | Go to line 5 |

---

## Editing Without Reaching for the Mouse

### Delete Things

| Key | What It Does |
|-----|-------------|
| `x` | Delete character under cursor |
| `dd` | Delete entire line |
| `dw` | Delete word |
| `d$` | Delete to end of line |

### The Magic of "Change"

`c` is like delete but drops you into Insert mode after. Game changer (literally).

| Key | What It Does |
|-----|-------------|
| `cw` | Change word |
| `cc` | Change entire line |
| `c$` | Change to end of line |

### Undo/Redo - Your Safety Net

| Key | What It Does |
|-----|-------------|
| `u` | Undo |
| `Ctrl+r` | Redo |

---

## The Dot Command - Your Best Friend

This is the secret weapon. `.` repeats your last change.

Here's a scenario: you need to delete the word "foo" from five different lines.

1. Go to first "foo"
2. Type `dw` to delete the word
3. Move to next line
4. Press `.`
5. Repeat until done

You just did the work of one deletion and four dots. That's the power of the dot command.

---

## Searching - Find Anything

### Inside Your File

| Key | What It Does |
|-----|-------------|
| `/pattern` | Search forward |
| `?pattern` | Search backward |
| `n` | Next match |
| `N` | Previous match |
| `*` | Search for word under cursor |

Try it:
1. Press `/`
2. Type "function"
3. Press Enter
4. Press `n` to jump through matches

### Find and Replace

This is where pZep gets spicy:

```
:s/old/new
```

That's "substitute". Replaces "old" with "new" on the current line.

Want to replace everywhere in the file?

```
:%s/old/new/g
```

The `%` means "entire file", `g` means "global" (all occurrences on each line).

---

## Visual Mode - Selecting Stuff

Press `v` to enter Visual mode. Now you can select text with your movement keys and do things to it.

| Key | What It Does |
|-----|-------------|
| `v` | Visual mode (character-wise) |
| `V` | Visual mode (line-wise) |
| `Ctrl+v` | Visual block mode |

Workflow:
1. Press `v`
2. Move with `j`/`k` to select
3. Press `d` to delete, or `y` to yank (copy)

---

## Registers - Copy/Paste on Steroids

Vim has multiple clipboards called "registers".

| Key | What It Does |
|-----|-------------|
| `"ay` | Yank to register a |
| `"ap` | Paste from register a |
| `"+y` | Copy to system clipboard |
| `"+p` | Paste from system clipboard |

Want to copy a word and paste it five times? Yank it once, dot it four times.

---

## Macros - Automate Your Life

This is the advanced stuff, but start simple.

Record a macro:
1. Press `q` followed by a letter (say `a`) to start recording
2. Do your thing (any sequence of keys)
3. Press `q` again to stop

Play it back:
- `@a` - Play register a once
- `5@a` - Play it 5 times

Example: You need to add a semicolon to the end of 20 lines.
1. Go to first line
2. Press `qa` to start recording
3. Press `$` to go to end, `a` to enter insert, `;` to add semicolon, `ESC` to exit
4. Press `q` to stop
5. Move to next line with `j`
6. Press `@a` to replay
7. Keep pressing `j` and `@a` or do `19@a`

---

## Buffer Commands - Multiple Files

Working on a project with multiple files? pZep handles that.

| Command | What It Does |
|---------|-------------|
| `:e filename` | Open file |
| `:bn` | Next buffer |
| `:bp` | Previous buffer |
| `:b 2` | Go to buffer 2 |
| `:buffers` | List all buffers |

---

## :set Options - Customize Your Experience

pZep has options you can toggle:

| Command | What It Does |
|---------|-------------|
| `:set number` | Show line numbers |
| `:set nonumber` | Hide line numbers |
| `:set wrap` | Wrap long lines |
| `:set nowrap` | Don't wrap |
| `:set autoindent` | Auto-indent new lines |
| `:set spell` | Enable spell check |

Query what something is set to:

```
:set number?
```

Shows `number` if enabled.

---

## Git Integration - Because It's 2026

If you're using pZep with a project that has git, you've got commands:

| Command | What It Does |
|---------|-------------|
| `:Gstatus` | Show git status |
| `:Gdiff` | Show changes |
| `:Gblame` | Blame someone |
| `:Gcommit "msg"` | Commit changes |

---

## The pZep Philosophy

Here's the thing about learning pZep: you're not going to learn everything in a day. And that's fine.

Start with:
1. `i` to insert, `ESC` to exit
2. `h/j/k/l` to move
3. `dd`, `dw`, `x` to delete
4. `u` to undo
5. `/` to search
6. `:w` to save

Master those, then add:
- `.` (dot command)
- Visual mode (`v`)
- `:s` for find/replace

Then graduate to:
- Macros (`q`)
- Multiple cursors
- Custom keybindings

---

## Quick Reference Card

```
MOVEMENT          EDITING
h j k l          x - delete char
w b e            dw, cw, dd - delete word/line
0 $              u - undo
gg G             Ctrl+r - redo

FILES             SEARCH
:e file           /pattern
:bn :bp          n N
:w :q :wq        * - search word

VISUAL            EXTRAS
v V Ctrl+v        . - repeat
y d c            ~ - toggle case
```

---

## Wrapping Up

Look, here's the deal. pZep isn't about being a keyboard ninja (though you will become one). It's about flow. It's about keeping your hands on the keyboard and your eyes on the screen.

You're not going to remember everything in this guide. That's okay. Pick three things to focus on this week. Next week, pick three more.

Before you know it, you'll be wondering how you ever edited text any other way.

Now go open pZep and break some things. That's how you learn.

```
:wq
```

---

*Made with ❤️ for the Vim-curious*