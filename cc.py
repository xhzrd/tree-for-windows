import json
import os

# === CONFIG ===
project_root = os.path.abspath('.')
src_dir = os.path.join(project_root, 'src')
build_dir = os.path.join(project_root, 'build')
include_dirs = ['include']
compiler = 'clang++'
flags = [
    '-std=c++20',
    '-Wall',
    '-Wextra',
]

def get_cpp_files(directory):
    cpp_files = []
    for root, _, files in os.walk(directory):
        for file in files:
            if file.endswith('.cpp'):
                full_path = os.path.join(root, file)
                cpp_files.append(full_path)
    return cpp_files

def generate_compile_command(file_path):
    rel_path = os.path.relpath(file_path, project_root)
    return {
        'directory': project_root,
        'command': f"{compiler} {' '.join(flags)} -c {rel_path} -o /dev/null",
        'file': rel_path
    }

def main():
    cpp_files = get_cpp_files(src_dir)
    commands = [generate_compile_command(f) for f in cpp_files]

    with open('compile_commands.json', 'w') as f:
        json.dump(commands, f, indent=4)
    print('[+] compile_commands.json generated.')

if __name__ == '__main__':
    main()
