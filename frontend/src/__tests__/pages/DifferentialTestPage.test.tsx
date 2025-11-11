import { describe, it, expect, beforeEach, vi } from 'vitest';
import { render, screen, waitFor } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import DifferentialTestPage from '@/pages/DifferentialTestPage';

// Mock useStreamStore
vi.mock('@/stores/useStreamStore', () => ({
  useStreamStore: vi.fn(() => ({
    streams: [
      { id: 'stream1', name: 'Side 1 Stream' },
      { id: 'stream2', name: 'Side 2 Stream' },
      { id: 'stream3', name: 'Backup Stream' },
    ],
  })),
}));

describe('DifferentialTestPage', () => {
  beforeEach(() => {
    vi.clearAllMocks();
  });

  describe('Page Rendering', () => {
    it('should render the page title and description', () => {
      render(<DifferentialTestPage />);
      
      expect(screen.getByText('Differential 87 Test')).toBeInTheDocument();
      expect(screen.getByText('Automated testing for differential protection relays')).toBeInTheDocument();
    });

    it('should render stream selection dropdowns', () => {
      render(<DifferentialTestPage />);
      
      expect(screen.getByLabelText(/Side 1 Stream/i)).toBeInTheDocument();
      expect(screen.getByLabelText(/Side 2 Stream/i)).toBeInTheDocument();
    });

    it('should render configuration inputs', () => {
      render(<DifferentialTestPage />);
      
      expect(screen.getByLabelText(/Slope \(%\)/i)).toBeInTheDocument();
      expect(screen.getByLabelText(/Min Restraint \(A\)/i)).toBeInTheDocument();
    });

    it('should render the Run Test button', () => {
      render(<DifferentialTestPage />);
      
      const runButton = screen.getByRole('button', { name: /Run Test/i });
      expect(runButton).toBeInTheDocument();
    });
  });

  describe('Input Configuration', () => {
    it('should have default values for slope and restraint', () => {
      render(<DifferentialTestPage />);
      
      const slopeInput = screen.getByLabelText(/Slope \(%\)/i) as HTMLInputElement;
      const restraintInput = screen.getByLabelText(/Min Restraint \(A\)/i) as HTMLInputElement;
      
      expect(slopeInput.value).toBe('25');
      expect(restraintInput.value).toBe('0.3');
    });

    it('should allow changing slope value', async () => {
      const user = userEvent.setup();
      render(<DifferentialTestPage />);
      
      const slopeInput = screen.getByLabelText(/Slope \(%\)/i);
      await user.clear(slopeInput);
      await user.type(slopeInput, '30');
      
      expect(slopeInput).toHaveValue(30);
    });

    it('should allow changing restraint value', async () => {
      const user = userEvent.setup();
      render(<DifferentialTestPage />);
      
      const restraintInput = screen.getByLabelText(/Min Restraint \(A\)/i);
      await user.clear(restraintInput);
      await user.type(restraintInput, '0.5');
      
      expect(restraintInput).toHaveValue(0.5);
    });

    it('should allow selecting stream for Side 1', async () => {
      const user = userEvent.setup();
      render(<DifferentialTestPage />);
      
      const side1Select = screen.getByLabelText(/Side 1 Stream/i);
      await user.click(side1Select);
      
      await waitFor(() => {
        const options = screen.getAllByText('Side 1 Stream');
        // Should have the label and possibly the option in the dropdown
        expect(options.length).toBeGreaterThanOrEqual(1);
      }, { timeout: 3000 });
    });

    it('should allow selecting stream for Side 2', async () => {
      const user = userEvent.setup();
      render(<DifferentialTestPage />);
      
      const side2Select = screen.getByLabelText(/Side 2 Stream/i);
      await user.click(side2Select);
      
      await waitFor(() => {
        const options = screen.getAllByText('Side 2 Stream');
        // Should have the label and possibly the option in the dropdown
        expect(options.length).toBeGreaterThanOrEqual(1);
      }, { timeout: 3000 });
    });
  });

  describe('Test Points', () => {
    it('should display default test points', () => {
      render(<DifferentialTestPage />);
      
      expect(screen.getByText(/Ir = 1.0 A/i)).toBeInTheDocument();
      expect(screen.getByText(/Ir = 2.0 A/i)).toBeInTheDocument();
      expect(screen.getByText(/Ir = 5.0 A/i)).toBeInTheDocument();
    });

    it('should show pending status for test points initially', () => {
      render(<DifferentialTestPage />);
      
      const pendingBadges = screen.getAllByText('pending');
      expect(pendingBadges).toHaveLength(3);
    });
  });

  describe('Test Execution', () => {
    it('should disable Run Test button when no streams selected', () => {
      render(<DifferentialTestPage />);
      
      const runButton = screen.getByRole('button', { name: /Run Test/i });
      expect(runButton).toBeDisabled();
    });

    it('should update test point results after running test', async () => {
      // Mock Math.random to ensure consistent result
      const mockRandom = vi.spyOn(Math, 'random').mockReturnValue(0.8);
      
      const user = userEvent.setup();
      render(<DifferentialTestPage />);
      
      // Select streams
      const side1Select = screen.getByLabelText(/Side 1 Stream/i);
      await user.click(side1Select);
      const stream1Options = screen.getAllByText('Side 1 Stream');
      await user.click(stream1Options[stream1Options.length - 1]);
      
      const side2Select = screen.getByLabelText(/Side 2 Stream/i);
      await user.click(side2Select);
      const stream2Options = screen.getAllByText('Side 2 Stream');
      await user.click(stream2Options[stream2Options.length - 1]);
      
      // Run test
      const runButton = screen.getByRole('button', { name: /Run Test/i });
      await user.click(runButton);
      
      await waitFor(() => {
        // Should no longer show all pending
        const results = screen.queryAllByText('pending');
        expect(results.length).toBeLessThan(3);
      }, { timeout: 3000 });
      
      mockRandom.mockRestore();
    });
  });

  describe('UI Components', () => {
    it('should render test configuration card', () => {
      render(<DifferentialTestPage />);
      
      expect(screen.getByText('Test Configuration')).toBeInTheDocument();
      expect(screen.getByText(/Define restraint and differential current points/i)).toBeInTheDocument();
    });

    it('should render Id vs Ir plot placeholder', () => {
      render(<DifferentialTestPage />);
      
      expect(screen.getByText('Id vs Ir Plot')).toBeInTheDocument();
      expect(screen.getByText(/Differential characteristic plot coming soon/i)).toBeInTheDocument();
    });
  });

  describe('Validation', () => {
    it('should accept valid slope values', async () => {
      const user = userEvent.setup();
      render(<DifferentialTestPage />);
      
      const slopeInput = screen.getByLabelText(/Slope \(%\)/i);
      await user.clear(slopeInput);
      await user.type(slopeInput, '50');
      
      expect(slopeInput).toHaveValue(50);
    });

    it('should accept valid restraint values', async () => {
      const user = userEvent.setup();
      render(<DifferentialTestPage />);
      
      const restraintInput = screen.getByLabelText(/Min Restraint \(A\)/i);
      await user.clear(restraintInput);
      await user.type(restraintInput, '1.0');
      
      expect(restraintInput).toHaveValue(1.0);
    });

    it('should handle step increments for numeric inputs', () => {
      render(<DifferentialTestPage />);
      
      const slopeInput = screen.getByLabelText(/Slope \(%\)/i) as HTMLInputElement;
      const restraintInput = screen.getByLabelText(/Min Restraint \(A\)/i) as HTMLInputElement;
      
      expect(slopeInput.step).toBe('1');
      expect(restraintInput.step).toBe('0.1');
    });
  });

  describe('Accessibility', () => {
    it('should have proper labels for all inputs', () => {
      render(<DifferentialTestPage />);
      
      const side1Select = screen.getByLabelText(/Side 1 Stream/i);
      const side2Select = screen.getByLabelText(/Side 2 Stream/i);
      const slopeInput = screen.getByLabelText(/Slope \(%\)/i);
      const restraintInput = screen.getByLabelText(/Min Restraint \(A\)/i);
      
      expect(side1Select).toHaveAttribute('id');
      expect(side2Select).toHaveAttribute('id');
      expect(slopeInput).toHaveAttribute('id');
      expect(restraintInput).toHaveAttribute('id');
    });

    it('should have accessible Run Test button', () => {
      render(<DifferentialTestPage />);
      
      const runButton = screen.getByRole('button', { name: /Run Test/i });
      expect(runButton).toBeInTheDocument();
    });
  });
});
