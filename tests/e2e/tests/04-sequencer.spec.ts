import { test, expect } from '@playwright/test';

/**
 * E2E Test Suite: Sequence Builder (Module 15)
 * Tests state machine builder and sequence execution
 */

test.describe('Sequence Builder', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/');
    await page.getByRole('link', { name: /sequence.*builder/i }).click();
    await expect(page).toHaveURL(/.*sequence/);
  });

  test('should display sequence builder interface', async ({ page }) => {
    await expect(page.getByRole('heading', { name: /sequence.*builder/i })).toBeVisible();
    await expect(page.getByRole('button', { name: /add state/i })).toBeVisible();
    await expect(page.getByRole('button', { name: /add transition/i })).toBeVisible();
  });

  test('should create states', async ({ page }) => {
    // Add Pre-Fault state
    await page.getByRole('button', { name: /add state/i }).click();
    await page.getByLabel(/state name/i).fill('Pre-Fault');
    await page.getByLabel(/duration/i).fill('1000');
    await page.getByRole('button', { name: /save|create/i }).click();

    // Verify state appears
    await expect(page.getByText('Pre-Fault')).toBeVisible();

    // Add Fault state
    await page.getByRole('button', { name: /add state/i }).click();
    await page.getByLabel(/state name/i).fill('Fault');
    await page.getByLabel(/duration/i).fill('500');
    await page.getByRole('button', { name: /save|create/i }).click();

    await expect(page.getByText('Fault')).toBeVisible();
  });

  test('should configure state phasors', async ({ page }) => {
    // Create state
    await page.getByRole('button', { name: /add state/i }).click();
    await page.getByLabel(/state name/i).fill('Config Test');
    await page.getByLabel(/duration/i).fill('1000');
    await page.getByRole('button', { name: /save|create/i }).click();

    // Edit state phasors
    const stateCard = page.locator('[data-testid="state-card"]').filter({ hasText: 'Config Test' });
    await stateCard.getByRole('button', { name: /configure/i }).click();

    // Set voltage magnitudes
    await page.getByLabel(/v-a.*magnitude/i).fill('63500');
    await page.getByLabel(/v-b.*magnitude/i).fill('63500');
    await page.getByLabel(/v-c.*magnitude/i).fill('63500');

    await page.getByRole('button', { name: /save/i }).click();
  });

  test('should create transitions', async ({ page }) => {
    // Create two states
    await page.getByRole('button', { name: /add state/i }).click();
    await page.getByLabel(/state name/i).fill('State A');
    await page.getByLabel(/duration/i).fill('1000');
    await page.getByRole('button', { name: /save|create/i }).click();

    await page.getByRole('button', { name: /add state/i }).click();
    await page.getByLabel(/state name/i).fill('State B');
    await page.getByLabel(/duration/i).fill('1000');
    await page.getByRole('button', { name: /save|create/i }).click();

    // Add transition
    await page.getByRole('button', { name: /add transition/i }).click();
    
    // Select from state
    await page.getByLabel(/from state/i).click();
    await page.getByRole('option', { name: /state a/i }).click();
    
    // Select to state
    await page.getByLabel(/to state/i).click();
    await page.getByRole('option', { name: /state b/i }).click();

    // Set trigger
    await page.getByLabel(/trigger type/i).click();
    await page.getByRole('option', { name: /timer/i }).click();

    await page.getByRole('button', { name: /save|create/i }).click();

    // Verify transition created
    await expect(page.getByText(/state a.*state b/i)).toBeVisible();
  });

  test('should execute sequence', async ({ page }) => {
    // Create simple sequence
    await page.getByRole('button', { name: /add state/i }).click();
    await page.getByLabel(/state name/i).fill('Exec Test');
    await page.getByLabel(/duration/i).fill('500');
    await page.getByRole('button', { name: /save|create/i }).click();

    // Start sequence
    await page.getByRole('button', { name: /start.*sequence/i }).click();

    // Verify execution
    await expect(page.getByText(/running|executing/i)).toBeVisible();
    await expect(page.getByRole('button', { name: /pause/i })).toBeVisible();
    await expect(page.getByRole('button', { name: /stop/i })).toBeVisible();
  });

  test('should pause and resume sequence', async ({ page }) => {
    // Create and start sequence
    await page.getByRole('button', { name: /add state/i }).click();
    await page.getByLabel(/state name/i).fill('Pause Test');
    await page.getByLabel(/duration/i).fill('5000'); // Long duration
    await page.getByRole('button', { name: /save|create/i }).click();

    await page.getByRole('button', { name: /start.*sequence/i }).click();

    // Pause
    await page.getByRole('button', { name: /pause/i }).click();
    await expect(page.getByText(/paused/i)).toBeVisible();

    // Resume
    await page.getByRole('button', { name: /resume/i }).click();
    await expect(page.getByText(/running|executing/i)).toBeVisible();
  });

  test('should save and load sequences', async ({ page }) => {
    // Create sequence
    await page.getByRole('button', { name: /add state/i }).click();
    await page.getByLabel(/state name/i).fill('Save Test');
    await page.getByLabel(/duration/i).fill('1000');
    await page.getByRole('button', { name: /save|create/i }).click();

    // Save sequence
    await page.getByRole('button', { name: /save sequence/i }).click();
    await page.getByLabel(/sequence name/i).fill('Test Sequence');
    await page.getByRole('button', { name: /save|confirm/i }).click();

    // Verify saved
    await expect(page.getByText('Test Sequence')).toBeVisible();
  });
});
