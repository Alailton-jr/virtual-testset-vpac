#ifndef SIGNAL_PROCESSING_HPP
#define SIGNAL_PROCESSING_HPP

#include <vector>
#include <cmath>
#include <algorithm>  // for std::min, std::clamp

// Resample multi-channel signal data using linear interpolation
// NOTE: Linear interpolation is acceptable for low-frequency signals (<20% Nyquist)
// For higher frequencies or demanding applications, consider:
//   - Polyphase FIR filter for proper band-limiting
//   - Sinc interpolation for minimal distortion
//   - Anti-aliasing filter before downsampling
inline  std::vector<std::vector<double>> resample(std::vector<std::vector<double>> data, float fs, float new_fs){

    std::vector<std::vector<double>> data_resampled;
    data_resampled.reserve(data.size());  // Pre-allocate outer vector

    double resample_ratio = static_cast<double>(new_fs) / static_cast<double>(fs);

    for (const auto& signal : data) {
        std::vector<double> resampled_signal;
        size_t new_length = static_cast<size_t>(std::round(signal.size() * resample_ratio));
        resampled_signal.reserve(new_length);  // Pre-allocate inner vector

        for (size_t i = 0; i < new_length; ++i) {
            double original_index = static_cast<double>(i) / resample_ratio;

            size_t index_left = static_cast<size_t>(std::floor(original_index));
            size_t index_right = std::min(index_left + 1, signal.size() - 1);

            double t = original_index - static_cast<double>(index_left);
            double interpolated_value = (1.0 - t) * signal[index_left] + t * signal[index_right];

            resampled_signal.push_back(interpolated_value);
        }

        data_resampled.push_back(std::move(resampled_signal));  // Move to avoid copy
    }

    return data_resampled;
}



#endif