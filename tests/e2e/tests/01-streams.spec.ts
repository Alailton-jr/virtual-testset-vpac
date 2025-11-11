import { test, expect } from '@playwright/test';

/**
 * E2E Test Suite: Stream Management (Module 13)
 * Tests the complete CRUD flow for SV streams
 */

test.describe('Stream Management', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/');
    await page.getByRole('link', { name: /streams/i }).click();
    await expect(page).toHaveURL(/.*streams/);
  });

  test('should display streams page with table', async ({ page }) => {
    await expect(page.getByRole('heading', { name: /sv streams/i })).toBeVisible();
    await expect(page.getByRole('button', { name: /add stream/i })).toBeVisible();
  });

  test('should create a new stream', async ({ page }) => {
    // Click Add Stream button
    await page.getByRole('button', { name: /add stream/i }).click();
    
    // Fill form
    await page.getByLabel(/name/i).fill('E2E Test Stream');
    await page.getByLabel(/sv id/i).fill('SV_E2E');
    await page.getByLabel(/app id/i).fill('0x4000');
    await page.getByLabel(/mac.*destination/i).fill('01:0C:CD:04:00:01');
    await page.getByLabel(/vlan id/i).fill('100');
    await page.getByLabel(/sample rate/i).fill('4800');
    
    // Submit form
    await page.getByRole('button', { name: /create|save/i }).click();
    
    // Verify stream appears in table
    await expect(page.getByText('E2E Test Stream')).toBeVisible();
    await expect(page.getByText('SV_E2E')).toBeVisible();
  });

  test('should start and stop a stream', async ({ page }) => {
    // Create stream first
    await page.getByRole('button', { name: /add stream/i }).click();
    await page.getByLabel(/name/i).fill('Start Test Stream');
    await page.getByLabel(/sv id/i).fill('SV_START');
    await page.getByLabel(/app id/i).fill('0x4001');
    await page.getByLabel(/mac.*destination/i).fill('01:0C:CD:04:00:02');
    await page.getByLabel(/vlan id/i).fill('100');
    await page.getByLabel(/sample rate/i).fill('4800');
    await page.getByRole('button', { name: /create|save/i }).click();
    
    // Find and click Start button
    const streamRow = page.getByRole('row').filter({ hasText: 'Start Test Stream' });
    await streamRow.getByRole('button', { name: /start/i }).click();
    
    // Wait for status change
    await expect(streamRow.getByText(/running|active/i)).toBeVisible({ timeout: 5000 });
    
    // Click Stop button
    await streamRow.getByRole('button', { name: /stop/i }).click();
    
    // Verify stopped
    await expect(streamRow.getByText(/stopped|inactive/i)).toBeVisible({ timeout: 5000 });
  });

  test('should edit stream configuration', async ({ page }) => {
    // Create stream
    await page.getByRole('button', { name: /add stream/i }).click();
    await page.getByLabel(/name/i).fill('Edit Test Stream');
    await page.getByLabel(/sv id/i).fill('SV_EDIT');
    await page.getByLabel(/app id/i).fill('0x4002');
    await page.getByLabel(/mac.*destination/i).fill('01:0C:CD:04:00:03');
    await page.getByLabel(/vlan id/i).fill('100');
    await page.getByLabel(/sample rate/i).fill('4800');
    await page.getByRole('button', { name: /create|save/i }).click();
    
    // Edit stream
    const streamRow = page.getByRole('row').filter({ hasText: 'Edit Test Stream' });
    await streamRow.getByRole('button', { name: /edit/i }).click();
    
    // Change name
    await page.getByLabel(/name/i).clear();
    await page.getByLabel(/name/i).fill('Edit Test Stream Updated');
    await page.getByRole('button', { name: /save|update/i }).click();
    
    // Verify update
    await expect(page.getByText('Edit Test Stream Updated')).toBeVisible();
  });

  test('should delete a stream', async ({ page }) => {
    // Create stream
    await page.getByRole('button', { name: /add stream/i }).click();
    await page.getByLabel(/name/i).fill('Delete Test Stream');
    await page.getByLabel(/sv id/i).fill('SV_DELETE');
    await page.getByLabel(/app id/i).fill('0x4003');
    await page.getByLabel(/mac.*destination/i).fill('01:0C:CD:04:00:04');
    await page.getByLabel(/vlan id/i).fill('100');
    await page.getByLabel(/sample rate/i).fill('4800');
    await page.getByRole('button', { name: /create|save/i }).click();
    
    // Delete stream
    const streamRow = page.getByRole('row').filter({ hasText: 'Delete Test Stream' });
    await streamRow.getByRole('button', { name: /delete/i }).click();
    
    // Confirm deletion
    await page.getByRole('button', { name: /confirm|yes|delete/i }).click();
    
    // Verify removed
    await expect(page.getByText('Delete Test Stream')).not.toBeVisible();
  });

  test('should validate form inputs', async ({ page }) => {
    await page.getByRole('button', { name: /add stream/i }).click();
    
    // Try to submit empty form
    await page.getByRole('button', { name: /create|save/i }).click();
    
    // Check for validation errors
    await expect(page.getByText(/required|invalid/i).first()).toBeVisible();
    
    // Test invalid MAC address
    await page.getByLabel(/mac.*destination/i).fill('invalid-mac');
    await page.getByRole('button', { name: /create|save/i }).click();
    await expect(page.getByText(/invalid.*mac|format/i)).toBeVisible();
    
    // Test invalid App ID
    await page.getByLabel(/app id/i).fill('invalid');
    await page.getByRole('button', { name: /create|save/i }).click();
    await expect(page.getByText(/invalid.*hex|format/i)).toBeVisible();
  });
});
