# Vim Ex Commands Specification for pZep

## 1. Overview

This document defines the complete specification for implementing Vim-style ex commands (`:command`) in nZep. The goal is compatibility with Vim's ex command behavior.

---

## 2. File Operations

### 2.1 Write Commands

| Command | Description | Syntax |
|---------|-------------|--------|
| `:w` | Write (save) current buffer | `:w [filename]` |
| `:w!` | Write forcing overwrite | `:w! [filename]` |
| `:wq` | Write and quit | `:wq [filename]` |
| `:wq!` | Write, force, and quit | `:wq! [filename]` |
| `:x` | Same as `:wq` | `:x [filename]` |
| `:wa` | Write all modified buffers | `:wa` |
| `:wa!` | Force write all | `:wa!` |
| `:wn` | Write and move to next file | `:wn [filename]` |
| `:up` | Update (write if modified) | `:up` |
| `:wall` | Write all | `:wall` |

### 2.2 Quit Commands

| Command | Description | Syntax |
|---------|-------------|--------|
| `:q` | Quit (fails if unsaved) | `:q` |
| `:q!` | Quit without saving | `:q!` |
| `:qa` | Quit all windows | `:qa` |
| `:qa!` | Force quit all | `:qa!` |
| `:qa` | Quit all | `:qa` |
| `:cq` | Quit always (exit Vim) | `:cq` |
| `:qa` | Quit all | `:qa` |
| `:qall` | Quit all windows/tabs | `:qall` |

### 2.3 Buffer Commands

| Command | Description | Syntax |
|---------|-------------|--------|
| `:bnext` / `:bn` | Next buffer | `:bn` |
| `:bprev` / `:bp` | Previous buffer | `:bp` |
| `:bfirst` | First buffer | `:bfirst` |
| `:blast` | Last buffer | `:blast` |
| `:bdelete` / `:bd` | Delete buffer | `:bd [n]` |
| `:buffer` / `:b` | Go to buffer | `:b n` or `:b filename` |
| `:buffers` / `:ls` | List buffers | `:ls` |
| `:ball` | Open all buffers | `:ball` |
| `:badd` | Add buffer | `:badd filename` |
| `:bunload` | Unload buffer | `:bunload [n]` |
| `:bmodified` / `:bm` | Next modified buffer | `:bm` |

### 2.4 Edit Commands

| Command | Description | Syntax |
|---------|-------------|--------|
| `:e` | Edit file | `:e [filename]` |
| `:e!` | Edit discarding changes | `:e!` |
| `:enew` | Edit new file | `:enew` |
| `:fin` | Find next file | `:fin[d] [pattern]` |
| `:f` | Show file info | `:f` or `:file` |
| `:r` | Read file into buffer | `:r [filename]` |
| `:r!` | Read command output | `:r! command` |
| `:sp` | Split horizontally | `:sp [filename]` |
| `:vsp` | Split vertically | `:vsp [filename]` |

---

## 3. Search and Replace

### 3.1 Substitution Command

| Command | Description | Syntax |
|---------|-------------|--------|
| `:s` | Substitute | `:s/pattern/replacement/[flags]` |
| `:%s` | Substitute all | `:%s/pattern/replacement/[flags]` |
| `:s#` | Alt delimiter | `:s#pattern#replacement#` |

**Flags:**
- `g` - Global (all on line)
- `c` - Confirm each
- `i` - Ignore case
- `I` - Case sensitive
- `&` - Use last search pattern

**Range:**
- `.` - Current line
- `$` - Last line
- `%` - Entire file
- `n,m` - Lines n to m
- `'t` - Marker t
- `'<` - Visual start
- `'>` - Visual end

### 3.2 Global Commands

| Command | Description | Syntax |
|---------|-------------|--------|
| `:g` | Global search | `:g/pattern/command` |
| `:g!` | Global inverse | `:g!/pattern/command` |
| `:g/pattern/d` | Delete matching | `:g/pattern/d` |
| `:g/pattern/p` | Print matching | `:g/pattern/p` |
| `:g/pattern/y` | Yank matching | `:g/pattern/y` |

### 3.3 Search Navigation

