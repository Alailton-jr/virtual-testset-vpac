#ifndef IMPEDANCE_CALCULATOR_HPP
#define IMPEDANCE_CALCULATOR_HPP

#include <complex>
#include <string>

namespace vts {
namespace testers {

/**
 * @brief Fault types for impedance injection
 */
enum class FaultType {
    AG,      // Phase A to Ground
    BG,      // Phase B to Ground
    CG,      // Phase C to Ground
    AB,      // Phase A to Phase B
    BC,      // Phase B to Phase C
    CA,      // Phase C to Phase A
    ABG,     // Phase A-B to Ground
    BCG,     // Phase B-C to Ground
    CAG,     // Phase C-A to Ground
    ABC      // Three-phase fault
};

/**
 * @brief Source impedance parameters (sequence networks)
 */
struct SourceImpedance {
    double RS1;      // Positive sequence resistance (Ω)
    double XS1;      // Positive sequence reactance (Ω)
    double RS0;      // Zero sequence resistance (Ω)
    double XS0;      // Zero sequence reactance (Ω)
    double Vprefault; // Pre-fault voltage magnitude (V)
};

/**
 * @brief Fault impedance
 */
struct FaultImpedance {
    double R;  // Resistance (Ω)
    double X;  // Reactance (Ω)
};

/**
 * @brief Three-phase phasor set
 */
struct ThreePhasePhasor {
    std::complex<double> A;
    std::complex<double> B;
    std::complex<double> C;
};

/**
 * @brief Complete phasor state (voltages and currents)
 */
struct PhasorState {
    ThreePhasePhasor voltage;  // Line-to-ground voltages (V)
    ThreePhasePhasor current;  // Phase currents (A)
    ThreePhasePhasor neutral;  // Neutral quantities if applicable
};

/**
 * @brief Impedance Calculator using symmetrical components
 * 
 * Converts fault impedance and type into voltage and current phasors
 * for all three phases using sequence network analysis.
 */
class ImpedanceCalculator {
public:
    ImpedanceCalculator();
    ~ImpedanceCalculator() = default;
    
    /**
     * @brief Calculate phasors for a given fault condition
     * 
     * @param faultType Type of fault
     * @param faultZ Fault impedance (R + jX)
     * @param source Source impedance parameters
     * @return PhasorState Complete voltage and current phasors
     */
    PhasorState calculateFault(FaultType faultType, 
                               const FaultImpedance& faultZ,
                               const SourceImpedance& source);
    
    /**
     * @brief Convert fault type string to enum
     */
    static FaultType parseFaultType(const std::string& str);
    
    /**
     * @brief Convert fault type enum to string
     */
    static std::string faultTypeToString(FaultType type);

private:
    // Alpha operator for symmetrical components (1∠120°)
    static constexpr std::complex<double> alpha() {
        return std::complex<double>(-0.5, 0.866025403784439);
    }
    
    // Alpha squared (1∠240°)
    static constexpr std::complex<double> alpha2() {
        return std::complex<double>(-0.5, -0.866025403784439);
    }
    
    /**
     * @brief Transform ABC to sequence components (012)
     * 
     * V0 = (Va + Vb + Vc) / 3
     * V1 = (Va + α*Vb + α²*Vc) / 3
     * V2 = (Va + α²*Vb + α*Vc) / 3
     */
    ThreePhasePhasor abcToSequence(const ThreePhasePhasor& abc);
    
    /**
     * @brief Transform sequence components (012) to ABC
     * 
     * Va = V0 + V1 + V2
     * Vb = V0 + α²*V1 + α*V2
     * Vc = V0 + α*V1 + α²*V2
     */
    ThreePhasePhasor sequenceToAbc(const ThreePhasePhasor& seq);
    
    /**
     * @brief Calculate fault for single line-to-ground faults
     */
    PhasorState calculateSLG(char phase, const FaultImpedance& faultZ, 
                            const SourceImpedance& source);
    
    /**
     * @brief Calculate fault for line-to-line faults
     */
    PhasorState calculateLL(char phase1, char phase2, 
                           const FaultImpedance& faultZ,
                           const SourceImpedance& source);
    
    /**
     * @brief Calculate fault for double line-to-ground faults
     */
    PhasorState calculateDLG(char phase1, char phase2,
                            const FaultImpedance& faultZ,
                            const SourceImpedance& source);
    
    /**
     * @brief Calculate fault for three-phase faults
     */
    PhasorState calculate3Ph(const FaultImpedance& faultZ,
                            const SourceImpedance& source);
};

} // namespace testers
} // namespace vts

#endif // IMPEDANCE_CALCULATOR_HPP
