import { test, expect } from '@playwright/test';

/**
 * E2E Test Suite: Real-Time Analyzer (Module 16)
 * Tests waveform capture and phasor analysis
 */

test.describe('Real-Time Analyzer', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/');
    await page.getByRole('link', { name: /analyzer/i }).click();
    await expect(page).toHaveURL(/.*analyzer/);
  });

  test('should display analyzer interface', async ({ page }) => {
    await expect(page.getByRole('heading', { name: /real.*time.*analyzer/i })).toBeVisible();
    await expect(page.getByRole('button', { name: /select.*stream/i })).toBeVisible();
    await expect(page.getByText(/no stream selected/i)).toBeVisible();
  });

  test('should select stream for analysis', async ({ page }) => {
    // Select stream
    await page.getByRole('button', { name: /select.*stream/i }).click();
    await page.getByRole('option').first().click();

    // Verify stream selected
    await expect(page.getByText(/no stream selected/i)).not.toBeVisible();
    await expect(page.getByRole('button', { name: /start capture/i })).toBeVisible();
  });

  test('should start waveform capture', async ({ page }) => {
    // Select stream
    await page.getByRole('button', { name: /select.*stream/i }).click();
    await page.getByRole('option').first().click();

    // Start capture
    await page.getByRole('button', { name: /start capture/i }).click();

    // Verify capture started
    await expect(page.getByRole('button', { name: /stop capture/i })).toBeVisible();
    await expect(page.getByText(/capturing/i)).toBeVisible();
  });

  test('should display waveform plot', async ({ page }) => {
    // Select and start
    await page.getByRole('button', { name: /select.*stream/i }).click();
    await page.getByRole('option').first().click();
    await page.getByRole('button', { name: /start capture/i }).click();

    // Wait for plot
    const plotCanvas = page.locator('canvas').first();
    await expect(plotCanvas).toBeVisible({ timeout: 5000 });
  });

  test('should display phasor table', async ({ page }) => {
    // Select and start
    await page.getByRole('button', { name: /select.*stream/i }).click();
    await page.getByRole('option').first().click();
    await page.getByRole('button', { name: /start capture/i }).click();

    // Check phasor table
    await expect(page.getByRole('table')).toBeVisible();
    await expect(page.getByText(/v-a|voltage|magnitude/i)).toBeVisible();
  });

  test('should display FFT analysis', async ({ page }) => {
    // Select and start
    await page.getByRole('button', { name: /select.*stream/i }).click();
    await page.getByRole('option').first().click();
    await page.getByRole('button', { name: /start capture/i }).click();

    // Switch to FFT tab
    await page.getByRole('tab', { name: /fft|spectrum|harmonics/i }).click();

    // Verify FFT plot
    const fftCanvas = page.locator('canvas[data-testid="fft-plot"]');
    await expect(fftCanvas).toBeVisible({ timeout: 5000 });
  });

  test('should toggle channels', async ({ page }) => {
    // Select and start
    await page.getByRole('button', { name: /select.*stream/i }).click();
    await page.getByRole('option').first().click();
    await page.getByRole('button', { name: /start capture/i }).click();

    // Find channel toggles
    const vaToggle = page.getByRole('checkbox', { name: /v-a/i });
    await expect(vaToggle).toBeVisible();

    // Toggle off
    if (await vaToggle.isChecked()) {
      await vaToggle.click();
      await expect(vaToggle).not.toBeChecked();
    }

    // Toggle on
    await vaToggle.click();
    await expect(vaToggle).toBeChecked();
  });

  test('should adjust time window', async ({ page }) => {
    // Select and start
    await page.getByRole('button', { name: /select.*stream/i }).click();
    await page.getByRole('option').first().click();
    await page.getByRole('button', { name: /start capture/i }).click();

    // Find time window control
    const timeWindow = page.getByLabel(/time.*window|window.*size/i);
    await expect(timeWindow).toBeVisible();

    // Change value
    await timeWindow.fill('2000');
    await expect(timeWindow).toHaveValue('2000');
  });

  test('should export waveform data', async ({ page }) => {
    // Select and start
    await page.getByRole('button', { name: /select.*stream/i }).click();
    await page.getByRole('option').first().click();
    await page.getByRole('button', { name: /start capture/i }).click();

    // Wait for data
    await page.waitForTimeout(1000);

    // Export
    const downloadPromise = page.waitForEvent('download');
    await page.getByRole('button', { name: /export|download/i }).click();
    const download = await downloadPromise;

    // Verify download
    expect(download.suggestedFilename()).toMatch(/\.csv|\.dat/i);
  });
});
