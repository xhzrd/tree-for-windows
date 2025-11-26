#include <fileapi.h>
#include <io.h>
#include <windows.h>
#include <iostream>
#include <filesystem>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <unordered_set>
#include <sstream>
#include <regex>

namespace fs = std::filesystem;

enum class Color : WORD {
    DEFAULT = 7,
    BLACK = 0,
    BLUE = 1,
    GREEN = 2,
    CYAN = 3,
    RED = 4,
    MAGENTA = 5,
    YELLOW = 6,
    LIGHT_BLUE = 9,
    LIGHT_CYAN = 11,
    LIGHT_MAGENTA = 13,
    LIGHT_RED = 12,
    WHITE = 15,
    LIGHT_YELLOW = 14
};

void set_console_color(HANDLE hConsole, Color color) {
    SetConsoleTextAttribute(hConsole, static_cast<WORD>(color));
}

struct options {
    fs::path root_path = ".";
    int max_depth = -1;  // -1 means unlimited
    std::unordered_set<std::wstring> ignore_paths;
    std::vector<std::regex> gitignore_patterns;
    bool use_gitignore = false;
    bool show_hidden = false;  // Show .git and other hidden folders
    Color dir_color = Color::LIGHT_CYAN;
    Color file_color = Color::DEFAULT;
    Color link_color = Color::LIGHT_MAGENTA;
    Color error_color = Color::LIGHT_RED;
};

fs::path get_config_path() {
    char* appdata = std::getenv("APPDATA");
    if (!appdata) {
        std::cerr << "Could not find APPDATA environment variable\n";
        return "";
    }
    fs::path config_dir = fs::path(appdata) / ".tree-for-windows";
    if (!fs::exists(config_dir)) {
        fs::create_directories(config_dir);
    }
    return config_dir / "config.txt";
}

void save_config(const std::vector<std::string>& args) {
    fs::path config_path = get_config_path();
    if (config_path.empty()) return;

    std::ofstream file(config_path);
    if (!file.is_open()) {
        std::cerr << "Failed to save config to: " << config_path.string() << "\n";
        return;
    }

    for (const auto& arg : args) {
        file << arg << "\n";
    }
    
    std::cout << "Configuration saved to: " << config_path.string() << "\n";
    std::cout << "Saved options:\n";
    for (const auto& arg : args) {
        std::cout << "  " << arg << "\n";
    }
}

std::vector<std::string> load_config() {
    std::vector<std::string> args;
    fs::path config_path = get_config_path();
    if (config_path.empty() || !fs::exists(config_path)) return args;

    std::ifstream file(config_path);
    if (!file.is_open()) return args;

    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) {
            args.push_back(line);
        }
    }
    return args;
}

void print_usage() {
    std::cout <<
        "Usage: tree [path] [options]\n"
        "Options:\n"
        "  path                     Root directory to list (default: current directory)\n"
        "  --depth=N                Limit recursion depth to N (default: unlimited)\n"
        "  --ignore=folder1,folder2 Ignore listed folders/files\n"
        "  --gitignore              Skip files/folders matching .gitignore\n"
        "  --show-hidden            Show .git and other hidden system folders\n"
        "  --color=COLOR            Set directory color (cyan, yellow, blue, magenta, red)\n"
        "  --set-config [options]   Save options as defaults for future use\n"
        "  --help, -h               Show this help message\n";
}

// Convert gitignore pattern to regex, handling ** and * properly
std::string gitignore_to_regex(const std::string& pattern) {
    std::string regex_pattern = "^";
    bool starts_with_slash = !pattern.empty() && pattern[0] == '/';
    
    size_t start = starts_with_slash ? 1 : 0;
    
    for (size_t i = start; i < pattern.size(); ++i) {
        if (pattern[i] == '*' && i + 1 < pattern.size() && pattern[i + 1] == '*') {
            regex_pattern += ".*";
            i++; // Skip the second *
            if (i + 1 < pattern.size() && pattern[i + 1] == '/') {
                regex_pattern += "/?";
                i++; // Skip the /
            }
        } else if (pattern[i] == '*') {
            regex_pattern += "[^/]*";
        } else if (pattern[i] == '?') {
            regex_pattern += "[^/]";
        } else if (pattern[i] == '.') {
            regex_pattern += "\\.";
        } else if (pattern[i] == '/') {
            regex_pattern += "/";
        } else {
            regex_pattern += pattern[i];
        }
    }
    
    // If pattern ends with /, match directories
    if (!pattern.empty() && pattern.back() == '/') {
        regex_pattern += ".*";
    } else {
        regex_pattern += "(/.*)?";
    }
    
    regex_pattern += "$";
    return regex_pattern;
}

