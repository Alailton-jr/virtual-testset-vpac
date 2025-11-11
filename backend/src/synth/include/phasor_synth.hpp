#pragma once
#include <vector>
#include <cstdint>

struct PhasorComponent {
    double magnitude;
    double angle;
};

struct HarmonicComponent {
    int order;
    double magnitude;
    double angle;
};

class PhasorSynth {
public:
    // Generate samples from phasor (fundamental only)
    static std::vector<int16_t> synthesize(
        const PhasorComponent& phasor,
        double frequency,
        uint32_t sampleRate,
        uint32_t startSample,
        uint32_t numSamples
    );

    // Generate samples with harmonics
    static std::vector<int16_t> synthesizeWithHarmonics(
        const PhasorComponent& fundamental,
        const std::vector<HarmonicComponent>& harmonics,
        double frequency,
        uint32_t sampleRate,
        uint32_t startSample,
        uint32_t numSamples
    );

private:
    static constexpr double SCALE_FACTOR = 3276.7; // for ±10V to 16-bit
};
