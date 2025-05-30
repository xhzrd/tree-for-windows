# tree-for-windows

📁 .  
├── 📁 src  
│   └── 📄 main.cpp  
├── 📄 Makefile  
├── 📄 cc.py  
└── 📄 compile_commands.json  


## What is this?

A fast, colorful, advanced Windows command-line `tree` utility written in C++20 using `std::filesystem`.  
Features:  
- Unicode icons (📁📄🔗) for directories, files, symlinks  
- Color output using native Windows console API  
- Command line options with `--max-deep-recursive=N` to limit recursion depth  
- Handles access permission errors gracefully  
- Uses clang++ and optimized Makefile for small builds  

## Usage

```bash
tree [path] [--max-deep-recursive=N]
````

* `path` - Root directory to list (defaults to current directory)
* `--max-deep-recursive=N` - Limit directory recursion depth (default: unlimited)
* `-h`, `--help` - Show this help message

Example:

```bash
tree C:\Users --max-deep-recursive=3
```

## Building

Make sure you have `clang++` installed on Windows (via LLVM installer or WSL).
Run:

```bash
make
```

This compiles `src/main.cpp` into `build/tree.exe` with C++20 and optimizations.

---

## License

This project is licensed under the Apache License 2.0. See [LICENSE](LICENSE) for details.