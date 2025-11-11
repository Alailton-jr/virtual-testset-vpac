import { describe, it, expect, beforeEach, vi } from 'vitest';
import { render, screen, waitFor } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import DistanceTestPage from '@/pages/DistanceTestPage';

describe('DistanceTestPage', () => {
  beforeEach(() => {
    vi.clearAllMocks();
  });

  describe('Page Rendering', () => {
    it('should render the page title and description', () => {
      render(<DistanceTestPage />);
      
      expect(screen.getByText('Distance 21 Test')).toBeInTheDocument();
      expect(screen.getByText('Automated testing for distance protection relays')).toBeInTheDocument();
    });

    it('should render impedance configuration inputs', () => {
      render(<DistanceTestPage />);
      
      expect(screen.getByLabelText(/Resistance \(Ω\)/i)).toBeInTheDocument();
      expect(screen.getByLabelText(/Reactance \(Ω\)/i)).toBeInTheDocument();
    });

    it('should render Add Test Point button', () => {
      render(<DistanceTestPage />);
      
      const addButton = screen.getByRole('button', { name: /Add Test Point/i });
      expect(addButton).toBeInTheDocument();
    });

    it('should render Run Test button', () => {
      render(<DistanceTestPage />);
      
      const runButton = screen.getByRole('button', { name: /Run Test/i });
      expect(runButton).toBeInTheDocument();
    });
  });

  describe('Test Points Management', () => {
    it('should display default test points', () => {
      render(<DistanceTestPage />);
      
      expect(screen.getByText(/R=2Ω, X=4Ω/i)).toBeInTheDocument();
      expect(screen.getByText(/R=5Ω, X=10Ω/i)).toBeInTheDocument();
    });

    it('should show pending status for test points initially', () => {
      render(<DistanceTestPage />);
      
      const pendingBadges = screen.getAllByText('pending');
      expect(pendingBadges).toHaveLength(2);
    });

    it('should allow adding new test points', async () => {
      const user = userEvent.setup();
      render(<DistanceTestPage />);
      
      const rInput = screen.getByLabelText(/Resistance \(Ω\)/i);
      const xInput = screen.getByLabelText(/Reactance \(Ω\)/i);
      
      await user.clear(rInput);
      await user.type(rInput, '3');
      await user.clear(xInput);
      await user.type(xInput, '6');
      
      const addButton = screen.getByRole('button', { name: /Add Test Point/i });
      await user.click(addButton);
      
      await waitFor(() => {
        expect(screen.getByText(/R=3Ω, X=6Ω/i)).toBeInTheDocument();
      });
    });

    it('should reset input fields after adding test point', async () => {
      const user = userEvent.setup();
      render(<DistanceTestPage />);
      
      const rInput = screen.getByLabelText(/Resistance \(Ω\)/i) as HTMLInputElement;
      const xInput = screen.getByLabelText(/Reactance \(Ω\)/i) as HTMLInputElement;
      
      await user.clear(rInput);
      await user.type(rInput, '3');
      await user.clear(xInput);
      await user.type(xInput, '6');
      
      const addButton = screen.getByRole('button', { name: /Add Test Point/i });
      await user.click(addButton);
      
      await waitFor(() => {
        expect(rInput.value).toBe('0');
        expect(xInput.value).toBe('0');
      });
    });
  });

  describe('Input Configuration', () => {
    it('should have default values for impedance inputs', () => {
      render(<DistanceTestPage />);
      
      const rInput = screen.getByLabelText(/Resistance \(Ω\)/i) as HTMLInputElement;
      const xInput = screen.getByLabelText(/Reactance \(Ω\)/i) as HTMLInputElement;
      
      expect(rInput.value).toBe('0');
      expect(xInput.value).toBe('0');
    });

    it('should allow changing resistance value', async () => {
      const user = userEvent.setup();
      render(<DistanceTestPage />);
      
      const rInput = screen.getByLabelText(/Resistance \(Ω\)/i);
      await user.clear(rInput);
      await user.type(rInput, '7.5');
      
      expect(rInput).toHaveValue(7.5);
    });

    it('should allow changing reactance value', async () => {
      const user = userEvent.setup();
      render(<DistanceTestPage />);
      
      const xInput = screen.getByLabelText(/Reactance \(Ω\)/i);
      await user.clear(xInput);
      await user.type(xInput, '15.2');
      
      expect(xInput).toHaveValue(15.2);
    });

    it('should handle step increments for numeric inputs', () => {
      render(<DistanceTestPage />);
      
      const rInput = screen.getByLabelText(/Resistance \(Ω\)/i) as HTMLInputElement;
      const xInput = screen.getByLabelText(/Reactance \(Ω\)/i) as HTMLInputElement;
      
      expect(rInput.step).toBe('0.1');
      expect(xInput.step).toBe('0.1');
    });
  });

  describe('Test Execution', () => {
    it('should update test point statuses after running test', async () => {
      const user = userEvent.setup();
      render(<DistanceTestPage />);
      
      const runButton = screen.getByRole('button', { name: /Run Test/i });
      await user.click(runButton);
      
      await waitFor(() => {
        // Should no longer show all pending
        const results = screen.queryAllByText('pending');
        expect(results.length).toBeLessThan(2);
      });
    });

    it('should display pass or fail results', async () => {
      const user = userEvent.setup();
      render(<DistanceTestPage />);
      
      const runButton = screen.getByRole('button', { name: /Run Test/i });
      await user.click(runButton);
      
      await waitFor(() => {
        const passOrFail = screen.queryAllByText(/(pass|fail)/i);
        expect(passOrFail.length).toBeGreaterThan(0);
      });
    });
  });

  describe('UI Components', () => {
    it('should render test points configuration card', () => {
      render(<DistanceTestPage />);
      
      expect(screen.getByText('Test Points Configuration')).toBeInTheDocument();
      expect(screen.getByText(/Define R-X impedance points/i)).toBeInTheDocument();
    });

    it('should render R-X diagram placeholder', () => {
      render(<DistanceTestPage />);
      
      expect(screen.getByText('R-X Diagram')).toBeInTheDocument();
      expect(screen.getByText(/R-X diagram with zones coming soon/i)).toBeInTheDocument();
    });
  });

  describe('Validation', () => {
    it('should accept valid resistance values', async () => {
      const user = userEvent.setup();
      render(<DistanceTestPage />);
      
      const rInput = screen.getByLabelText(/Resistance \(Ω\)/i);
      await user.clear(rInput);
      await user.type(rInput, '10.5');
      
      expect(rInput).toHaveValue(10.5);
    });

    it('should accept valid reactance values', async () => {
      const user = userEvent.setup();
      render(<DistanceTestPage />);
      
      const xInput = screen.getByLabelText(/Reactance \(Ω\)/i);
      await user.clear(xInput);
      await user.type(xInput, '20.8');
      
      expect(xInput).toHaveValue(20.8);
    });

    it('should accept zero values', async () => {
      const user = userEvent.setup();
      render(<DistanceTestPage />);
      
      const rInput = screen.getByLabelText(/Resistance \(Ω\)/i);
      const xInput = screen.getByLabelText(/Reactance \(Ω\)/i);
      
      await user.clear(rInput);
      await user.type(rInput, '0');
      await user.clear(xInput);
      await user.type(xInput, '0');
      
      expect(rInput).toHaveValue(0);
      expect(xInput).toHaveValue(0);
    });
  });

  describe('Accessibility', () => {
    it('should have proper labels for all inputs', () => {
      render(<DistanceTestPage />);
      
      const rInput = screen.getByLabelText(/Resistance \(Ω\)/i);
      const xInput = screen.getByLabelText(/Reactance \(Ω\)/i);
      
      expect(rInput).toHaveAttribute('id');
      expect(xInput).toHaveAttribute('id');
    });

    it('should have accessible buttons', () => {
      render(<DistanceTestPage />);
      
      const addButton = screen.getByRole('button', { name: /Add Test Point/i });
      const runButton = screen.getByRole('button', { name: /Run Test/i });
      
      expect(addButton).toBeEnabled();
      expect(runButton).toBeEnabled();
    });
  });
});
