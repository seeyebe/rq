# rq

**rq** is a high-performance recursive file search utility for Windows, written in modern C17.

## Quick Start

```bash
# Find all C/C++ source files modified this year
rq C:\Dev "*.c" --glob --ext c,h --after 2025-01-01

# Search for large images
rq D:\Photos beach --ext jpg,png --size +500K

# Export recent documents as JSON
rq C:\Users\me\Documents report --ext pdf,docx --after 2025-01-01 --json
```

---

## Usage

```text
rq - fast file search tool for Windows

Usage: rq <directory> <pattern> [OPTIONS]

Arguments:
  <directory>         The directory to search in
  <pattern>           Search pattern (use --glob for wildcards)

Search Options:
  -c, --case              Case-sensitive search
  -g, --glob              Enable glob patterns (* ? [] {})
  -r, --regex             Enable regex patterns (filename matching)
  -H, --include-hidden    Include hidden files and directories
  -L, --follow-symlinks   Follow symbolic links
      --no-skip           Don't skip common directories (node_modules, .git, etc.)

Filters:
  -e, --ext <list>    Filter by file extensions (comma-separated)
  -t, --type <type>   Filter by file type (text, image, video, audio, archive)
      --min <size>    Minimum file size (supports K, M, G, T suffixes)
      --max <size>    Maximum file size (supports K, M, G, T suffixes)
      --size <size>   Exact file size, or +size (larger), -size (smaller)
      --after <date>  Files modified after date (YYYY-MM-DD)
      --before <date> Files modified before date (YYYY-MM-DD)
  -d, --max-depth <n> Maximum recursion depth (0 = no recursion, default = unlimited)
      --max-results <n>   Maximum number of results (0 = unlimited)

Performance:
  -j, --threads <n>   Number of worker threads (0 = auto)
      --timeout <ms>  Search timeout in milliseconds
      --stats         Show real-time thread pool statistics

Output:
      --preview [<n>]     Show preview of text files (default: 10 lines)
      --out <file>        Write output to file
      --json              Output results as JSON

General:
  -h, --help          Show this help message
  -V, --version       Show version information

Examples:
  Search for all PNG files:
    rq D:\ "*.png" --glob

  Find documents larger than 1MB:
    rq . document --min 1M --ext pdf,docx

  Find files smaller than 100KB:
    rq . "" --size -100K --ext txt

  Case-sensitive search with thread monitoring:
    rq C:\ "Config" --case --stats --threads 8

For glob patterns: * (any chars), ? (single char), [abc] (char set), {jpg,png} (alternatives)
```

---

## Building

### Option 1: GCC (Recommended)

```bash
gcc -std=c17 -Wall -Wextra -O3 -I src src/*.c -lshlwapi -o rq.exe
```

### Option 2: Make (MinGW or Bash)

```bash
make
./build/rq.exe
```

---

## License

rq is released under the MIT License. See the [LICENSE](LICENSE) file for full license details.

Copyright (c) 2025 rq Contributors. All rights reserved.