| Command | Description | Syntax |
|---------|-------------|--------|
| `:n` | Next file in arglist | `:n` |
| `:prev` | Previous file in arglist | `:prev` |
| `:argdo` | Execute on all args | `:argdo {cmd}` |
| `/` | Forward search | `/pattern` |
| `?` | Backward search | `?pattern` |
| `n` | Next match | `n` |
| `N` | Previous match | `N` |
| `*` | Search word forward | `*` |
| `#` | Search word backward | `#` |
| `gd` | Go to definition | `gd` |
| `gD` | Go to global definition | `gD` |

---

## 4. Navigation Commands

### 4.1 Line Navigation

| Command | Description | Syntax |
|---------|-------------|--------|
| `:` | Go to line | `:n` or `:nG` |
| `:nu` | Numbered lines | `:nu` |
| `:number` | Show line numbers | `:set number` |
| `:nonu` | Hide line numbers | `:set nonumber` |
| `:{n}` | Go to line n | `:42` |
| `''` | Return to last line | `''` |
| '`"`` | Return to cursor position | '`"` |

### 4.2 Window Commands

| Command | Description | Syntax |
|---------|-------------|--------|
| `:split` / `:sp` | Horizontal split | `:sp` |
| `:vsplit` / `:vs` | Vertical split | `:vs` |
| `:new` | New horizontal window | `:new` |
| `:vnew` | New vertical window | `:vnew` |
| `:close` / `:clo` | Close window | `:clo` |
| `:only` / `:on` | Show only current | `:on` |
| `:quit` / `:q` | Quit window | `:q` |
| `Ctrl-w h` | Move left | `Ctrl-w h` |
| `Ctrl-w j` | Move down | `Ctrl-w j` |
| `Ctrl-w k` | Move up | `Ctrl-w k` |
| `Ctrl-w l` | Move right | `Ctrl-w l` |
| `Ctrl-w w` | Cycle windows | `Ctrl-w w` |
| `Ctrl-w H` | Move window left | `Ctrl-w H` |
| `Ctrl-w J` | Move window down | `Ctrl-w J` |
| `Ctrl-w K` | Move window up | `Ctrl-w K` |
| `Ctrl-w L` | Move window right | `Ctrl-w L` |
| `Ctrl-w =` | Equal size | `Ctrl-w =` |
| `Ctrl-w _` | Max height | `Ctrl-w _` |
| `Ctrl-w |` | Max width | `Ctrl-w |` |
| `Ctrl-w r` | Rotate windows | `Ctrl-w r` |
| `Ctrl-w R` | Reverse rotate | `Ctrl-w R` |
| `Ctrl-w x` | Exchange with next | `Ctrl-w x` |

### 4.3 Tab Commands

| Command | Description | Syntax |
|---------|-------------|--------|
| `:tabnew` / `:tabe` | New tab | `:tabe [file]` |
| `:tabnext` / `:tabn` | Next tab | `:tabn` |
| `:tabprevious` / `:tabp` | Previous tab | `:tabp` |
| `:tabfirst` / `:tabfir` | First tab | `:tabfir` |
| `:tablast` / `:tabl` | Last tab | `:tabl` |
| `:tabclose` / `:tabc` | Close tab | `:tabc` |
| `:tabonly` / `:tabo` | Close other tabs | `:tabo` |
| `:tabmove` | Move tab | `:tabmove [n]` |
| `:tabs` | List tabs | `:tabs` |

---

## 5. Text Operations

### 5.1 Yank/Paste

| Command | Description | Syntax |
|---------|-------------|--------|
| `:y` / `:yank` | Yank lines | `:y [range]` |
| `:y a` | Yank to register a | `:y a {motion}` |
| `:pu` / `:put` | Put text | `:pu` |
| `:registers` / `:reg` | Show registers | `:reg` |

### 5.2 Delete

| Command | Description | Syntax |
|---------|-------------|--------|
| `:d` / `:delete` | Delete lines | `:d [range]` |
| `:d a` | Delete to register a | `:d a {motion}` |
| `:dl` | Delete character | `:dl` |
| `:dw` | Delete word | `:dw` |
| `:dd` | Delete line | `:dd` |

### 5.3 Change

| Command | Description | Syntax |
|---------|-------------|--------|
| `:c` / `:change` | Change lines | `:c [range]` |
| `:cH` | Change highlighted | `:cH` |

