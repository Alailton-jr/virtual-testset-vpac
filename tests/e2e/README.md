# E2E Tests

End-to-end tests for Virtual TestSet using Playwright.

## Prerequisites

- Node.js 20+
- Docker and Docker Compose (for running the full application)
- Playwright browsers installed

## Installation

```bash
cd tests/e2e
npm install
npx playwright install chromium
```

## Running Tests

### Full Test Suite

```bash
npm test
```

### With Browser UI (Headed Mode)

```bash
npm run test:headed
```

### Interactive UI Mode

```bash
npm run test:ui
```

### Debug Mode (Step Through Tests)

```bash
npm run test:debug
```

### View HTML Report

```bash
npm run report
```

## Test Structure

- `01-streams.spec.ts` - Stream management CRUD operations
- `02-manual-injection.spec.ts` - Manual phasor injection controls
- `03-comtrade.spec.ts` - COMTRADE file upload and playback
- `04-sequencer.spec.ts` - Sequence builder and state machine
- `05-analyzer.spec.ts` - Real-time waveform analyzer
- `06-navigation.spec.ts` - Navigation, error handling, responsive design

## Test Coverage

### Module 13: SV Stream Management ✅
- Create/Read/Update/Delete streams
- Start/Stop stream transmission
- Form validation
- Configuration updates

### Module 2: Manual Phasor Injection ✅
- Add streams to injection panel
- Start/Stop phasor injection
- Adjust frequency, magnitude, angle
- 120° angle linking
- Harmonics panel

### Module 14: COMTRADE Playback ✅
- Upload CFG/DAT files
- Parse COMTRADE format
- Channel mapping to streams
- Playback controls (start/pause/stop)
- Progress tracking

### Module 15: Sequence Builder ✅
- Create states with phasor configurations
- Define transitions between states
- Timer/trigger-based transitions
- Execute sequences
- Pause/Resume/Stop controls
- Save/Load sequences

### Module 16: Real-Time Analyzer ✅
- Select streams for analysis
- Start/Stop waveform capture
- Display time-domain plots
- Phasor table with magnitudes/angles
- FFT/Spectrum analysis
- Channel visibility toggles
- Time window adjustment
- Export waveform data

### Navigation & Error Handling ✅
- Global navigation between pages
- 404 error handling
- API error handling and retry logic
- Responsive design (mobile/tablet)
- Performance tests

## Running with Docker

The tests automatically start Docker Compose services via the `webServer` configuration in `playwright.config.ts`. To run manually:

```bash
# From project root
cd backend
docker compose --profile dev up -d

# Run tests
cd ../tests/e2e
npm test

# Cleanup
cd ../../backend
docker compose down -v
```

## CI/CD Integration

Tests run automatically in GitHub Actions via `.github/workflows/ci.yml`:

```yaml
e2e-tests:
  runs-on: ubuntu-latest
  steps:
    - Install Playwright + Chromium
    - Start Docker Compose services
    - Run: npx playwright test
    - Upload test results + screenshots on failure
```

## Configuration

Edit `playwright.config.ts` to adjust:

- **Base URL**: `http://localhost:5173`
- **Timeout**: 120 seconds for web server startup
- **Retries**: 2 in CI, 0 locally
- **Workers**: 1 in CI (sequential), parallel locally
- **Reporters**: HTML, JUnit, Console List
- **Capture**: Screenshots on failure, traces on first retry

## Troubleshooting

### Tests Fail to Start

**Issue**: "Error: page.goto: net::ERR_CONNECTION_REFUSED"

**Solution**: Ensure Docker Compose services are running:
```bash
cd backend
docker compose --profile dev up -d
```

### Port Conflicts

**Issue**: "Error: Port 5173 is already in use"

**Solution**: Stop conflicting services or change port in `playwright.config.ts`

### Type Errors in IDE

**Issue**: "Cannot find module '@playwright/test'"

**Solution**: Run `npm install` in `tests/e2e/` directory

### Slow Test Execution

**Issue**: Tests take too long

**Solution**: Run specific test file instead of full suite:
```bash
npx playwright test 01-streams.spec.ts
```

## Writing New Tests

1. Create a new `.spec.ts` file in `tests/e2e/tests/`
2. Import Playwright test utilities:
   ```typescript
   import { test, expect } from '@playwright/test';
   ```
3. Write test suites using `test.describe()` blocks
4. Add assertions using `expect()` API
5. Run tests: `npm test -- your-file.spec.ts`

## Best Practices

- **Use data-testid**: Add `data-testid` attributes to critical UI elements for stable selectors
- **Wait for visibility**: Use `await expect(element).toBeVisible()` instead of arbitrary timeouts
- **Clean up**: Tests create test streams - consider cleanup in `afterEach` hooks
- **Isolation**: Each test should be independent and idempotent
- **Parallelization**: Tests can run in parallel - avoid shared state

## Useful Commands

```bash
# Run specific test file
npm test -- 01-streams.spec.ts

# Run tests matching pattern
npm test -- --grep "should create"

# Run with browser visible
npm run test:headed

# Generate code from browser interactions
npx playwright codegen http://localhost:5173

# Show trace viewer for failed test
npx playwright show-trace trace.zip
```

## Results

Test results are saved in:
- `playwright-report/` - HTML report (open with `npm run report`)
- `test-results/` - Screenshots, traces, videos
- `test-results/junit.xml` - JUnit format for CI integration

## Resources

- [Playwright Documentation](https://playwright.dev)
- [Playwright Test API](https://playwright.dev/docs/api/class-test)
- [Best Practices](https://playwright.dev/docs/best-practices)
- [Debugging Tests](https://playwright.dev/docs/debug)
