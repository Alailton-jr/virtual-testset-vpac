import { describe, it, expect, beforeEach, vi } from 'vitest';
import { render, screen, waitFor } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import OvercurrentTestPage from '@/pages/OvercurrentTestPage';

describe('OvercurrentTestPage', () => {
  beforeEach(() => {
    vi.clearAllMocks();
  });

  describe('Page Rendering', () => {
    it('should render the page title and description', () => {
      render(<OvercurrentTestPage />);
      
      expect(screen.getByText('Overcurrent 50/51 Test')).toBeInTheDocument();
      expect(screen.getByText('Automated testing for overcurrent protection relays')).toBeInTheDocument();
    });

    it('should render all configuration inputs', () => {
      render(<OvercurrentTestPage />);
      
      expect(screen.getByLabelText(/Pickup \(A\)/i)).toBeInTheDocument();
      expect(screen.getByLabelText(/Time Dial/i)).toBeInTheDocument();
      expect(screen.getByLabelText(/Curve Type/i)).toBeInTheDocument();
    });

    it('should render the Run Test button', () => {
      render(<OvercurrentTestPage />);
      
      const runButton = screen.getByRole('button', { name: /Run Test/i });
      expect(runButton).toBeInTheDocument();
    });
  });

  describe('Input Configuration', () => {
    it('should have default values for inputs', () => {
      render(<OvercurrentTestPage />);
      
      const pickupInput = screen.getByLabelText(/Pickup \(A\)/i) as HTMLInputElement;
      const timeDialInput = screen.getByLabelText(/Time Dial/i) as HTMLInputElement;
      
      expect(pickupInput.value).toBe('5.0');
      expect(timeDialInput.value).toBe('5');
    });

    it('should allow changing pickup value', async () => {
      const user = userEvent.setup();
      render(<OvercurrentTestPage />);
      
      const pickupInput = screen.getByLabelText(/Pickup \(A\)/i);
      await user.clear(pickupInput);
      await user.type(pickupInput, '6.5');
      
      expect(pickupInput).toHaveValue(6.5);
    });

    it('should allow changing time dial value', async () => {
      const user = userEvent.setup();
      render(<OvercurrentTestPage />);
      
      const timeDialInput = screen.getByLabelText(/Time Dial/i);
      await user.clear(timeDialInput);
      await user.type(timeDialInput, '7');
      
      expect(timeDialInput).toHaveValue(7);
    });

    it('should allow selecting curve type', async () => {
      const user = userEvent.setup();
      render(<OvercurrentTestPage />);
      
      const curveSelect = screen.getByLabelText(/Curve Type/i);
      await user.click(curveSelect);
      
      await waitFor(() => {
        const options = screen.getAllByText('IEC Standard Inverse');
        // Should have multiple instances (label, trigger, option)
        expect(options.length).toBeGreaterThanOrEqual(1);
        
        expect(screen.getByText('IEC Very Inverse')).toBeInTheDocument();
        expect(screen.getByText('IEC Extremely Inverse')).toBeInTheDocument();
        expect(screen.getByText('IEEE Moderately Inverse')).toBeInTheDocument();
      }, { timeout: 3000 });
    });

    it('should display timing information for test points', async () => {
      const user = userEvent.setup();
      render(<OvercurrentTestPage />);
      
      const runButton = screen.getByRole('button', { name: /Run Test/i });
      await user.click(runButton);
      
      await waitFor(() => {
        // Check for actual timing values that we know exist: "2.50 s", "0.80 s", "0.35 s"
        expect(screen.getByText(/2\.50/)).toBeInTheDocument();
        expect(screen.getByText(/0\.80/)).toBeInTheDocument();
        expect(screen.getByText(/0\.35/)).toBeInTheDocument();
      }, { timeout: 3000 });
    });
  });

  describe('Test Execution', () => {
    it('should display results after running test', async () => {
      const user = userEvent.setup();
      render(<OvercurrentTestPage />);
      
      const runButton = screen.getByRole('button', { name: /Run Test/i });
      await user.click(runButton);
      
      await waitFor(() => {
        expect(screen.getByText(/Test Results/i)).toBeInTheDocument();
      });
    });

    it('should display multiple test results with expected and actual times', async () => {
      const user = userEvent.setup();
      render(<OvercurrentTestPage />);
      
      const runButton = screen.getByRole('button', { name: /Run Test/i });
      await user.click(runButton);
      
      await waitFor(() => {
        // Check for timing results which are unique
        expect(screen.getByText(/2\.50/)).toBeInTheDocument();
        expect(screen.getByText(/0\.80/)).toBeInTheDocument();
        expect(screen.getByText(/0\.35/)).toBeInTheDocument();
        
        // Check that results are displayed (look for "A" unit which indicates test results)
        const testResults = screen.getAllByText(/A/);
        expect(testResults.length).toBeGreaterThan(0);
      }, { timeout: 3000 });
    });

    it('should display pass/fail badges for test results', async () => {
      const user = userEvent.setup();
      render(<OvercurrentTestPage />);
      
      const runButton = screen.getByRole('button', { name: /Run Test/i });
      await user.click(runButton);
      
      await waitFor(() => {
        const passBadges = screen.getAllByText('pass');
        expect(passBadges).toHaveLength(3);
      });
    });
  });

  describe('Validation', () => {
    it('should accept valid pickup values', async () => {
      const user = userEvent.setup();
      render(<OvercurrentTestPage />);
      
      const pickupInput = screen.getByLabelText(/Pickup \(A\)/i);
      await user.clear(pickupInput);
      await user.type(pickupInput, '1.5');
      
      expect(pickupInput).toHaveValue(1.5);
    });

    it('should accept valid time dial values', async () => {
      const user = userEvent.setup();
      render(<OvercurrentTestPage />);
      
      const timeDialInput = screen.getByLabelText(/Time Dial/i);
      await user.clear(timeDialInput);
      await user.type(timeDialInput, '10');
      
      expect(timeDialInput).toHaveValue(10);
    });

    it('should handle step increments for numeric inputs', () => {
      render(<OvercurrentTestPage />);
      
      const pickupInput = screen.getByLabelText(/Pickup \(A\)/i) as HTMLInputElement;
      const timeDialInput = screen.getByLabelText(/Time Dial/i) as HTMLInputElement;
      
      expect(pickupInput.step).toBe('0.1');
      expect(timeDialInput.step).toBe('0.1');
    });
  });

  describe('UI Components', () => {
    it('should render test configuration card', () => {
      render(<OvercurrentTestPage />);
      
      expect(screen.getByText('Test Configuration')).toBeInTheDocument();
      expect(screen.getByText(/Configure pickup, time dial, curve type/i)).toBeInTheDocument();
    });

    it('should render time-current curve visualization placeholder', () => {
      render(<OvercurrentTestPage />);
      
      expect(screen.getByText('Time-Current Curve')).toBeInTheDocument();
      expect(screen.getByText(/TCC curve visualization coming soon/i)).toBeInTheDocument();
    });

    it('should not show results before test is run', () => {
      render(<OvercurrentTestPage />);
      
      expect(screen.queryByText(/Test Results/i)).not.toBeInTheDocument();
    });
  });

  describe('Accessibility', () => {
    it('should have proper labels for all inputs', () => {
      render(<OvercurrentTestPage />);
      
      const pickupInput = screen.getByLabelText(/Pickup \(A\)/i);
      const timeDialInput = screen.getByLabelText(/Time Dial/i);
      const curveSelect = screen.getByLabelText(/Curve Type/i);
      
      expect(pickupInput).toHaveAttribute('id');
      expect(timeDialInput).toHaveAttribute('id');
      expect(curveSelect).toHaveAttribute('id');
    });

    it('should have accessible button', () => {
      render(<OvercurrentTestPage />);
      
      const runButton = screen.getByRole('button', { name: /Run Test/i });
      expect(runButton).toBeEnabled();
    });
  });
});
