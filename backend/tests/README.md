# Unit Tests

This directory contains unit tests for the Virtual Test Set project using Google Test framework.

## Test Coverage

### Phase 1: P0 Hotfixes
- **test_ber_encoding.cpp**: BER length encoding (Phases 1.2, 1.7, 2.2)
  - Short form (≤127 bytes)
  - Long form 0x81 (128-255 bytes)
  - Long form 0x82 (256-65535 bytes)
  - Edge cases: 127, 128, 255, 256, 65535
  - allData >255 bytes encoding

- **test_vlan.cpp**: VLAN parameter validation (Phase 1.8)
  - Priority validation (0-7)
  - VLAN ID validation (0-4095)
  - DEI validation (0-1)
  - TCI encoding correctness

- **test_mac_parser.cpp**: MAC address parsing (Phase 2.3)
  - Valid format (XX:XX:XX:XX:XX:XX)
  - Invalid format rejection
  - Hex digit validation
  - Pre-allocation (std::array return)

- **test_smpCnt_wrap.cpp**: smpCnt wrapping (Phase 1.4)
  - 16-bit wrap at 65535
  - 70,000 sample test (acceptance criteria)
  - Rate-based modulo

### Phase 3: Threading
- **test_threadpool.cpp**: ThreadPool behavior (Phase 3.2)
  - Safe initialization
  - Drain-on-shutdown (100 queued tasks)
  - Thread joining
  - Exception handling

## Building and Running Tests

### Build Tests
```bash
# From project root
cmake -S . -B build
cmake --build build --target vts_tests

# Or use ninja if available
cmake -S . -B build -G Ninja
ninja -C build vts_tests
```

### Run All Tests
```bash
cd build
./tests/vts_tests
```

### Run Specific Test Suites
```bash
# Run only BER encoding tests
./tests/vts_tests --gtest_filter=BEREncodingTest.*

# Run only VLAN tests
./tests/vts_tests --gtest_filter=VLANTest.*

# Run only MAC parser tests
./tests/vts_tests --gtest_filter=MACParserTest.*

# Run only smpCnt wrap tests
./tests/vts_tests --gtest_filter=SmpCntWrapTest.*

# Run only ThreadPool tests
./tests/vts_tests --gtest_filter=ThreadPoolTest.*
```

### Run with Verbose Output
```bash
./tests/vts_tests --gtest_print_time=1 --gtest_color=yes
```

### Run with Sanitizers
```bash
# Address Sanitizer
cmake -S . -B build -DENABLE_ASAN=ON
cmake --build build --target vts_tests
./build/tests/vts_tests

# Thread Sanitizer
cmake -S . -B build -DENABLE_TSAN=ON
cmake --build build --target vts_tests
./build/tests/vts_tests

# Undefined Behavior Sanitizer
cmake -S . -B build -DENABLE_UBSAN=ON
cmake --build build --target vts_tests
./build/tests/vts_tests
```

## CTest Integration

Tests are also registered with CTest:

```bash
# Run all tests via CTest
cd build
ctest --output-on-failure

# Run specific test
ctest -R BER_Encoding --verbose

# Run with parallel execution
ctest -j4 --output-on-failure
```

## Test Structure

Each test file follows this pattern:

```cpp
#include <gtest/gtest.h>
#include "header_to_test.hpp"

// Test fixture (optional)
class MyTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup before each test
    }
    
    void TearDown() override {
        // Cleanup after each test
    }
    
    // Helper functions
};

// Tests
TEST_F(MyTest, TestName) {
    // Arrange
    // Act
    // Assert
    EXPECT_EQ(actual, expected);
}
```

## Assertions

Common Google Test assertions:

- `EXPECT_EQ(a, b)` - Expects a == b
- `EXPECT_NE(a, b)` - Expects a != b
- `EXPECT_LT(a, b)` - Expects a < b
- `EXPECT_LE(a, b)` - Expects a <= b
- `EXPECT_GT(a, b)` - Expects a > b
- `EXPECT_GE(a, b)` - Expects a >= b
- `EXPECT_TRUE(condition)` - Expects condition is true
- `EXPECT_FALSE(condition)` - Expects condition is false
- `EXPECT_THROW(statement, exception_type)` - Expects statement throws
- `EXPECT_NO_THROW(statement)` - Expects statement doesn't throw

Use `ASSERT_*` instead of `EXPECT_*` to stop test execution on failure.

## Adding New Tests

1. Create new test file in `tests/` directory
2. Include Google Test header: `#include <gtest/gtest.h>`
3. Include headers being tested
4. Write test cases using `TEST()` or `TEST_F()` macros
5. Add test file to `tests/CMakeLists.txt`
6. Rebuild and run tests

## Continuous Integration

Tests should be run in CI with:
- All sanitizers (ASAN, TSAN, UBSAN)
- Multiple build configurations (Debug, Release)
- Coverage reporting (if enabled)

## Next Steps

- [ ] Add integration tests for packet replay
- [ ] Add fuzz testing harness for ASN.1 decoders
- [ ] Add performance benchmarks
- [ ] Add coverage reporting
- [ ] Add memory leak detection with Valgrind
