#include <fileapi.h>
#include <windows.h>
#include <iostream>
#include <filesystem>
#include <string>
#include <vector>
#include <algorithm>
#include <fcntl.h> // For _O_U8TEXT

namespace fs = std::filesystem;

// Colors for Windows console using SetConsoleTextAttribute
enum class Color : WORD {
    DEFAULT = 7,
    DIR = 11,      // Light cyan
    FILE = 7,      // White
    SYMBOLIC_LINK = 13, // Light magenta
    ERRORa = 12     // Light red
};

void set_console_color(HANDLE hConsole, Color color) {
    SetConsoleTextAttribute(hConsole, static_cast<WORD>(color));
}

void print_usage() {
    std::cout <<
        "Usage: tree [path] [--max-deep-recursive=N]\n"
        "Options:\n"
        "  path                     Root directory to list (default: current directory)\n"
        "  --max-deep-recursive=N   Limit recursion depth to N (default: unlimited)\n";
}

struct options {
    fs::path root_path = ".";
    int max_depth = -1; // -1 means unlimited
};

bool parse_args(int argc, char* argv[], options& opt) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            print_usage();
            return false;
        }
        else if (arg.starts_with("--max-deep-recursive=")) {
            std::string val = arg.substr(21);
            try {
                opt.max_depth = std::stoi(val);
                if (opt.max_depth < 0) throw std::invalid_argument("negative");
            }
            catch (...) {
                std::cerr << "Invalid value for --max-deep-recursive\n";
                return false;
            }
        }
        else if (arg.starts_with("-")) {
            std::cerr << "Unknown option: " << arg << "\n";
            return false;
        }
        else {
            opt.root_path = arg;
        }
    }
    return true;
}

std::string repeat_str(const std::string& s, int n) {
    std::string out;
    for (int i = 0; i < n; ++i) out += s;
    return out;
}

void print_tree(const fs::path& path, HANDLE hConsole, int depth, int max_depth, bool is_last) {
    // limit recursion
    if (max_depth != -1 && depth > max_depth) return;

    // indentation + branch symbol
    if (depth > 0) {
        std::cout << repeat_str("│   ", depth - 1);
        std::cout << (is_last ? "└── " : "├── ");
    }

    // Determine file type & print icon + name with color
    DWORD attrs = GetFileAttributesW(path.c_str());

    bool is_dir = (attrs != INVALID_FILE_ATTRIBUTES) && (attrs & FILE_ATTRIBUTE_DIRECTORY);
    bool is_reparse_point = (attrs != INVALID_FILE_ATTRIBUTES) && (attrs & FILE_ATTRIBUTE_REPARSE_POINT);

    if (is_dir) {
        set_console_color(hConsole, Color::DIR);
        std::cout << "📁 " << path.filename().string() << "\n";
    }
    else if (is_reparse_point) {
        set_console_color(hConsole, Color::SYMBOLIC_LINK);
        std::cout << "🔗 " << path.filename().string() << "\n";
    }
    else {
        set_console_color(hConsole, Color::FILE);
        std::cout << "📄 " << path.filename().string() << "\n";
    }
    set_console_color(hConsole, Color::DEFAULT);

    if (is_dir) {
        std::vector<fs::directory_entry> entries;
        try {
            for (auto& entry : fs::directory_iterator(path))
                entries.push_back(entry);
        }
        catch (const std::exception& e) {
            set_console_color(hConsole, Color::ERRORa);
            std::cout << repeat_str("│   ", depth) << "[Access Denied]\n";
            set_console_color(hConsole, Color::DEFAULT);
            return;
        }

        // Sort entries by name (folders first)
        std::sort(entries.begin(), entries.end(), [](const fs::directory_entry& a, const fs::directory_entry& b) {
            bool a_is_dir = a.is_directory();
            bool b_is_dir = b.is_directory();
            if (a_is_dir != b_is_dir)
                return a_is_dir > b_is_dir;
            return a.path().filename().wstring() < b.path().filename().wstring();
        });

        for (size_t i = 0; i < entries.size(); ++i) {
            bool last = (i == entries.size() - 1);
            print_tree(entries[i].path(), hConsole, depth + 1, max_depth, last);
        }
    }
}

int main(int argc, char* argv[]) {
    options opt;
	 // Set console output to UTF-8
    SetConsoleOutputCP(CP_UTF8);

    if (!parse_args(argc, argv, opt)) {
        return 1;
    }

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to get console handle.\n";
        return 1;
    }

    // Check if path exists and is directory
    if (!fs::exists(opt.root_path)) {
        std::cerr << "Path does not exist: " << opt.root_path.string() << "\n";
        return 1;
    }

    // Print root folder itself
    set_console_color(hConsole, Color::DIR);
    // std::cout << "📁 " << fs::absolute(opt.root_path).filename().string() << "\n";
    set_console_color(hConsole, Color::DEFAULT);

    print_tree(opt.root_path, hConsole, 0, opt.max_depth, true);

    return 0;
}