### 5.4 Shift

| Command | Description | Syntax |
|---------|-------------|--------|
| `:>` | Shift right | `:>` or `:[range]>` |
| `:<` | Shift left | `:<` or `:[range]<` |
| `:>>` | Shift 2 right | `:>>` |
| `:<<` | Shift 2 left | `:<<` |
| `:=` | Reformat | `:=` |

---

## 6. Display Commands

### 6.1 View Options

| Command | Description | Syntax |
|---------|-------------|--------|
| `:set` | Show changed options | `:set` |
| `:set all` | Show all options | `:set all` |
| `:set number` | Show line numbers | `:set number` |
| `:set nonumber` | Hide line numbers | `:set nonumber` |
| `:set relativenumber` | Relative numbers | `:set relativenumber` |
| `:set norelativenumber` | No relative numbers | `:set norelativenumber` |
| `:set wrap` | Wrap lines | `:set wrap` |
| `:set nowrap` | No wrap | `:set nowrap` |
| `:set list` | Show invisible chars | `:set list` |
| `:set nolist` | Hide invisible chars | `:set nolist` |
| `:set cursorline` | Highlight cursor line | `:set cursorline` |
| `:set nocursorline` | No cursor line | `:set nocursorline` |
| `:set cursorcolumn` | Highlight cursor column | `:set cursorcolumn` |
| `:set spell` | Spell check | `:set spell` |
| `:set nospell` | No spell check | `:set nospell` |

### 6.2 Folding

| Command | Description | Syntax |
|---------|-------------|--------|
| `:zf` | Create fold | `:zf{motion}` |
| `:zd` | Delete fold | `:zd` |
| `:zD` | Delete all folds | `:zD` |
| `:zo` | Open fold | `:zo` |
| `:zO` | Open all folds | `:zO` |
| `:zc` | Close fold | `:zc` |
| `:zC` | Close all folds | `:zC` |
| `:za` | Toggle fold | `:za` |
| `:zR` | Open all folds | `:zR` |
| `:zM` | Close all folds | `:zM` |
| `:zn` | Disable folds | `:zn` |
| `:zN` | Enable folds | `:zN` |
| `:zi` | Invert folds | `:zi` |
| `:set foldmethod` | Set fold method | `:set foldmethod=syntax` |
| `:set foldcolumn` | Show fold column | `:set foldcolumn=n` |

### 6.3 Viewport

| Command | Description | Syntax |
|---------|-------------|--------|
| `z{height}<CR>` | Set window height | `z20<CR>` |
| `Ctrl-e` | Scroll down | `Ctrl-e` |
| `Ctrl-y` | Scroll up | `Ctrl-y` |
| `z.` | Redraw at cursor | `z.` |
| `z-` | Redraw at bottom | `z-` |
| `z<CR>` | Redraw at top | `z<CR>` |
| `zt` | Redraw at top | `zt` |
| `zz` | Redraw at center | `zz` |
| `zb` | Redraw at bottom | `zb` |
| `zh` | Scroll right | `zh` |
| `zl` | Scroll left | `zl` |
| `zH` | Scroll half right | `zH` |
| `zL` | Scroll half left | `zL` |
| `zs` | Scroll to side start | `zs` |
| `ze` | Scroll to side end | `ze` |

---

## 7. Shell Commands

| Command | Description | Syntax |
|---------|-------------|--------|
| `:!` | Execute shell command | `:!command` |
| `:!!` | Repeat last command | `:!!` |
| `:sh` | Start shell | `:sh` |
| `:terminal` | Open terminal | `:terminal [cmd]` |
| `:!&` | Async shell command | `:!&cmd` |
| `:suspend` / `:st` | Suspend Vim | `:st` |
| `:!{motion}` | Filter | `!{motion}command` |
| `:w !sudo tee %` | Sudo write | `:w !sudo tee %` |
| `:r !{cmd}` | Read command output | `:r !date` |
| `:!gcc %` | Compile current file | `:!gcc %` |

---

## 8. Git Integration