std::vector<std::regex> parse_gitignore(const fs::path& root) {
    std::vector<std::regex> patterns;
    fs::path gitignore_path = root / ".gitignore";
    
    std::ifstream f(gitignore_path);
    if (!f.is_open()) return patterns;

    std::string line;
    while (std::getline(f, line)) {
        // Remove whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        
        if (line.empty() || line[0] == '#') continue;
        
        try {
            std::string regex_str = gitignore_to_regex(line);
            patterns.emplace_back(regex_str, std::regex::ECMAScript | std::regex::icase);
        } catch (...) {
            std::cerr << "Warning: Invalid pattern in .gitignore: " << line << "\n";
        }
    }
    return patterns;
}

bool parse_args(const std::vector<std::string>& args, options& opt) {
    for (size_t i = 0; i < args.size(); ++i) {
        std::string arg = args[i];
        if (arg == "--help" || arg == "-h") {
            print_usage();
            return false;
        }
        else if (arg.starts_with("--depth=")) {
            try {
                opt.max_depth = std::stoi(arg.substr(8));
                if (opt.max_depth < 0) throw std::invalid_argument("negative");
            }
            catch (...) {
                std::cerr << "Invalid value for --depth\n";
                return false;
            }
        }
        else if (arg.starts_with("--ignore=")) {
            std::string val = arg.substr(9);
            std::istringstream ss(val);
            std::string token;
            while (std::getline(ss, token, ',')) {
                try {
                    fs::path ignore_path = token;
                    if (!fs::exists(ignore_path)) {
                        std::cerr << "Warning: Ignore path does not exist: " << token << "\n";
                    } else {
                        opt.ignore_paths.insert(fs::absolute(ignore_path).wstring());
                    }
                } catch (...) {
                    std::cerr << "Warning: Invalid ignore path: " << token << "\n";
                }
            }
        }
        else if (arg == "--gitignore") {
            opt.use_gitignore = true;
        }
        else if (arg == "--show-hidden") {
            opt.show_hidden = true;
        }
        else if (arg.starts_with("--color=")) {
            std::string col = arg.substr(8);
            if (col == "cyan") opt.dir_color = Color::LIGHT_CYAN;
            else if (col == "yellow") opt.dir_color = Color::LIGHT_YELLOW;
            else if (col == "blue") opt.dir_color = Color::LIGHT_BLUE;
            else if (col == "magenta") opt.dir_color = Color::LIGHT_MAGENTA;
            else if (col == "red") opt.dir_color = Color::LIGHT_RED;
        }
        else if (!arg.starts_with("--")) {
            opt.root_path = arg;
        }
    }
    return true;
}

std::string wstring_to_utf8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string result(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &result[0], size, NULL, NULL);
    return result;
}

bool is_ignored(const fs::path& path, const fs::path& root, const options& opt) {
    fs::path abs = fs::absolute(path);
    
    // Check explicit ignore paths
    for (const auto& ignore_path : opt.ignore_paths) {
        fs::path ignore_abs(ignore_path);
        if (fs::equivalent(abs, ignore_abs)) {
            return true;
        }
    }
    
    // Hide common system/build folders unless --show-hidden is used
    if (!opt.show_hidden) {
        std::string filename = wstring_to_utf8(path.filename().wstring());
        if (filename == ".git" || filename == ".cache" || filename == "node_modules" || 
            filename == ".vscode" || filename == ".idea" || filename == "__pycache__") {
            return true;
        }
    }
    
    if (opt.use_gitignore && !opt.gitignore_patterns.empty()) {
        // Get relative path from root
        fs::path rel = fs::relative(abs, fs::absolute(root));
        std::string rel_str = rel.string();
        
        // Normalize path separators to forward slashes
        std::replace(rel_str.begin(), rel_str.end(), '\\', '/');
        
        for (auto& r : opt.gitignore_patterns) {
            if (std::regex_match(rel_str, r)) {
                return true;
            }
        }
    }
    return false;
}

