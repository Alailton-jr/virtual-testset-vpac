#include "impedance_calculator.hpp"
#include <cmath>
#include <stdexcept>

namespace vts {
namespace testers {

ImpedanceCalculator::ImpedanceCalculator() {
}

FaultType ImpedanceCalculator::parseFaultType(const std::string& str) {
    if (str == "AG" || str == "ag") return FaultType::AG;
    if (str == "BG" || str == "bg") return FaultType::BG;
    if (str == "CG" || str == "cg") return FaultType::CG;
    if (str == "AB" || str == "ab") return FaultType::AB;
    if (str == "BC" || str == "bc") return FaultType::BC;
    if (str == "CA" || str == "ca") return FaultType::CA;
    if (str == "ABG" || str == "abg") return FaultType::ABG;
    if (str == "BCG" || str == "bcg") return FaultType::BCG;
    if (str == "CAG" || str == "cag") return FaultType::CAG;
    if (str == "ABC" || str == "abc" || str == "3PH" || str == "3ph") return FaultType::ABC;
    
    throw std::invalid_argument("Unknown fault type: " + str);
}

std::string ImpedanceCalculator::faultTypeToString(FaultType type) {
    switch (type) {
        case FaultType::AG: return "AG";
        case FaultType::BG: return "BG";
        case FaultType::CG: return "CG";
        case FaultType::AB: return "AB";
        case FaultType::BC: return "BC";
        case FaultType::CA: return "CA";
        case FaultType::ABG: return "ABG";
        case FaultType::BCG: return "BCG";
        case FaultType::CAG: return "CAG";
        case FaultType::ABC: return "ABC";
        default: return "UNKNOWN";
    }
}

ThreePhasePhasor ImpedanceCalculator::abcToSequence(const ThreePhasePhasor& abc) {
    ThreePhasePhasor seq;
    
    // Zero sequence: (Va + Vb + Vc) / 3
    seq.A = (abc.A + abc.B + abc.C) / 3.0;
    
    // Positive sequence: (Va + α*Vb + α²*Vc) / 3
    seq.B = (abc.A + alpha() * abc.B + alpha2() * abc.C) / 3.0;
    
    // Negative sequence: (Va + α²*Vb + α*Vc) / 3
    seq.C = (abc.A + alpha2() * abc.B + alpha() * abc.C) / 3.0;
    
    return seq;
}

ThreePhasePhasor ImpedanceCalculator::sequenceToAbc(const ThreePhasePhasor& seq) {
    ThreePhasePhasor abc;
    
    // Va = V0 + V1 + V2
    abc.A = seq.A + seq.B + seq.C;
    
    // Vb = V0 + α²*V1 + α*V2
    abc.B = seq.A + alpha2() * seq.B + alpha() * seq.C;
    
    // Vc = V0 + α*V1 + α²*V2
    abc.C = seq.A + alpha() * seq.B + alpha2() * seq.C;
    
    return abc;
}

PhasorState ImpedanceCalculator::calculateSLG(char phase, 
                                              const FaultImpedance& faultZ,
                                              const SourceImpedance& source) {
    PhasorState result;
    
    // Source impedances in sequence networks
    std::complex<double> Zs1(source.RS1, source.XS1);
    std::complex<double> Zs0(source.RS0, source.XS0);
    std::complex<double> Zs2 = Zs1;  // Typically equal for balanced systems
    
    // Fault impedance
    std::complex<double> Zf(faultZ.R, faultZ.X);
    
    // Pre-fault voltage (positive sequence only)
    std::complex<double> Vprefault(source.Vprefault, 0.0);
    
    // For SLG fault on phase A:
    // V1 = V2 = V0
    // I1 = I2 = I0 = Vprefault / (Zs1 + Zs2 + Zs0 + 3*Zf)
    
    std::complex<double> Zeq = Zs1 + Zs2 + Zs0 + 3.0 * Zf;
    std::complex<double> I1 = Vprefault / Zeq;
    std::complex<double> I2 = I1;
    std::complex<double> I0 = I1;
    
    // Sequence currents
    ThreePhasePhasor Iseq;
    Iseq.A = I0;
    Iseq.B = I1;
    Iseq.C = I2;
    
    // Transform to ABC
    ThreePhasePhasor Iabc = sequenceToAbc(Iseq);
    
    // Sequence voltages at fault point
    ThreePhasePhasor Vseq;
    Vseq.A = -Zs0 * I0;  // V0
    Vseq.B = Vprefault - Zs1 * I1;  // V1
    Vseq.C = -Zs2 * I2;  // V2
    
    // Transform to ABC
    ThreePhasePhasor Vabc = sequenceToAbc(Vseq);
    
    // Rotate based on which phase has the fault
    if (phase == 'B' || phase == 'b') {
        // Rotate by -120° (multiply by α²)
        result.current.A = Iabc.C;
        result.current.B = Iabc.A;
        result.current.C = Iabc.B;
        
        result.voltage.A = Vabc.C;
        result.voltage.B = Vabc.A;
        result.voltage.C = Vabc.B;
    } else if (phase == 'C' || phase == 'c') {
        // Rotate by +120° (multiply by α)
        result.current.A = Iabc.B;
        result.current.B = Iabc.C;
        result.current.C = Iabc.A;
        
        result.voltage.A = Vabc.B;
        result.voltage.B = Vabc.C;
        result.voltage.C = Vabc.A;
    } else {
        // Phase A (no rotation)
        result.current = Iabc;
        result.voltage = Vabc;
    }
    
    return result;
}

PhasorState ImpedanceCalculator::calculateLL(char phase1, char phase2,
                                             const FaultImpedance& faultZ,
                                             const SourceImpedance& source) {
    PhasorState result;
    
    // Source impedances
    std::complex<double> Zs1(source.RS1, source.XS1);
    std::complex<double> Zs2 = Zs1;
    std::complex<double> Zf(faultZ.R, faultZ.X);
    
    // Pre-fault voltage
    std::complex<double> Vprefault(source.Vprefault, 0.0);
    
    // For LL fault (no ground):
    // I0 = 0 (no zero sequence)
    // I1 = -I2 = Vprefault / (Zs1 + Zs2 + Zf)
    
    std::complex<double> Zeq = Zs1 + Zs2 + Zf;
    std::complex<double> I1 = Vprefault / Zeq;
    std::complex<double> I2 = -I1;
    
    // Sequence currents
    ThreePhasePhasor Iseq;
    Iseq.A = 0.0;  // I0
    Iseq.B = I1;
    Iseq.C = I2;
    
    // Transform to ABC
    ThreePhasePhasor Iabc = sequenceToAbc(Iseq);
    
    // Sequence voltages
    ThreePhasePhasor Vseq;
    Vseq.A = 0.0;  // V0
    Vseq.B = Vprefault - Zs1 * I1;  // V1
    Vseq.C = -Zs2 * I2;  // V2
    
    // Transform to ABC
    ThreePhasePhasor Vabc = sequenceToAbc(Vseq);
    
    // Rotate based on which phases have the fault
    if ((phase1 == 'B' && phase2 == 'C') || (phase1 == 'C' && phase2 == 'B')) {
        // BC fault - no rotation needed for BC
        result.current = Iabc;
        result.voltage = Vabc;
    } else if ((phase1 == 'C' && phase2 == 'A') || (phase1 == 'A' && phase2 == 'C')) {
        // CA fault - rotate by +120°
        result.current.A = Iabc.B;
        result.current.B = Iabc.C;
        result.current.C = Iabc.A;
        
        result.voltage.A = Vabc.B;
        result.voltage.B = Vabc.C;
        result.voltage.C = Vabc.A;
    } else {
        // AB fault - rotate by -120°
        result.current.A = Iabc.C;
        result.current.B = Iabc.A;
        result.current.C = Iabc.B;
        
        result.voltage.A = Vabc.C;
        result.voltage.B = Vabc.A;
        result.voltage.C = Vabc.B;
    }
    
    return result;
}

PhasorState ImpedanceCalculator::calculateDLG(char phase1, char phase2,
                                              const FaultImpedance& faultZ,
                                              const SourceImpedance& source) {
    PhasorState result;
    
    // Source impedances
    std::complex<double> Zs1(source.RS1, source.XS1);
    std::complex<double> Zs2 = Zs1;
    std::complex<double> Zs0(source.RS0, source.XS0);
    std::complex<double> Zf(faultZ.R, faultZ.X);
    
    // Pre-fault voltage
    std::complex<double> Vprefault(source.Vprefault, 0.0);
    
    // For DLG fault (phases B and C to ground):
    // More complex - parallel connection of zero and negative sequences
    // I1 = Vprefault / (Zs1 + (Zs2 || (Zs0 + 3*Zf)))
    
    std::complex<double> Z0_branch = Zs0 + 3.0 * Zf;
    std::complex<double> Zparallel = (Zs2 * Z0_branch) / (Zs2 + Z0_branch);
    std::complex<double> Zeq = Zs1 + Zparallel;
    
    std::complex<double> I1 = Vprefault / Zeq;
    
    // Voltage at fault point
    std::complex<double> V1 = Vprefault - Zs1 * I1;
    
    // I2 and I0 from voltage divider
    std::complex<double> I2 = -V1 / Zs2;
    std::complex<double> I0 = -V1 / Z0_branch;
    
    // Sequence currents
    ThreePhasePhasor Iseq;
    Iseq.A = I0;
    Iseq.B = I1;
    Iseq.C = I2;
    
    // Transform to ABC
    ThreePhasePhasor Iabc = sequenceToAbc(Iseq);
    
    // Sequence voltages
    ThreePhasePhasor Vseq;
    Vseq.A = -Zs0 * I0;  // V0
    Vseq.B = V1;         // V1
    Vseq.C = -Zs2 * I2;  // V2
    
    // Transform to ABC
    ThreePhasePhasor Vabc = sequenceToAbc(Vseq);
    
    // Rotate based on which phases have the fault
    if ((phase1 == 'B' && phase2 == 'C') || (phase1 == 'C' && phase2 == 'B')) {
        // BC-G fault
        result.current = Iabc;
        result.voltage = Vabc;
    } else if ((phase1 == 'C' && phase2 == 'A') || (phase1 == 'A' && phase2 == 'C')) {
        // CA-G fault - rotate by +120°
        result.current.A = Iabc.B;
        result.current.B = Iabc.C;
        result.current.C = Iabc.A;
        
        result.voltage.A = Vabc.B;
        result.voltage.B = Vabc.C;
        result.voltage.C = Vabc.A;
    } else {
        // AB-G fault - rotate by -120°
        result.current.A = Iabc.C;
        result.current.B = Iabc.A;
        result.current.C = Iabc.B;
        
        result.voltage.A = Vabc.C;
        result.voltage.B = Vabc.A;
        result.voltage.C = Vabc.B;
    }
    
    return result;
}

PhasorState ImpedanceCalculator::calculate3Ph(const FaultImpedance& faultZ,
                                              const SourceImpedance& source) {
    PhasorState result;
    
    // Source impedance
    std::complex<double> Zs1(source.RS1, source.XS1);
    std::complex<double> Zf(faultZ.R, faultZ.X);
    
    // Pre-fault voltage
    std::complex<double> Vprefault(source.Vprefault, 0.0);
    
    // For balanced 3-phase fault:
    // Only positive sequence exists
    // I0 = I2 = 0
    // I1 = Vprefault / (Zs1 + Zf)
    
    std::complex<double> Zeq = Zs1 + Zf;
    std::complex<double> I1 = Vprefault / Zeq;
    
    // Sequence currents
    ThreePhasePhasor Iseq;
    Iseq.A = 0.0;  // I0
    Iseq.B = I1;   // I1
    Iseq.C = 0.0;  // I2
    
    // Transform to ABC
    result.current = sequenceToAbc(Iseq);
    
    // Sequence voltages
    ThreePhasePhasor Vseq;
    Vseq.A = 0.0;  // V0
    Vseq.B = Vprefault - Zs1 * I1;  // V1
    Vseq.C = 0.0;  // V2
    
    // Transform to ABC
    result.voltage = sequenceToAbc(Vseq);
    
    return result;
}

PhasorState ImpedanceCalculator::calculateFault(FaultType faultType,
                                                const FaultImpedance& faultZ,
                                                const SourceImpedance& source) {
    switch (faultType) {
        case FaultType::AG:
            return calculateSLG('A', faultZ, source);
        case FaultType::BG:
            return calculateSLG('B', faultZ, source);
        case FaultType::CG:
            return calculateSLG('C', faultZ, source);
        case FaultType::AB:
            return calculateLL('A', 'B', faultZ, source);
        case FaultType::BC:
            return calculateLL('B', 'C', faultZ, source);
        case FaultType::CA:
            return calculateLL('C', 'A', faultZ, source);
        case FaultType::ABG:
            return calculateDLG('A', 'B', faultZ, source);
        case FaultType::BCG:
            return calculateDLG('B', 'C', faultZ, source);
        case FaultType::CAG:
            return calculateDLG('C', 'A', faultZ, source);
        case FaultType::ABC:
            return calculate3Ph(faultZ, source);
        default:
            throw std::invalid_argument("Unknown fault type");
    }
}

} // namespace testers
} // namespace vts
