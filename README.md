# snub

A suspiciously fast, lightweight recursive file search utility for the Windows command line, written in modern C17. It aims to do what Windows Explorer *thinks* it's doing — but faster, smarter, and with actual control.

## Quick Start

```cmd
:: Find all code files modified this year
snub C:\Dev "*.c" --glob --ext c,h --after 2025-01-01

:: Search for images larger than 500KB
snub D:\Photos beach --ext jpg,png --min 500K

:: Export recent documents as JSON
snub C:\Users\me\Documents report --ext pdf,docx --after 2024-12-01 --json
```

## Building

This already feels dangerous.

### Option 1: GCC (Recommended)

```cmd
gcc -std=c17 -Wall -Wextra -Wpedantic -I src src/*.c main.c -lshlwapi -o snub.exe
.\snub.exe
```

### Option 2: Make (MinGW or Bash)

```sh
make
./build/snub.exe
```