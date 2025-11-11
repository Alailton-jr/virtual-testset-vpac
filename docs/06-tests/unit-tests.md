# Unit Tests

## Overview

This document describes the unit test implementation for the Virtual TestSet frontend application using Vitest and React Testing Library.

## Testing Framework

- **Test Runner**: Vitest
- **Testing Library**: React Testing Library
- **Coverage Tool**: Vitest Coverage (c8)

## Test Files

### Protection Function Tests

#### 1. Overcurrent Protection (`OvercurrentTestPage.test.tsx`)
**18 tests covering:**
- Page rendering and UI components
- Input configuration (pickup, time dial, curve type)
- Test execution and results display
- Validation of numeric inputs
- Accessibility features

#### 2. Differential Protection (`DifferentialTestPage.test.tsx`)
**20 tests covering:**
- Page rendering with stream selection
- Slope and restraint configuration
- Test point management
- Test execution with stream dependencies
- Id vs Ir characteristic validation
- Accessibility features

#### 3. Distance Protection (`DistanceTestPage.test.tsx`)
**21 tests covering:**
- Page rendering and configuration
- R-X impedance point management
- Test point addition and deletion
- Test execution on impedance plane
- R-X diagram visualization
- Accessibility features

#### 4. Ramping Test (`RampingTestPage.test.tsx`)
**26 tests covering:**
- Page rendering with stream selection
- Ramp configuration (start, end, step, duration)
- Variable selection (voltage, current, frequency)
- Test KPIs (pickup, dropout, reset)
- Test execution state management
- Accessibility features

## Test Categories

### Rendering Tests
- Verify page titles, descriptions, and UI structure
- Check for presence of all input fields
- Validate button availability

### Input Configuration Tests
- Default values verification
- User input changes
- Dropdown/select interactions
- Numeric validation with step increments

### Test Execution Tests
- Button state changes (enabled/disabled)
- Result display after test completion
- Pass/fail status indicators
- Test point status updates

### Validation Tests
- Input range validation
- Required field enforcement
- Numeric precision handling

### Accessibility Tests
- Label associations (for screen readers)
- Button accessibility
- Keyboard navigation support
- ARIA attributes

## Running Tests

```bash
# Run all tests
cd frontend
npm test

# Run tests with coverage
npm run test:coverage

# Run tests in watch mode
npm run test:watch

# Run specific test file
npm test OvercurrentTestPage.test.tsx
```

## Test Coverage Goals

| Protection Function | Total Tests | Target Coverage |
|-------------------|-------------|-----------------|
| Overcurrent 50/51 | 18 | >90% |
| Differential 87 | 20 | >90% |
| Distance 21 | 21 | >90% |
| Ramping | 26 | >90% |

## Best Practices

1. **Test User Interactions**: Use `userEvent` from `@testing-library/user-event` for simulating user interactions
2. **Async Handling**: Use `waitFor`, `findBy*` queries for async operations
3. **Accessibility**: Include tests for ARIA labels, keyboard navigation
4. **Mocking**: Mock API calls and external dependencies
5. **Isolation**: Each test should be independent and not rely on others

## Continuous Integration

Unit tests are automatically run on:
- Pull requests
- Main branch commits
- Pre-commit hooks (optional)

## Troubleshooting

### Common Issues

**Tests timing out:**
- Increase timeout in `vitest.config.ts`
- Check for unresolved promises

**Mock data not working:**
- Verify mock setup in test files
- Check API mock responses match expected format

**Accessibility tests failing:**
- Ensure proper ARIA labels
- Check form field associations