| Command | Description | Syntax |
|---------|-------------|--------|
| `:Gstatus` | Git status | `:Gstatus` |
| `:Gdiff` | Git diff | `:Gdiff` |
| `:vdiff` | Vertical diff | `:vdiff` |
| `:Gblame` | Git blame | `:Gblame` |
| `:Gcommit` | Git commit | `:Gcommit message` |
| `:Gpush` | Git push | `:Gpush` |
| `:Gpull` | Git pull | `:Gpull` |
| `:Git` | Git command | `:Git [args]` |
| `:Gdiffsplit` | Split diff | `:Gdiffsplit` |
| `:Gread` | Read from git | `:Gread` |
| `:Gwrite` | Write to git | `:Gwrite` |

---

## 9. Option Commands

### 9.1 Boolean Options

| Command | Description |
|---------|-------------|
| `:set number` | Enable line numbers |
| `:set nonumber` | Disable line numbers |
| `:set relativenumber` | Enable relative numbers |
| `:set norelativenumber` | Disable relative numbers |
| `:set wrap` | Enable line wrap |
| `:set nowrap` | Disable line wrap |
| `:set spell` | Enable spell check |
| `:set nospell` | Disable spell check |
| `:set hlsearch` | Highlight searches |
| `:set nohlsearch` | Disable search highlight |
| `:set incsearch` | Incremental search |
| `:set noincsearch` | Disable incremental search |
| `:set ignorecase` | Ignore case in search |
| `:set noignorecase` | Case sensitive search |
| `:set smartcase` | Smart case |
| `:set autoindent` | Auto-indent |
| `:set noautoindent` | No auto-indent |
| `:set smartindent` | Smart indent |
| `:set cindent` | C-style indent |
| `:set expandtab` | Use spaces |
| `:set noexpandtab` | Use tabs |
| `:set showcmd` | Show commands |
| `:set noshowcmd` | Hide commands |
| `:set showmode` | Show mode |
| `:set noshowmode` | Hide mode |
| `:set cursorline` | Highlight line |
| `:set nocursorline` | No line highlight |
| `:set paste` | Paste mode |
| `:set nopaste` | Normal mode |
| `:set ruler` | Show cursor position |
| `:set noruler` | Hide position |
| `:set list` | Show invisible chars |
| `:set nolist` | Hide invisible chars |

### 9.2 String Options

| Command | Description | Default |
|---------|-------------|---------|
| `:set tabstop` | Tab width | `ts=8` |
| `:set softtabstop` | Soft tab width | `sts=8` |
| `:set shiftwidth` | Indent width | `sw=8` |
| `:set expandtab` | Expand tabs | `noexpandtab` |
| `:set fileformat` | File format | `ff=unix` |
| `:set encoding` | File encoding | `enc=utf-8` |
| `:set fileencoding` | Buffer encoding | `fenc=` |
| `:set fileformats` | EOL formats | `ffs=unix,dos` |
| `:set clipboard` | Clipboard | `cb=unnamed` |
| `:set background` | Dark/light | `bg=dark` |
| `:set shell` | Shell to use | varies |
| `:set path` | Include path | `path=.,,` |
| `:set tags` | Tag files | `tags=./tags,tags` |
| `:set makeprg` | Make program | `mp=make` |
| `:set grepprg` | Grep program | `gp=grep -n` |
| `:set formatprg` | Formatter | `fp=` |
| `:set equalprg` | Equalizer | `ep=` |

### 9.3 Number Options

| Command | Description | Default |
|---------|-------------|---------|
| `:set tabstop=n` | Tab width | 8 |
| `:set shiftwidth=n` | Indent width | 8 |
| `:set scroll=n` | Scroll offset | 0 |
| `:set scrolloff=n` | Context lines | 0 |
| `:set sidescrolloff=n` | Side context | 0 |
| `:set columns=n` | Screen columns | 80 |
| `:set lines=n` | Screen lines | 24 |
| `:set textwidth=n` | Max line length | 0 |
| `:set wrapmargin=n` | Wrap margin | 0 |
| `:set timeoutlen=n` | Timeout ms | 1000 |
| `:set matchtime=n` | Match highlight | 5 |
| `:set report=n` | Report threshold | 2 |
| `:set highlight=n` | Highlight delay | 250 |
| `:set updatetime=n` | Update time | 4000 |
| `:set redrawtime=n` | Redraw time | 2000 |
| `:set cmdheight=n` | Command height | 1 |
| `:set cmdwinheight=n` | Cmd window | 20 |
| `:set foldcolumn=n` | Fold column | 0 |
| `:set foldlevel=n` | Fold level | 0 |
| `:set foldlevelstart=n` | Fold start | -1 |
| `:set maxmapdepth=n` | Map depth | 1000 |
| `:set maxmem=n` | Max memory | 2626 |
| `:set maxmempattern=n` | Max pattern | 2046 |

