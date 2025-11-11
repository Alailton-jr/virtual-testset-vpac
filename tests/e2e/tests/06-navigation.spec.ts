import { test, expect } from '@playwright/test';

/**
 * E2E Test Suite: Navigation and Layout
 * Tests global navigation, responsive design, and error handling
 */

test.describe('Navigation', () => {
  test('should load home page', async ({ page }) => {
    await page.goto('/');
    await expect(page).toHaveTitle(/virtual.*testset/i);
    await expect(page.getByRole('heading', { name: /virtual.*testset/i })).toBeVisible();
  });

  test('should navigate to all main pages', async ({ page }) => {
    await page.goto('/');

    // Navigate to Streams
    await page.getByRole('link', { name: /streams/i }).click();
    await expect(page).toHaveURL(/.*streams/);
    await expect(page.getByRole('heading', { name: /sv streams/i })).toBeVisible();

    // Navigate to Manual Injection
    await page.getByRole('link', { name: /manual.*injection/i }).click();
    await expect(page).toHaveURL(/.*manual/);

    // Navigate to COMTRADE
    await page.getByRole('link', { name: /comtrade/i }).click();
    await expect(page).toHaveURL(/.*comtrade/);

    // Navigate to Sequence Builder
    await page.getByRole('link', { name: /sequence/i }).click();
    await expect(page).toHaveURL(/.*sequence/);

    // Navigate to Analyzer
    await page.getByRole('link', { name: /analyzer/i }).click();
    await expect(page).toHaveURL(/.*analyzer/);

    // Navigate back to Home
    await page.getByRole('link', { name: /home/i }).click();
    await expect(page).toHaveURL('/');
  });

  test('should display navigation menu', async ({ page }) => {
    await page.goto('/');
    
    const nav = page.getByRole('navigation');
    await expect(nav).toBeVisible();

    // Check all menu items present
    await expect(nav.getByRole('link', { name: /home/i })).toBeVisible();
    await expect(nav.getByRole('link', { name: /streams/i })).toBeVisible();
    await expect(nav.getByRole('link', { name: /manual/i })).toBeVisible();
    await expect(nav.getByRole('link', { name: /comtrade/i })).toBeVisible();
    await expect(nav.getByRole('link', { name: /sequence/i })).toBeVisible();
    await expect(nav.getByRole('link', { name: /analyzer/i })).toBeVisible();
  });

  test('should handle 404 page', async ({ page }) => {
    await page.goto('/nonexistent-page');
    await expect(page.getByText(/not found|404/i)).toBeVisible();
  });
});

test.describe('API Error Handling', () => {
  test('should display error on API failure', async ({ page }) => {
    // Mock API failure
    await page.route('**/api/v1/streams', route => {
      route.fulfill({ status: 500, body: 'Internal Server Error' });
    });

    await page.goto('/streams');
    
    // Should show error message
    await expect(page.getByText(/error|failed|unable/i)).toBeVisible({ timeout: 5000 });
  });

  test('should retry failed requests', async ({ page }) => {
    let requestCount = 0;
    await page.route('**/api/v1/streams', route => {
      requestCount++;
      if (requestCount < 2) {
        route.fulfill({ status: 500 });
      } else {
        route.fulfill({ status: 200, body: JSON.stringify([]) });
      }
    });

    await page.goto('/streams');
    
    // Should eventually succeed
    await expect(page.getByRole('table')).toBeVisible({ timeout: 10000 });
    expect(requestCount).toBeGreaterThanOrEqual(2);
  });
});

test.describe('Responsive Design', () => {
  test('should work on mobile viewport', async ({ page }) => {
    await page.setViewportSize({ width: 375, height: 667 });
    await page.goto('/');

    // Check mobile menu
    const menuButton = page.getByRole('button', { name: /menu/i });
    if (await menuButton.isVisible()) {
      await menuButton.click();
      await expect(page.getByRole('navigation')).toBeVisible();
    }
  });

  test('should work on tablet viewport', async ({ page }) => {
    await page.setViewportSize({ width: 768, height: 1024 });
    await page.goto('/streams');

    // Table should be visible and responsive
    await expect(page.getByRole('table')).toBeVisible();
  });
});

test.describe('Performance', () => {
  test('should load home page quickly', async ({ page }) => {
    const startTime = Date.now();
    await page.goto('/');
    await page.waitForLoadState('networkidle');
    const loadTime = Date.now() - startTime;

    expect(loadTime).toBeLessThan(5000); // 5 seconds max
  });

  test('should handle multiple streams', async ({ page }) => {
    await page.goto('/streams');

    // Create 5 streams
    for (let i = 0; i < 5; i++) {
      await page.getByRole('button', { name: /add stream/i }).click();
      await page.getByLabel(/name/i).fill(`Perf Test ${i}`);
      await page.getByLabel(/sv id/i).fill(`SV_${i}`);
      await page.getByLabel(/app id/i).fill(`0x${(6000 + i).toString(16)}`);
      await page.getByLabel(/mac.*destination/i).fill(`01:0C:CD:06:00:0${i}`);
      await page.getByLabel(/vlan id/i).fill('100');
      await page.getByLabel(/sample rate/i).fill('4800');
      await page.getByRole('button', { name: /create|save/i }).click();
      await page.waitForTimeout(200);
    }

    // Verify all visible
    const rows = page.getByRole('row').filter({ hasText: /perf test/i });
    await expect(rows).toHaveCount(5);
  });
});
