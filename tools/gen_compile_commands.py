#!/usr/bin/env python3
import json
import os
import glob

def main():
    root_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    
    cxx = "clang++"
    std_flags = "-std=c++23 -Wall -Wextra -Wshadow -pedantic -Isrc"
    debug_flags = "-O2 -g -fsanitize=address -fsanitize=undefined -DLOCAL -DDEBUG -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC"
    test_flags = f"{debug_flags} -DVERIFY_CONSISTENCY -Itests"
    
    commands = []
    
    # 1. Engine sources
    engine_sources = glob.glob(os.path.join(root_dir, "src", "*.cpp"))
    for src in engine_sources:
        rel_src = os.path.relpath(src, root_dir)
        # Add entry for main debug build
        cmd = f"{cxx} {std_flags} {debug_flags} -c {rel_src}"
        commands.append({
            "directory": root_dir,
            "command": cmd,
            "file": rel_src
        })
        
        # Add entry for test build (if not main.cpp)
        if os.path.basename(src) != "main.cpp":
            cmd_test = f"{cxx} {std_flags} {test_flags} -c {rel_src}"
            commands.append({
                "directory": root_dir,
                "command": cmd_test,
                "file": rel_src
            })

    # 2. Test sources
    test_sources = (
        glob.glob(os.path.join(root_dir, "tests", "*.cpp")) +
        glob.glob(os.path.join(root_dir, "tests", "unit", "*.cpp")) +
        glob.glob(os.path.join(root_dir, "tests", "integration", "*.cpp"))
    )
    for src in test_sources:
        rel_src = os.path.relpath(src, root_dir)
        cmd = f"{cxx} {std_flags} {test_flags} -c {rel_src}"
        commands.append({
            "directory": root_dir,
            "command": cmd,
            "file": rel_src
        })

    output_path = os.path.join(root_dir, "compile_commands.json")
    with open(output_path, "w") as f:
        json.dump(commands, f, indent=2)
    print(f"Generated compile_commands.json at {output_path}")

if __name__ == "__main__":
    main()
