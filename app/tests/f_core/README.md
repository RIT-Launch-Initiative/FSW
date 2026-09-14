# F_Core Unit Tests

This directory contains unit tests for the core library modules located in `include/f_core` and `lib/f_core`.

## Test Structure

Each test directory should contain:
- `main.c` - Test implementation using ZTest framework
- `CMakeLists.txt` - CMake configuration for the test
- `prj.conf` - Zephyr project configuration
- `testcase.yaml` - Test metadata for Twister discovery
- `Kconfig` - Kconfig configuration file

## Running Tests

Tests are automatically discovered and run by the CI using `west twister`. The CI uses the mappings in `.github/hardware-roots.yaml` to determine which tests to run based on changed files.

To run tests locally (after setting up the Zephyr environment):

```bash
# Run a specific test
west twister -T app/tests/f_core/gnss_utils

# Run all f_core tests
west twister -T app/tests/f_core

# Run tests on a specific platform
west twister -T app/tests/f_core -p native_sim
```

## Writing New Tests

### 1. Create Test Directory Structure

Create a new directory under `app/tests/f_core/` for your module:

```bash
mkdir -p app/tests/f_core/<module_name>
```

### 2. Create Test Implementation (`main.c`)

Use the ZTest framework. Example:

```c
#include <zephyr/ztest.h>
#include <f_core/your/header.h>

ZTEST(test_suite_name, test_case_name) {
    // Test implementation
    zassert_equal(actual, expected, "Error message");
}

ZTEST_SUITE(test_suite_name, NULL, NULL, NULL, NULL, NULL);
```

### 3. Create CMakeLists.txt

```cmake
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr COMPONENTS unittest REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(your_test_name)

# Include f_core headers
include_directories(${ZEPHYR_BASE}/../FSW.git/include)

target_sources(testbinary
  PRIVATE
  main.c
  # Add any source files from lib/f_core if needed
)
```

### 4. Create prj.conf

```
CONFIG_ZTEST=y
# Add any additional configuration needed for your tests
```

### 5. Create testcase.yaml

```yaml
common:
  tags:
    - f_core
    - <module_category>
tests:
  f_core.<module_name>:
    type: unit
```

### 6. Create Kconfig

```
mainmenu "Test Configuration"

source "Kconfig.zephyr"
```

### 7. Update CI Configuration

Add your test to `.github/hardware-roots.yaml`:

```yaml
app/tests/f_core/<module_name>:
  - paths:
      - "app/tests/f_core/<module_name>/**"
      - "include/f_core/path/to/header.h"
      - "lib/f_core/path/to/source.c"  # if applicable
```

## ZTest Assertions

Common ZTest assertions:
- `zassert_true(condition, msg, ...)` - Assert condition is true
- `zassert_false(condition, msg, ...)` - Assert condition is false
- `zassert_equal(a, b, msg, ...)` - Assert a == b
- `zassert_not_equal(a, b, msg, ...)` - Assert a != b
- `zassert_within(actual, expected, delta, msg, ...)` - Assert |actual - expected| <= delta
- `zassert_null(ptr, msg, ...)` - Assert pointer is NULL
- `zassert_not_null(ptr, msg, ...)` - Assert pointer is not NULL

## Examples

See existing tests for reference:
- `app/tests/utils/` - C++ debouncer tests
- `app/tests/f_core/gnss_utils/` - GNSS utility function tests
- `app/tests/f_core/golay/` - Golay protocol encoding/decoding tests

## References

- [Zephyr ZTest Documentation](https://docs.zephyrproject.org/latest/develop/test/ztest.html)
- [Zephyr Twister Documentation](https://docs.zephyrproject.org/latest/develop/test/twister.html)
