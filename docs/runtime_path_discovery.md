### Runtime Path Discovery (Relative Asset Lookup)

The third common approach to loading data in C is **Runtime Path Discovery**. Instead of hardcoding a path at compile-time (Macro) or baking the data into the binary (#embed), the program dynamically determines where it is installed and looks for its data files relative to its own location.

#### 1. Why use Runtime Path Discovery?
- **Portability**: You can move the entire application folder to a different location or drive, and it will still find its assets.
- **Flexibility**: You can update the data files (like `monsters.json`) without needing to recompile the executable.
- **Standard Practice**: This is how most professional desktop applications and games handle assets (e.g., looking in a `Resources` or `assets` folder).

#### 2. How it works
The program performs the following steps at startup:
1.  **Get Executable Path**: Ask the OS for the absolute path to the currently running executable.
2.  **Resolve Directory**: Strip the filename from the path to get the directory where the binary lives.
3.  **Check Relative Locations**: Look for the data file in a set of known relative paths (e.g., `./data/`, `../share/`, or the same directory).

#### 3. Implementation (Platform Specific)
Unlike Java's `ClassLoader.getResource()`, standard C does not have a cross-platform function to find the executable's path. You typically use platform-specific APIs:

- **macOS**: `_NSGetExecutablePath()` (from `<mach-o/dyld.h>`)
- **Linux**: `readlink("/proc/self/exe", buffer, size)`
- **Windows**: `GetModuleFileName()`

#### 4. Example Pattern
A common pattern is to implement a `find_asset(const char* filename)` function:

```c
char* get_asset_path(const char* asset_name) {
    char exe_path[1024];
    // 1. Get the path to the executable (macOS example)
    uint32_t size = sizeof(exe_path);
    if (_NSGetExecutablePath(exe_path, &size) != 0) return NULL;

    // 2. Resolve the real path (handles symlinks)
    char real_exe_path[1024];
    realpath(exe_path, real_exe_path);

    // 3. Find the directory (strip filename)
    char* last_slash = strrchr(real_exe_path, '/');
    if (last_slash) *last_slash = '\0';

    // 4. Construct path to asset (e.g., in a 'data' subfolder)
    static char final_path[2048];
    snprintf(final_path, sizeof(final_path), "%s/data/%s", real_exe_path, asset_name);
    
    return final_path;
}
```

#### 5. Comparison
| Feature | Macro Path | #embed | Runtime Discovery |
| :--- | :--- | :--- | :--- |
| **Best For** | Development / Fixed installs | Small assets / Single binaries | Games / Distributed Apps |
| **Data Update** | Requires Recompile | Requires Recompile | No Recompile needed |
| **Executable** | Small | Large (includes data) | Small |
| **Complexity** | Low | Very Low (C23) | Medium (Platform APIs) |
