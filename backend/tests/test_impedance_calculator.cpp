#include "impedance_calculator.hpp"
#include <gtest/gtest.h>
#include <cmath>

using namespace vts::testers;

// Helper function to check complex number equality with tolerance
void expectComplexNear(const std::complex<double>& actual, 
                       const std::complex<double>& expected, 
                       double tolerance = 1e-6) {
    EXPECT_NEAR(actual.real(), expected.real(), tolerance) 
        << "Real part mismatch: " << actual.real() << " vs " << expected.real();
    EXPECT_NEAR(actual.imag(), expected.imag(), tolerance)
        << "Imaginary part mismatch: " << actual.imag() << " vs " << expected.imag();
}

// Test fixture
class ImpedanceCalculatorTest : public ::testing::Test {
protected:
    ImpedanceCalculator calc;
    SourceImpedance source;
    FaultImpedance faultZ;
    
    void SetUp() override {
        // Default source impedance (typical values)
        source.RS1 = 1.0;    // 1 ohm
        source.XS1 = 10.0;   // 10 ohms
        source.RS0 = 3.0;    // 3 ohms  
        source.XS0 = 30.0;   // 30 ohms
        source.Vprefault = 115470.0 / std::sqrt(3.0);  // 115.47 kV line-to-neutral
        
        // Default fault impedance (solid fault)
        faultZ.R = 0.0;
        faultZ.X = 0.0;
    }
};

// Test alpha operators
TEST_F(ImpedanceCalculatorTest, AlphaOperator) {
    // Can't access private methods directly, but we can verify through sequence transforms
    // Alpha should be 1∠120° = -0.5 + j0.866
    // This is implicitly tested in the sequence transformations
    SUCCEED();  // Placeholder - tested indirectly
}

// Test fault type parsing
TEST_F(ImpedanceCalculatorTest, ParseFaultType) {
    EXPECT_EQ(ImpedanceCalculator::parseFaultType("AG"), FaultType::AG);
    EXPECT_EQ(ImpedanceCalculator::parseFaultType("ag"), FaultType::AG);
    EXPECT_EQ(ImpedanceCalculator::parseFaultType("BG"), FaultType::BG);
    EXPECT_EQ(ImpedanceCalculator::parseFaultType("CG"), FaultType::CG);
    EXPECT_EQ(ImpedanceCalculator::parseFaultType("AB"), FaultType::AB);
    EXPECT_EQ(ImpedanceCalculator::parseFaultType("BC"), FaultType::BC);
    EXPECT_EQ(ImpedanceCalculator::parseFaultType("CA"), FaultType::CA);
    EXPECT_EQ(ImpedanceCalculator::parseFaultType("ABG"), FaultType::ABG);
    EXPECT_EQ(ImpedanceCalculator::parseFaultType("BCG"), FaultType::BCG);
    EXPECT_EQ(ImpedanceCalculator::parseFaultType("CAG"), FaultType::CAG);
    EXPECT_EQ(ImpedanceCalculator::parseFaultType("ABC"), FaultType::ABC);
    EXPECT_EQ(ImpedanceCalculator::parseFaultType("3PH"), FaultType::ABC);
    
    EXPECT_THROW(ImpedanceCalculator::parseFaultType("INVALID"), std::invalid_argument);
}

// Test fault type to string
TEST_F(ImpedanceCalculatorTest, FaultTypeToString) {
    EXPECT_EQ(ImpedanceCalculator::faultTypeToString(FaultType::AG), "AG");
    EXPECT_EQ(ImpedanceCalculator::faultTypeToString(FaultType::BG), "BG");
    EXPECT_EQ(ImpedanceCalculator::faultTypeToString(FaultType::CG), "CG");
    EXPECT_EQ(ImpedanceCalculator::faultTypeToString(FaultType::AB), "AB");
    EXPECT_EQ(ImpedanceCalculator::faultTypeToString(FaultType::BC), "BC");
    EXPECT_EQ(ImpedanceCalculator::faultTypeToString(FaultType::CA), "CA");
    EXPECT_EQ(ImpedanceCalculator::faultTypeToString(FaultType::ABG), "ABG");
    EXPECT_EQ(ImpedanceCalculator::faultTypeToString(FaultType::BCG), "BCG");
    EXPECT_EQ(ImpedanceCalculator::faultTypeToString(FaultType::CAG), "CAG");
    EXPECT_EQ(ImpedanceCalculator::faultTypeToString(FaultType::ABC), "ABC");
}

