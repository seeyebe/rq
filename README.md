# snub

A suspiciously fast, lightweight recursive file search utility for the Windows command line, written in modern C17. It aims to do what Windows Explorer *thinks* it's doing — but faster, smarter, and with actual control.

## Quick Start

```cmd
:: Find all code files modified this year
snub C:\Dev *.c --ext .c,.h --after 2025-01-01

:: Search for images larger than 500KB
snub D:\Photos beach --ext .jpg,.png --min 512000

:: Export recent documents as JSON
snub C:\Users\me\Documents report --ext .pdf,.docx --after 2024-12-01 --json
```

## Building

This already feels dangerous.

### Option 1: GCC (Recommended)

```cmd
gcc -std=c17 -Wall -Wextra -Wpedantic -O3 -march=native src/search.c src/cli.c src/utils.c main.c -o snub.exe
.\snub.exe
```

### Option 2: Make (MinGW or Bash)

```sh
make
./build/snub.exe
```

### Option 3: CMake (Visual Studio 2022) - Known Issues

```cmd
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
.\build\Release\snub.exe
```

*Note: The CMake/MSVC build currently has compatibility issues. Use GCC for best results.*