---

## 10. Marks

| Command | Description | Syntax |
|---------|-------------|--------|
| `:marks` | List marks | `:marks` |
| `:delm` | Delete marks | `:delm [marks]` |
| `:delmarks` | Delete marks | `:delmarks a-z` |
| `:delmarks!` | Delete all | `:delmarks!` |
| `ma` | Set mark a | `ma` |
| `'a` | Go to mark a | `'a` |
| `` `a `` | Go to exact mark | `` `a `` |
| `]'` | Next mark line | `]'` |
| `['` | Prev mark line | `['` |
| `]` | Next lowercase mark | `]` |
| `[` | Prev lowercase mark | `[` |
| `:changes` | Show changes | `:changes` |
| `:jumps` | Show jumps | `:jumps` |

---

## 11. Command History

| Command | Description | Syntax |
|---------|-------------|--------|
| `:` | Command history | `:` |
| `q:` | Command window | `q:` |
| `Ctrl-f` | Command history | `Ctrl-f` |
| `:history` | Show history | `:history` |
| `:his` | Show history | `:his` |
| `q/` | Search history | `q/` |
| `q?` | Search history | `q?` |

---

## 12. Command-Line Window

| Command | Description | Key |
|---------|-------------|-----|
| Command window | Open from normal | `q:` |
| Search window | Open from normal | `q/` |
| Search window | Open from normal | `q?` |
| Execute | Run command | `<CR>` |
| Exit | Cancel | `:q` |
| Complete | Word completion | `<Tab>` |
| Complete | Line completion | `<Ctrl-l>` |
| History up | Previous command | `<Up>` |
| History down | Next command | `<Down>` |

---

## 13. Programming Commands

### 13.1 Make/Compile

| Command | Description | Syntax |
|---------|-------------|--------|
| `:make` | Run make | `:make [args]` |
| `:mak[e] {args}` | Make with args | `:make` |
| `:compiler` | Set compiler | `:compiler gcc` |
| `:cw` | Quickfix window | `:cw` |
| `:cwindow` | Quickfix window | `:cwindow` |
| `:copen` | Open quickfix | `:copen` |
| `:cclose` | Close quickfix | `:cclose` |
| `:clist` | List errors | `:clist` |
| `:cfirst` | First error | `:cfirst` |
| `:clast` | Last error | `:clast` |
| `:cnext` | Next error | `:cnext` |
| `:cprev` | Previous error | `:cprev` |
| `:cnfile` | Next in file | `:cnfile` |
| `:cpfile` | Prev in file | `:cpfile` |
| `:cc` | Show error | `:cc [n]` |

### 13.2 Grep/Search Files

| Command | Description | Syntax |
|---------|-------------|--------|
| `:grep` | Grep pattern | `:grep [args] pattern [files]` |
| `:vimgrep` | Vim grep | `:vimgrep pattern files` |
| `:lgrep` | Loclist grep | `:lgrep [args] pattern` |
| `:lvimgrep` | Loclist vimgrep | `:lvimgrep pattern files` |
| `:copen` | Location list | `:lopen` |
| `:lwindow` | Location window | `:lwindow` |
| `:lnext` | Next location | `:lnext` |
| `:lprev` | Prev location | `:lprev` |
| `:lfirst` | First location | `:lfirst` |
| `:llast` | Last location | `:llast` |

### 13.3 Tags