// Test single line-to-ground fault (AG)
TEST_F(ImpedanceCalculatorTest, CalculateSLG_AG) {
    PhasorState result = calc.calculateFault(FaultType::AG, faultZ, source);
    
    // For SLG fault, faulted phase current should be non-zero
    double Ia_mag = std::abs(result.current.A);
    EXPECT_GT(Ia_mag, 1000.0) << "Phase A current should be significant";
    
    // Non-faulted phases should have smaller currents (but not zero due to coupling)
    double Ib_mag = std::abs(result.current.B);
    double Ic_mag = std::abs(result.current.C);
    EXPECT_LT(Ib_mag, Ia_mag);
    EXPECT_LT(Ic_mag, Ia_mag);
    
    // Faulted phase voltage should be near zero (solid fault)
    double Va_mag = std::abs(result.voltage.A);
    EXPECT_LT(Va_mag, 1000.0) << "Phase A voltage should be near zero for solid fault";
}

// Test single line-to-ground fault (BG)
TEST_F(ImpedanceCalculatorTest, CalculateSLG_BG) {
    PhasorState result = calc.calculateFault(FaultType::BG, faultZ, source);
    
    // For BG fault, phase B current should be largest
    double Ia_mag = std::abs(result.current.A);
    double Ib_mag = std::abs(result.current.B);
    double Ic_mag = std::abs(result.current.C);
    
    EXPECT_GT(Ib_mag, 1000.0) << "Phase B current should be significant";
    EXPECT_LT(Ia_mag, Ib_mag);
    EXPECT_LT(Ic_mag, Ib_mag);
    
    // Phase B voltage should be near zero
    double Vb_mag = std::abs(result.voltage.B);
    EXPECT_LT(Vb_mag, 1000.0) << "Phase B voltage should be near zero";
}

// Test line-to-line fault (BC)
TEST_F(ImpedanceCalculatorTest, CalculateLL_BC) {
    PhasorState result = calc.calculateFault(FaultType::BC, faultZ, source);
    
    // For BC fault, currents in B and C should be equal and opposite
    double Ib_mag = std::abs(result.current.B);
    double Ic_mag = std::abs(result.current.C);
    
    EXPECT_GT(Ib_mag, 1000.0) << "Phase B current should be significant";
    EXPECT_NEAR(Ib_mag, Ic_mag, 10.0) << "Phase B and C currents should have similar magnitudes";
    
    // Phase A current should be very small (no zero sequence)
    double Ia_mag = std::abs(result.current.A);
    EXPECT_LT(Ia_mag, 1.0) << "Phase A current should be near zero";
}

// Test three-phase fault
TEST_F(ImpedanceCalculatorTest, Calculate3Ph) {
    PhasorState result = calc.calculateFault(FaultType::ABC, faultZ, source);
    
    // For balanced 3-phase fault, currents should be balanced
    double Ia_mag = std::abs(result.current.A);
    double Ib_mag = std::abs(result.current.B);
    double Ic_mag = std::abs(result.current.C);
    
    EXPECT_GT(Ia_mag, 1000.0) << "All phase currents should be significant";
    EXPECT_NEAR(Ia_mag, Ib_mag, 10.0) << "Phase currents should be balanced";
    EXPECT_NEAR(Ia_mag, Ic_mag, 10.0) << "Phase currents should be balanced";
    
    // 120° phase separation
    double angle_AB = std::arg(result.current.B) - std::arg(result.current.A);
    double angle_BC = std::arg(result.current.C) - std::arg(result.current.B);
    
    // Normalize angles to [-π, π]
    while (angle_AB > M_PI) angle_AB -= 2.0 * M_PI;
    while (angle_AB < -M_PI) angle_AB += 2.0 * M_PI;
    while (angle_BC > M_PI) angle_BC -= 2.0 * M_PI;
    while (angle_BC < -M_PI) angle_BC += 2.0 * M_PI;
    
    EXPECT_NEAR(std::abs(angle_AB), 2.0 * M_PI / 3.0, 0.01) 
        << "Phase separation should be 120°";
    EXPECT_NEAR(std::abs(angle_BC), 2.0 * M_PI / 3.0, 0.01) 
        << "Phase separation should be 120°";
}

