# C++ Formatting Standard for jbig2enc

This project uses `clang-format` to maintain a consistent coding style. The configuration is based on the **Google C++ Style Guide** with some specific modifications to match the existing codebase.

## Using `clang-format`

To format a file, run:
```bash
clang-format -i src/jbig2enc.cc
```

To check for formatting changes without applying them:
```bash
clang-format src/jbig2enc.cc | diff -u src/jbig2enc.cc -
```

## Key Rules

### 1. Indentation
*   Use **2 spaces** for indentation.
*   **Never use tabs**.

### 2. Line Length
*   Try to keep lines under **100 characters**.

### 3. Braces
*   Function definitions: The return type is often on its own line. The opening brace should be on the same line as the function parameters (after any wrapping).
    ```cpp
    static unsigned
    log2up(int v) {
      ...
    }
    ```
*   Control structures (`if`, `for`, `while`, `switch`): Opening brace is on the same line as the statement.

### 4. Spacing
*   Put a space after keywords (`if`, `for`, `while`, `switch`).
*   No space before function call parentheses: `pixInfo(pixd, "msg")`.
*   Pointer and reference alignment is to the **right**: `PIX *pix`, `std::vector<int> &v`.
*   A space should follow logical NOT (`!`) and C-style casts.

### 5. Short Statements
*   Short `if` statements and loops can be on a single line if they are simple:
    ```cpp
    if (verbose) pixInfo(pixd, "mask image: ");
    ```

### 6. Function Parameters
*   When a function declaration or call needs to be wrapped, align the parameters/arguments with the opening bracket.
*   Avoid "bin packing" parameters (prefer one per line if they don't fit on one line).

## Naming Conventions (Recommended)
*   Variables: `snake_case` (e.g., `is_pow_of_2`).
*   Functions: Primarily `snake_case` for internal logic, `camelCase` is also present. Public API uses `jbig2_...`.
*   Macros: `UPPER_CASE_WITH_UNDERSCORES`.

## Comments
*   Use `//` for single-line and multi-line comments.
*   Use a separator like `// -------------------` to divide functions and major sections.