| Command | Description | Syntax |
|---------|-------------|--------|
| `:tag` | Jump to tag | `:tag {ident}` |
| `:tags` | Show tag stack | `:tags` |
| `:pop` | Pop tag stack | `:pop` |
| `:tagnext` | Next tag | `:tagnext` |
| `:tagprev` | Prev tag | `:tagprev` |
| `:tagfirst` | First tag | `:tagfirst` |
| `:taglast` | Last tag | `:taglast` |
| `:ptag` | Preview tag | `:ptag {ident}` |
| `:pclose` | Close preview | `:pclose` |
| `:tselect` | Select tag | `:tselect` |
| `:stselect` | Split select tag | `:stselect` |

### 13.4 Formatting

| Command | Description | Syntax |
|---------|-------------|--------|
| `:gq` | Format text | `:gq{motion}` |
| `:gw` | Format with write | `:gw{motion}` |
| `gq` | Format operator | `gq{motion}` |
| `gw` | Format and save | `gw{motion}` |
| `:left` | Left align | `:[{range}]left [width]` |
| `:center` | Center align | `:[{range}]center [width]` |
| `:right` | Right align | `:[{range}]right [width]` |
| `:sort` | Sort lines | `:[{range}]sort [options]` |

---

## 14. Diff Mode

| Command | Description | Syntax |
|---------|-------------|--------|
| `:diffthis` | Make buffer diff | `:diffthis` |
| `:diffoff` | Turn off diff | `:diffoff` |
| `:diffsplit` | Split diff | `:diffsplit [file]` |
| `:diffupdate` | Update diff | `:diffupdate` |
| `]c` | Next diff | `]c` |
| `[c` | Previous diff | `[c` |
| `:diffget` | Get from other | `:diffget` |
| `:diffput` | Put to other | `:diffput` |
| `:diffohl` | Highlight links | `:diffohl` |

---

## 15. Registers

| Command | Description | Syntax |
|---------|-------------|--------|
| `:registers` / `:reg` | Show registers | `:reg` |
| `:display` / `:dis` | Show registers | `:dis` |
| `" ay` | Yank to register a | `" ay{motion}` |
| `" ap` | Put from register a | `" ap` |

---

## 16. Command Modifier Keywords

| Keyword | Description | Example |
|---------|-------------|---------|
| `:abo` | Above range | `:abo[veleft] {cmd}` |
| `:bel` | Below range | `:bel[owright] {cmd}` |
| `:keep` | Keep view | `:keepa[lt] {cmd}` |
| `:left` | Left align | `:left {cmd}` |
| `:noautocmd` | No auto events | `:noautocmd {cmd}` |
| `:sandbox` | Sandbox mode | `:sandbox {cmd}` |
| `:silent` | Silent | `:sil[ent] {cmd}` |
| `:unsilent` | Not silent | `:unsilent {cmd}` |
| `:vertical` | Vertical | `:vert[ical] {cmd}` |

---

## 17. Ranges

### 17.1 Range Syntax

| Range | Description |
|-------|-------------|
| `{number}` | Line {number} |
| `.` | Current line |
| `$` | Last line |
| `%` | Entire file |
| `*` | Last selection |
| `'<` | Visual start |
| `'>` | Visual end |
| `'t` | Mark t |
| `/{pattern}/` | Search pattern |
| `?{pattern}?` | Search backward |
| `+{n}` or `-{n}` | Offset |

### 17.2 Range Separators

| Separator | Description |
|-----------|-------------|
| `,` | From-to (e.g., `1,5`) |
| `;` | From-to with reset (e.g., `1;5`) |

### 17.3 Special Ranges

| Range | Description |
|-------|-------------|
| `:'<,'>` | Visual selection |
| `:.,$` | Current to end |
| `:.,+5` | Current + 5 lines |
| `:%` | Entire file |
| `:'<` | Visual start |
| `:'>` | Visual end |

---

## 18. Command Modifiers

| Modifier | Description |
|----------|-------------|
| `:keepmarks` | Don't move marks |
| `:keepjumps` | Don't move jumps |
| `:keeppatterns` | Keep search patterns |
| `:lockmarks` | Lock marks |
| `:noswapfile` | No swap file |
| `:savepos` | Save cursor position |
| `:verbose {n}` | Set verbosity |

---

## 19. Auto Commands

