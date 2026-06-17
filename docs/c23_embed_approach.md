### C23 #embed Approach for Resource Management

In C23, the `#embed` preprocessor directive provides a standard way to include binary data directly into your executable. This is the modern equivalent of Java's resource loading from a JAR file, solving the problem of "where is my data file?" by making the data part of the binary itself.

#### 1. Why use #embed?
- **Portability**: No need to worry about relative paths or working directories at runtime.
- **Simplicity**: No complex build scripts to convert binary files to C arrays.
- **Performance**: Data is mapped into memory by the OS loader, often more efficiently than manual file I/O.

#### 2. Implementation Example
Instead of using a file path and `fopen`, you can embed `monsters.json` directly into your code:

```c
#include <stdio.h>

// Embedding the data as a constant array
static const unsigned char monster_data[] = {
#embed "monsters.json"
};

void monsters_init_embedded() {
    // You can now pass monster_data directly to your JSON parser
    // instead of reading it from a file.
    printf("Loaded %zu bytes of monster data.\n", sizeof(monster_data));
    // parse_json((const char*)monster_data, sizeof(monster_data));
}
```

#### 3. Integration with this Project
To use this approach in the `text_adventures` project:

1.  **Compiler Support**: Ensure you are using a C23-compliant compiler (like the Homebrew LLVM Clang already configured in this project).
2.  **Include Path**: The `#embed` directive searches for files in the same way `#include` does. You might need to add the source directory to your include paths in `CMakeLists.txt`:
    ```cmake
    target_include_directories(${target_name} PRIVATE ${target_source_dir})
    ```
3.  **Code Change**: Replace `monsters_init(MONSTER_DATA_PATH)` with a version that handles memory buffers.

#### 4. Comparison with MONSTER_DATA_PATH
| Feature | MONSTER_DATA_PATH (Current) | #embed (Alternative) |
| :--- | :--- | :--- |
| **Storage** | External file | Inside executable |
| **Runtime** | Requires file access | No file access needed |
| **Updates** | Can swap file without recompile | Requires recompilation |
| **Complexity** | Low (path management) | Very Low (language feature) |

#### 5. Handling Null Termination
If you are embedding a text file (like JSON) and your parser expects a null-terminated string, you can add it manually:

```c
static const char monster_json[] = {
#embed "monsters.json"
, 0 // Manual null terminator
};
```
