# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.1-2026-06-16] - 2026-06-16

### Added
- Initial "feature complete" state with four independent adventure games:
  - **Chateau Gaillard**
  - **Citadel of Pershu**
  - **Asimovian Aftermath**
  - **Werewolves and Wanderer**
- New documentation for modern C23 resource management:
  - `docs/c23_embed_approach.md`: Using the `#embed` directive.
  - `docs/runtime_path_discovery.md`: Finding assets relative to the executable.
- Support for Address and Leak Sanitizers in terminal builds.

### Changed
- Updated `README.md` with comprehensive build and run instructions using both standard and CLion-bundled CMake.
- Refactored data loading to use the `MONSTER_DATA_PATH` compile-time macro, ensuring `monsters.json` can be found regardless of the current working directory.
- Configured CMake to automatically detect compiler support for sanitizer flags.
- Updated shell configuration (`.zshrc`) to use Homebrew LLVM Clang by default for better C23 compatibility.

### Fixed
- Resolved `unsupported option '-fsanitize=leak'` errors when building with default Apple Clang on macOS.
- Fixed several C23 compatibility issues (e.g., `constexpr` usage) and missing file newlines.
