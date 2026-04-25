Assumptions in the code / Design decisions:

- **UTF-8 full support**: All text is UTF-8. Font atlas preloads comprehensive Unicode ranges: Basic Latin, Latin Extended, Greek, Cyrillic, Georgian, Arabic, Arabic Supplement, Thaana, Devanagari, Thai, Currency Symbols (€£¥), Enclosed Alphanumerics, Box Drawing, Block Elements, Geometric Shapes, Miscellaneous Symbols. More can be added dynamically via config.

- **Cursor placement**: The cursor is only ever on a valid buffer location. This can be the hidden CR of the line, or the 0 at the end. The editor operates on byte/glyph iterators within the gap buffer.

- **Line endings**: Internally everything is '\n'; '\r\n' is converted on load and converted back on save if necessary. Mixed line endings are normalized to '\n'.

- **Wrapping and virtual lines**: The window converts visible buffer lines into screen spans (SpanInfo), handling word wrapping and generating virtual screen lines. Cursor moves on logical buffer lines (j/k) by default; screen-line motions (gj/gk) are separate and not yet mapped.

- **Scrolling**: Cursor can be off-screen; ScrollToCursor ensures visibility.

- **Configuration**: `~/.pzep/.pzeprc` is the user config root. Config files loaded in order: `pzeprc`, `_pzeprc`, `.pzeprc`. `--init` flag creates the directory and default config. Settings are applied immediately and broadcast to windows to refresh layout.

- **Minimap**: Rendered inline via text blocks; `set nominimap` hides the region. Width determined by `minimapWidth` config. Dragging and wheel scroll supported.

- **Code folding**: `ZepFold` tracks fold regions. `zf` creates manual folds; `RebuildFromIndentation()` auto-creates indent-based folds for recognized code file extensions on load. Folds are represented as placeholder spans showing hidden line count; indicator region draws small marker. Nested folds supported; visibility calculation merges closed intervals.

- **Mouse support**: Wheel scrolls text (just like minimap). Left-click moves cursor; drag creates visual selection. Ctrl+C copies (normal mode = line, visual mode = selection). Ctrl+V pastes from system clipboard. No right-click context menu yet.

- **Font handling**: Monospace fonts discovered via Windows GDI + registry enumeration. `:fonts` lists available; `:set font=<name>` switches at runtime (triggers layout rebuild). Font codepoint atlas reused across fonts; fallback to default if load fails.

- **Display**: Built on raylib/raygui. Regions layout system (VBox/HBox) with fixed/expanding sizes. `UpdateLayout` recomputes regions and line spans when dirty. Drawing passes: Background, LineNumbers, Text (multi-pass for widgets/syntax), Markers/Hints, Airline.

- **Modes**: Vim mode is default (`pZep`). Standard mode available. Modes own keymaps and airline content. Ex commands implemented for common operations (write, quit, buffer nav, set, folds, etc.).

- **Git integration**: `ZepGit` computes per-line diff status and shows background highlighting in the text region (not a separate gutter).

- **Line numbers**: Right-aligned in number region. Relative numbers supported via `set relativenumber`.

- **Indicators**: Narrow left region (~1.5 chars) shows small colored rectangles for markers (e.g., git status, fold closed state). Not a full diff column.

- **Undo/redo**: Command pattern with groups; dot command replays. Multiple registers supported including `+`/`*` clipboard.

- **REPL support**: Plugin system for external REPLs (Lua, Duktape, QuickJS) disabled by default.

- **Performance**: `RelWithDebInfo` is the recommended build for development (symbols + optimizations). Release builds strip debug info.
