# Text Adventures

A C23 project for building text-based adventure games.

## Features
- Written in C23.
- Modular structure.

## Getting Started

### Prerequisites
- CMake 4.2 or higher.
- A C23 compatible compiler (e.g., GCC 13+ or Clang 18+).

### Data Loading
The project uses `MONSTER_DATA_PATH` (defined via CMake) to locate `monsters.json` at runtime. Other common approaches in C include:
- **C23 #embed**: Baking data directly into the binary. See [docs/c23_embed_approach.md](docs/c23_embed_approach.md).
- **Runtime Path Discovery**: Dynamically finding assets relative to the executable. See [docs/runtime_path_discovery.md](docs/runtime_path_discovery.md).

### Building
```bash
# Ensure you are using the Homebrew LLVM compiler (configured in ~/.zshrc)
# If not, you can specify it manually:
# CC=/usr/local/opt/llvm/bin/clang CXX=/usr/local/opt/llvm/bin/clang++ cmake -S . -B build

Run this from the project root:

Configure (Debug)   cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
Configure (Release) cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
Build Everything    cmake --build build
Clean Build         cmake --build build --target clean

Building Specific Targets
cmake --build build --target chateau_gaillard
```

### Running
```bash
./build/chateau_gaillard
```