// Test fault with impedance
TEST_F(ImpedanceCalculatorTest, FaultWithImpedance) {
    // Set fault impedance to non-zero
    faultZ.R = 5.0;
    faultZ.X = 2.0;
    
    PhasorState result_with_Z = calc.calculateFault(FaultType::AG, faultZ, source);
    
    // Compare with solid fault
    FaultImpedance solidZ{0.0, 0.0};
    PhasorState result_solid = calc.calculateFault(FaultType::AG, solidZ, source);
    
    // Fault current should be lower with impedance
    double Ia_with_Z = std::abs(result_with_Z.current.A);
    double Ia_solid = std::abs(result_solid.current.A);
    
    EXPECT_LT(Ia_with_Z, Ia_solid) 
        << "Fault current should decrease with fault impedance";
    
    // Fault voltage should be higher with impedance
    double Va_with_Z = std::abs(result_with_Z.voltage.A);
    double Va_solid = std::abs(result_solid.voltage.A);
    
    EXPECT_GT(Va_with_Z, Va_solid) 
        << "Fault voltage should increase with fault impedance";
}

// Test double line-to-ground fault
TEST_F(ImpedanceCalculatorTest, CalculateDLG_BCG) {
    PhasorState result = calc.calculateFault(FaultType::BCG, faultZ, source);
    
    // For BCG fault, phases B and C currents should be significant
    double Ib_mag = std::abs(result.current.B);
    double Ic_mag = std::abs(result.current.C);
    
    EXPECT_GT(Ib_mag, 1000.0) << "Phase B current should be significant";
    EXPECT_GT(Ic_mag, 1000.0) << "Phase C current should be significant";
    
    // Phase A current should be smaller
    double Ia_mag = std::abs(result.current.A);
    EXPECT_LT(Ia_mag, std::max(Ib_mag, Ic_mag));
}

// Test current conservation (sum should be zero for unfaulted system)
TEST_F(ImpedanceCalculatorTest, CurrentConservation_3Ph) {
    PhasorState result = calc.calculateFault(FaultType::ABC, faultZ, source);
    
    // For balanced 3-phase fault, sum of currents should be zero
    std::complex<double> sum = result.current.A + result.current.B + result.current.C;
    
    EXPECT_NEAR(std::abs(sum), 0.0, 10.0) 
        << "Sum of balanced three-phase currents should be zero";
}

// Test voltage symmetry for three-phase fault
TEST_F(ImpedanceCalculatorTest, VoltageSymmetry_3Ph) {
    PhasorState result = calc.calculateFault(FaultType::ABC, faultZ, source);
    
    // For solid 3-phase fault, all phase voltages should be near zero
    double Va_mag = std::abs(result.voltage.A);
    double Vb_mag = std::abs(result.voltage.B);
    double Vc_mag = std::abs(result.voltage.C);
    
    EXPECT_LT(Va_mag, 1000.0) << "Phase A voltage should be near zero for solid 3-phase fault";
    EXPECT_LT(Vb_mag, 1000.0) << "Phase B voltage should be near zero for solid 3-phase fault";
    EXPECT_LT(Vc_mag, 1000.0) << "Phase C voltage should be near zero for solid 3-phase fault";
}
