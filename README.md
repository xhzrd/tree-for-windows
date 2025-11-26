# tree-for-windows

A fast, colorful, feature-rich Windows command-line `tree` utility written in C++20 using `std::filesystem`.

```
📁 .
├── 📁 src
│   └── 📄 main.cpp
├── 📄 .gitignore
├── 📄 LICENSE
├── 📄 Makefile
├── 📄 README.md
└── 📄 cc.py
```

## Features

- 🎨 **Colorful output** - Customizable colors using native Windows console API
- 📁 **Unicode icons** - Beautiful icons for directories (📁), files (📄), and symlinks (🔗)
- 🚫 **Smart filtering** - `.gitignore` support and automatic hiding of common system folders
- ⚙️ **Persistent config** - Save your preferences with `--set-config`
- 🎯 **Flexible options** - Control depth, ignore patterns, colors, and more
- ⚡ **Fast & efficient** - Built with C++20 and optimized compilation
- 🛡️ **Robust** - Handles permission errors and edge cases gracefully

## Installation

### Prerequisites
- `clang++` (via LLVM installer or MinGW-w64)
- Windows 10/11

### Build
```bash
make
```

This makes a build directory, Then you need to do:
```bash
# Balanced for speed & size
make

# Optimized for Tiny size
make tiny

# Optimized for Tiny size
make fast
```

This compiles `src/main.cpp` into `build/tree.exe` with C++20 optimizations.

## Usage

```bash
tree [path] [options]
```

### Options

| Option | Description |
|--------|-------------|
| `path` | Root directory to list (default: current directory) |
| `--depth=N` | Limit recursion depth to N levels (default: unlimited) |
| `--gitignore` | Skip files/folders matching `.gitignore` patterns |
| `--ignore=folder1,folder2` | Ignore specific folders or files |
| `--show-hidden` | Show `.git` and other hidden system folders |
| `--color=COLOR` | Set directory color: `cyan`, `yellow`, `blue`, `magenta`, `red` |
| `--set-config [options]` | Save options as defaults for future use |
| `--help`, `-h` | Show help message |

### Examples

**Basic usage:**
```bash
tree
```

**Limit depth:**
```bash
tree --depth=3
```

**Use gitignore filtering:**
```bash
tree --gitignore
```

**Custom color:**
```bash
tree --color=yellow
```

**Ignore specific folders:**
```bash
tree --ignore=node_modules,dist,.vscode
```

**Show hidden folders:**
```bash
tree --show-hidden --gitignore
```

**Save your preferences:**
```bash
# Set defaults once
tree --set-config --gitignore --color=cyan --depth=5

# Now tree uses these defaults every time
tree

# Override when needed
tree --depth=2 --color=yellow
```

**Clear saved config:**
```bash
tree --set-config
```

## Default Behavior

By default, `tree` automatically hides common system and build folders:
- `.git`
- `.cache`
- `node_modules`
- `.vscode`
- `.idea`
- `__pycache__`

Use `--show-hidden` to display these folders.

## Configuration

Your preferences are saved to `%APPDATA%\.tree-for-windows\config.txt` when using `--set-config`.

Command-line options always override saved configuration.

## License

This project is licensed under the Apache License 2.0. See [LICENSE](LICENSE) for details.

## Contributing

Contributions are welcome! Feel free to open issues or submit pull requests.

---

**Made with ❤️ for Windows developers who want a better tree command.**