| Command | Description | Syntax |
|---------|-------------|--------|
| `:au` | Auto command | `:au[tocmd]` |
| `:autocmd` | Define autocmd | `:autocmd [group] [events] [pat] [cmd]` |
| `:doautocmd` | Execute autocmd | `:doautocmd [group] [event] [fname]` |
| `:doautoall` | Execute on all | `:doautoall [group] [event]` |
| `:augroup` | Command group | `:augroup {name}` |
| `:autocmd!` | Delete autocmds | `:autocmd! [group]` |

### 19.1 Events

| Event | Description |
|-------|-------------|
| `BufNewFile` | New file |
| `BufReadPre` | Before reading file |
| `BufRead` | After reading file |
| `BufReadPost` | After reading (post) |
| `FileReadPre` | Before reading |
| `FileReadPost` | After reading |
| `FilterReadPre` | Before filter read |
| `FilterReadPost` | After filter read |
| `StdFileReadPre` | Before stdin read |
| `StdFileReadPost` | After stdin read |
| `BufWrite` | Before writing |
| `BufWritePre` | Before writing buffer |
| `BufWritePost` | After writing buffer |
| `FileWritePre` | Before writing |
| `FileWritePost` | After writing |
| `FilterWritePre` | Before filter write |
| `FilterWritePost` | After filter write |
| `BufAdd` | After buffer added |
| `BufDelete` | Before buffer delete |
| `BufWipeout` | Before buffer wipe |
| `BufFilePre` | Before buffer filename |
| `BufFilePost` | After buffer filename |
| `BufEnter` | After entering buffer |
| `BufLeave` | Before leaving buffer |
| `BufHidden` | After buffer hidden |
| `BufUnload` | Before unloading |
| `BufWinEnter` | After buffer in window |
| `BufWinLeave` | Before buffer from window |
| `BufFreeEnter` | After buffer display |
| `BufFreeLeave` | Before buffer display |
| `VimEnter` | After Vim startup |
| `GUIEnter` | After GUI starts |
| `TermResponse` | Terminal response |
| `VimLeavePre` | Before exiting |
| `VimLeave` | Before exiting (post) |
| `QuitPre` | Before quit |
| `WinNew` | After window created |
| `WinEnter` | After entering window |
| `WinLeave` | Before leaving window |
| `TabNew` | After tab created |
| `TabEnter` | After entering tab |
| `TabLeave` | Before leaving tab |
| `TabClosed` | After tab closed |
| `ShellCmdPost` | After shell command |
| `ShellFilterPost` | After shell filter |
| `CmdUndefined` | Command not found |
| `FuncUndefined` | Function not found |
| `SpellFileMissing` | Spell file missing |
| `SourcePre` | Before sourcing |
| `SourceCmd` | Source command |
| `SourcePost` | After sourcing |
| `VimResized` | After window resize |
| `FocusGained` | Focus gained |
| `FocusLost` | Focus lost |
| `CursorMoved` | Cursor moved |
| `CursorMovedI` | Cursor moved (insert) |
| `CursorHold` | Cursor idle |
| `CursorHoldI` | Cursor idle (insert) |
| `WinScrolled` | Window scrolled |
| `TextChanged` | Text changed |
| `TextChangedI` | Text changed (insert) |
| `TextChangedP` | Text changed (popup) |
| `TextChangedT` | Text changed (terminal) |
| `SafeState` | Safe state |
| `SafeStateAgain` | Safe state again |
| `DirChanged` | Directory changed |
| `Signal` | Signal received |
| `OptionSet` | Option set |
| `ModeChanged` | Mode changed |
| `Remote` | Remote message |
| `RemoteReply` | Remote reply |
| `InputMethodSwitch` | IM switch |
| `ColorScheme` | Color scheme |
| `ColorSchemePre` | Before colorscheme |
| `CompletionDone` | After completion |
| `CompleteChanged` | Completion changed |
| `CompleteDone` | Completion finished |
| `InsertEnter` | Enter insert mode |
| `InsertLeave` | Leave insert mode |
| `InsertCharPre` | Before char insert |
| `InsertChange` | Insert mode change |
| `ModeChanged` | Mode changed |
| `CompleteEnter` | Enter completion |
| `CompleteLeave` | Leave completion |

---

## 20. Version History

- v1.0 (2026-04-21) - Initial specification

---

*This specification defines Vim ex command compatibility for nZep.*