void print_tree(const fs::path& path, const fs::path& root, HANDLE hConsole, int depth,
                int max_depth, bool is_last,
                const options& opt, const std::vector<bool>& last_flags = {}) {
    if (max_depth != -1 && depth > max_depth) return;
    if (is_ignored(path, root, opt)) return;

    // Print the prefix (tree branches)
    for (size_t i = 0; i < last_flags.size(); ++i) {
        std::cout << (last_flags[i] ? "    " : "│   ");
    }
    
    if (depth > 0) {
        std::cout << (is_last ? "└── " : "├── ");
    }

    // Get file attributes
    DWORD attrs = GetFileAttributesW(path.c_str());
    bool is_dir = (attrs != INVALID_FILE_ATTRIBUTES) && (attrs & FILE_ATTRIBUTE_DIRECTORY);
    bool is_reparse_point = (attrs != INVALID_FILE_ATTRIBUTES) && (attrs & FILE_ATTRIBUTE_REPARSE_POINT);

    // Print the item name with appropriate icon and color
    std::string filename = wstring_to_utf8(path.filename().wstring());
    
    if (is_reparse_point) {
        set_console_color(hConsole, opt.link_color);
        std::cout << "🔗 " << filename;
        set_console_color(hConsole, Color::DEFAULT);
        std::cout << "\n";
    } else if (is_dir) {
        set_console_color(hConsole, opt.dir_color);
        std::cout << "📁 " << filename;
        set_console_color(hConsole, Color::DEFAULT);
        std::cout << "\n";
    } else {
        set_console_color(hConsole, opt.file_color);
        std::cout << "📄 " << filename;
        set_console_color(hConsole, Color::DEFAULT);
        std::cout << "\n";
    }

    // If it's a directory, recurse into it
    if (is_dir && !is_reparse_point) {
        std::vector<fs::directory_entry> entries;
        try {
            for (auto& entry : fs::directory_iterator(path))
                entries.push_back(entry);
        } catch (...) {
            set_console_color(hConsole, opt.error_color);
            for (size_t i = 0; i < last_flags.size(); ++i) {
                std::cout << (last_flags[i] ? "    " : "│   ");
            }
            std::cout << (is_last ? "    " : "│   ");
            std::cout << "[Access Denied]\n";
            set_console_color(hConsole, Color::DEFAULT);
            return;
        }

        // Sort: directories first, then alphabetically
        std::sort(entries.begin(), entries.end(), [](const fs::directory_entry& a, const fs::directory_entry& b) {
            bool a_is_dir = a.is_directory();
            bool b_is_dir = b.is_directory();
            if (a_is_dir != b_is_dir) return a_is_dir > b_is_dir;
            return a.path().wstring() < b.path().wstring();
        });

        // Recurse into each entry
        for (size_t i = 0; i < entries.size(); ++i) {
            bool last = (i == entries.size() - 1);
            std::vector<bool> next_flags = last_flags;
            if (depth > 0) {
                next_flags.push_back(is_last);
            }
            print_tree(entries[i].path(), root, hConsole, depth + 1, max_depth, last, opt, next_flags);
        }
    }
}

int main(int argc, char* argv[]) {
    SetConsoleOutputCP(CP_UTF8);
    setvbuf(stdout, nullptr, _IONBF, 0);  // Disable buffering to prevent padding

    // Check for --set-config
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--set-config") {
            std::vector<std::string> config_args;
            for (int j = i + 1; j < argc; ++j) {
                config_args.push_back(argv[j]);
            }
            save_config(config_args);
            return 0;
        }
    }

    // Load config defaults
    std::vector<std::string> config_args = load_config();
    
    // Add command line args
    std::vector<std::string> all_args = config_args;
    for (int i = 1; i < argc; ++i) {
        all_args.push_back(argv[i]);
    }

    options opt;
    if (!parse_args(all_args, opt)) return 1;

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to get console handle.\n";
        return 1;
    }

    if (!fs::exists(opt.root_path)) {
        std::cerr << "Path does not exist: " << opt.root_path.string() << "\n";
        return 1;
    }

    // Parse gitignore if requested
    if (opt.use_gitignore) {
        opt.gitignore_patterns = parse_gitignore(opt.root_path);
    }

    // Store absolute root path for relative path calculations
    fs::path abs_root = fs::absolute(opt.root_path);

    // Print root directory
    std::string root_name = wstring_to_utf8(abs_root.filename().wstring());
    if (root_name.empty()) root_name = ".";
    
    set_console_color(hConsole, opt.dir_color);
    std::cout << "📁 " << root_name;
    set_console_color(hConsole, Color::DEFAULT);
    std::cout << "\n";

    // Print tree starting from root's children
    std::vector<fs::directory_entry> entries;
    try {
        for (auto& entry : fs::directory_iterator(opt.root_path))
            entries.push_back(entry);
    } catch (...) {
        std::cerr << "Cannot access directory\n";
        return 1;
    }

    std::sort(entries.begin(), entries.end(), [](const fs::directory_entry& a, const fs::directory_entry& b) {
        bool a_is_dir = a.is_directory();
        bool b_is_dir = b.is_directory();
        if (a_is_dir != b_is_dir) return a_is_dir > b_is_dir;
        return a.path().wstring() < b.path().wstring();
    });

    for (size_t i = 0; i < entries.size(); ++i) {
        bool last = (i == entries.size() - 1);
        print_tree(entries[i].path(), abs_root, hConsole, 1, opt.max_depth, last, opt, {});
    }

    return 0;
}