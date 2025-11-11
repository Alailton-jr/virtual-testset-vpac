#include "phasor_synth.hpp"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

std::vector<int16_t> PhasorSynth::synthesize(
    const PhasorComponent& phasor,
    double frequency,
    uint32_t sampleRate,
    uint32_t startSample,
    uint32_t numSamples
) {
    std::vector<int16_t> samples;
    samples.reserve(numSamples);
    
    double omega = 2.0 * M_PI * frequency;
    double angleRad = phasor.angle * M_PI / 180.0;
    
    for (uint32_t i = 0; i < numSamples; i++) {
        double t = static_cast<double>(startSample + i) / sampleRate;
        double phase = omega * t + angleRad;
        
        // v(t) = √2 * V * sin(ωt + φ)
        double value = std::sqrt(2.0) * phasor.magnitude * std::sin(phase);
        
        // Scale to int16 range
        int16_t scaled = static_cast<int16_t>(value * SCALE_FACTOR);
        samples.push_back(scaled);
    }
    
    return samples;
}

std::vector<int16_t> PhasorSynth::synthesizeWithHarmonics(
    const PhasorComponent& fundamental,
    const std::vector<HarmonicComponent>& harmonics,
    double frequency,
    uint32_t sampleRate,
    uint32_t startSample,
    uint32_t numSamples
) {
    std::vector<int16_t> samples;
    samples.reserve(numSamples);
    
    double omega = 2.0 * M_PI * frequency;
    
    for (uint32_t i = 0; i < numSamples; i++) {
        double t = static_cast<double>(startSample + i) / sampleRate;
        double value = 0.0;
        
        // Add fundamental
        double fundPhase = omega * t + (fundamental.angle * M_PI / 180.0);
        value += std::sqrt(2.0) * fundamental.magnitude * std::sin(fundPhase);
        
        // Add harmonics
        for (const auto& harmonic : harmonics) {
            double harmPhase = harmonic.order * omega * t + (harmonic.angle * M_PI / 180.0);
            value += std::sqrt(2.0) * harmonic.magnitude * std::sin(harmPhase);
        }
        
        // Scale to int16 range
        int16_t scaled = static_cast<int16_t>(value * SCALE_FACTOR);
        samples.push_back(scaled);
    }
    
    return samples;
}
