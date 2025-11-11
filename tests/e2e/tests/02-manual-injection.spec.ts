import { test, expect } from '@playwright/test';

/**
 * E2E Test Suite: Manual Phasor Injection (Module 2)
 * Tests manual control of phasors for SV streams
 */

test.describe('Manual Phasor Injection', () => {
  test.beforeEach(async ({ page }) => {
    // Create a test stream first
    await page.goto('/streams');
    await page.getByRole('button', { name: /add stream/i }).click();
    await page.getByLabel(/name/i).fill('Phasor Test Stream');
    await page.getByLabel(/sv id/i).fill('SV_PHASOR');
    await page.getByLabel(/app id/i).fill('0x5000');
    await page.getByLabel(/mac.*destination/i).fill('01:0C:CD:05:00:00');
    await page.getByLabel(/vlan id/i).fill('100');
    await page.getByLabel(/sample rate/i).fill('4800');
    await page.getByRole('button', { name: /create|save/i }).click();
    
    // Navigate to Manual Injection page
    await page.getByRole('link', { name: /manual.*injection/i }).click();
    await expect(page).toHaveURL(/.*manual/);
  });

  test('should add stream to injection panel', async ({ page }) => {
    await page.getByRole('button', { name: /add stream/i }).click();
    await page.getByRole('option', { name: /phasor test stream/i }).click();
    
    await expect(page.getByText('Phasor Test Stream')).toBeVisible();
  });

  test('should start and stop phasor injection', async ({ page }) => {
    // Add stream
    await page.getByRole('button', { name: /add stream/i }).click();
    await page.getByRole('option', { name: /phasor test stream/i }).click();
    
    // Start injection
    const streamCard = page.locator('[data-testid="stream-card"]').filter({ hasText: 'Phasor Test Stream' });
    await streamCard.getByRole('button', { name: /start/i }).click();
    
    await expect(streamCard.getByText(/running|active/i)).toBeVisible();
    
    // Stop injection
    await streamCard.getByRole('button', { name: /stop/i }).click();
    await expect(streamCard.getByText(/stopped|inactive/i)).toBeVisible();
  });

  test('should adjust frequency slider', async ({ page }) => {
    // Add and start stream
    await page.getByRole('button', { name: /add stream/i }).click();
    await page.getByRole('option', { name: /phasor test stream/i }).click();
    const streamCard = page.locator('[data-testid="stream-card"]').filter({ hasText: 'Phasor Test Stream' });
    await streamCard.getByRole('button', { name: /start/i }).click();
    
    // Find frequency slider
    const freqSlider = streamCard.getByLabel(/frequency/i);
    await expect(freqSlider).toBeVisible();
    
    // Change frequency
    await freqSlider.fill('59.5');
    await expect(freqSlider).toHaveValue('59.5');
  });

  test('should adjust phasor magnitude', async ({ page }) => {
    // Add and start stream
    await page.getByRole('button', { name: /add stream/i }).click();
    await page.getByRole('option', { name: /phasor test stream/i }).click();
    const streamCard = page.locator('[data-testid="stream-card"]').filter({ hasText: 'Phasor Test Stream' });
    await streamCard.getByRole('button', { name: /start/i }).click();
    
    // Adjust V-A magnitude
    const vaMagSlider = streamCard.getByLabel(/v-a.*magnitude/i);
    await vaMagSlider.fill('65000');
    await expect(vaMagSlider).toHaveValue('65000');
  });

  test('should link 120° angles', async ({ page }) => {
    // Add and start stream
    await page.getByRole('button', { name: /add stream/i }).click();
    await page.getByRole('option', { name: /phasor test stream/i }).click();
    const streamCard = page.locator('[data-testid="stream-card"]').filter({ hasText: 'Phasor Test Stream' });
    await streamCard.getByRole('button', { name: /start/i }).click();
    
    // Set V-A angle
    const vaAngle = streamCard.getByLabel(/v-a.*angle/i);
    await vaAngle.fill('-5');
    
    // Click 120° link
    await streamCard.getByRole('button', { name: /link.*120/i }).click();
    
    // Verify V-B and V-C angles
    const vbAngle = streamCard.getByLabel(/v-b.*angle/i);
    const vcAngle = streamCard.getByLabel(/v-c.*angle/i);
    
    await expect(vbAngle).toHaveValue('-125'); // -5 - 120
    await expect(vcAngle).toHaveValue('115');  // -5 + 120
  });

  test('should display harmonics panel', async ({ page }) => {
    // Add and start stream
    await page.getByRole('button', { name: /add stream/i }).click();
    await page.getByRole('option', { name: /phasor test stream/i }).click();
    const streamCard = page.locator('[data-testid="stream-card"]').filter({ hasText: 'Phasor Test Stream' });
    await streamCard.getByRole('button', { name: /start/i }).click();
    
    // Expand harmonics
    await streamCard.getByRole('button', { name: /harmonics/i }).click();
    
    // Verify harmonics controls visible
    await expect(streamCard.getByText(/harmonic.*order|h3|h5/i)).toBeVisible();
  });
});
