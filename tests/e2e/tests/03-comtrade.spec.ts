import { test, expect } from '@playwright/test';

/**
 * E2E Test Suite: COMTRADE Playback (Module 14)
 * Tests COMTRADE file upload and channel mapping
 */

test.describe('COMTRADE Playback', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/');
    await page.getByRole('link', { name: /comtrade/i }).click();
    await expect(page).toHaveURL(/.*comtrade/);
  });

  test('should display COMTRADE page', async ({ page }) => {
    await expect(page.getByRole('heading', { name: /comtrade.*playback/i })).toBeVisible();
    await expect(page.getByText(/upload.*cfg.*dat/i)).toBeVisible();
  });

  test('should upload COMTRADE files', async ({ page }) => {
    // Create mock CFG file content
    const cfgContent = `Test Station,Test Device,2011
3,2A,1D
1,V-A,,,V,1,1,0,65535,-65535,1.0,0.0,P
2,I-A,,,A,1,1,0,65535,-65535,1.0,0.0,P
1,Status,,,,0,0,0,0,0
60
4800
01/01/2011,10:00:00.000000
01/01/2011,10:00:01.000000
BINARY
1`;

    // Upload CFG file
    const cfgInput = page.locator('input[type="file"]').first();
    await cfgInput.setInputFiles({
      name: 'test.cfg',
      mimeType: 'text/plain',
      buffer: Buffer.from(cfgContent)
    });

    // Verify parsing
    await expect(page.getByText(/2.*analog.*channel/i)).toBeVisible();
  });

  test('should map channels to stream', async ({ page }) => {
    // Upload mock file (simplified)
    const cfgInput = page.locator('input[type="file"]').first();
    await cfgInput.setInputFiles({
      name: 'test.cfg',
      mimeType: 'text/plain',
      buffer: Buffer.from('Test,Test,2011\n3,2A,1D\n1,V-A,,,V,1,1,0,65535,-65535,1.0,0.0,P\n2,I-A,,,A,1,1,0,65535,-65535,1.0,0.0,P\n1,Status,,,,0,0,0,0,0\n60\n4800\n01/01/2011,10:00:00.000000\n01/01/2011,10:00:01.000000\nBINARY\n1')
    });

    // Select stream
    await page.getByRole('button', { name: /select stream/i }).click();
    await page.getByRole('option').first().click();

    // Map channels
    const channelRows = page.locator('[data-testid="channel-row"]');
    await expect(channelRows).toHaveCount(2);

    // Map V-A to V-A
    await channelRows.first().getByRole('combobox').click();
    await page.getByRole('option', { name: /v-a/i }).click();
  });

  test('should start playback', async ({ page }) => {
    // Upload and map (simplified)
    const cfgInput = page.locator('input[type="file"]').first();
    await cfgInput.setInputFiles({
      name: 'test.cfg',
      mimeType: 'text/plain',
      buffer: Buffer.from('Test,Test,2011\n3,2A,1D\n1,V-A,,,V,1,1,0,65535,-65535,1.0,0.0,P\n2,I-A,,,A,1,1,0,65535,-65535,1.0,0.0,P\n1,Status,,,,0,0,0,0,0\n60\n4800\n01/01/2011,10:00:00.000000\n01/01/2011,10:00:01.000000\nBINARY\n1')
    });

    // Start playback
    await page.getByRole('button', { name: /start.*playback/i }).click();
    
    // Verify playback controls
    await expect(page.getByRole('button', { name: /pause/i })).toBeVisible();
    await expect(page.getByRole('button', { name: /stop/i })).toBeVisible();
  });

  test('should display playback progress', async ({ page }) => {
    // Upload and start (simplified)
    const cfgInput = page.locator('input[type="file"]').first();
    await cfgInput.setInputFiles({
      name: 'test.cfg',
      mimeType: 'text/plain',
      buffer: Buffer.from('Test,Test,2011\n3,2A,1D\n1,V-A,,,V,1,1,0,65535,-65535,1.0,0.0,P\n2,I-A,,,A,1,1,0,65535,-65535,1.0,0.0,P\n1,Status,,,,0,0,0,0,0\n60\n4800\n01/01/2011,10:00:00.000000\n01/01/2011,10:00:01.000000\nBINARY\n1')
    });
    await page.getByRole('button', { name: /start.*playback/i }).click();

    // Check progress bar
    const progressBar = page.locator('[role="progressbar"]');
    await expect(progressBar).toBeVisible();
  });
});
