# Static Analysis and Code Quality CI

This workflow provides stricter code quality checks for the FSW repository.

## Jobs

### 1. Compiler Warnings (`compiler-warnings`)
- Builds all projects with `-Wall` enabled
- Catches common coding errors and potential issues at compile time
- Runs on `native_sim` platform

### 2. Address Sanitizer (`asan-build`)
- Builds and tests with AddressSanitizer (ASAN)
- Detects memory errors:
  - Use-after-free
  - Heap buffer overflow
  - Stack buffer overflow
  - Memory leaks
- Only runs on `native_sim` (Linux-only)

### 3. Undefined Behavior Sanitizer (`ubsan-build`)
- Builds and tests with UndefinedBehaviorSanitizer (UBSAN)
- Detects undefined behavior:
  - Integer overflows
  - Null pointer dereferences
  - Invalid type casts
  - Alignment violations
- Only runs on `native_sim` (Linux-only)

### 4. Valgrind Memory Checker (`valgrind-test`)
- Runs executables under Valgrind
- Additional memory error detection
- Detects memory leaks and invalid memory access
- Uses suppressions file for known Zephyr false positives

## Using Snippets Locally

You can use these analysis tools locally when building:

```bash
# Build with ASAN
west build -b native_sim app/your-app -- -DSNIPPET=asan

# Build with UBSAN
west build -b native_sim app/your-app -- -DSNIPPET=ubsan

# Build with compiler warnings
west build -b native_sim app/your-app -- -DSNIPPET=compiler-warnings
```

## Global Compiler Warnings

All builds now include `-Wall` by default through the root `CMakeLists.txt`.
This ensures production code meets stricter quality standards.

## References

- [Zephyr Static Code Analysis](https://docs.zephyrproject.org/latest/develop/sca/index.html)
- [Zephyr Native Sim with Sanitizers](https://docs.zephyrproject.org/latest/boards/native/native_sim/doc/index.html)
