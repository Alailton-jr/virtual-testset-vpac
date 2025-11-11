#include "comtrade_parser.hpp"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cctype>

namespace vts {
namespace io {

ComtradeParser::ComtradeParser() 
    : loaded_(false) {
}

ComtradeParser::~ComtradeParser() {
    clear();
}

void ComtradeParser::clear() {
    config_ = ComtradeConfig();
    samples_.clear();
    loaded_ = false;
    lastError_.clear();
}

void ComtradeParser::setError(const std::string& msg) {
    lastError_ = msg;
    loaded_ = false;
}

std::string ComtradeParser::trim(const std::string& str) {
    if (str.empty()) {
        return "";
    }
    
    size_t start = 0;
    size_t end = str.length();
    
    while (start < end && std::isspace(static_cast<unsigned char>(str[start]))) {
        start++;
    }
    
    while (end > start && std::isspace(static_cast<unsigned char>(str[end - 1]))) {
        end--;
    }
    
    // Ensure we don't have underflow when start >= end
    if (start >= end) {
        return "";
    }
    
    // Safe calculation: we know end > start from the check above
    size_t length = end - start;
    return str.substr(start, length);
}

std::vector<std::string> ComtradeParser::splitLine(const std::string& line, char delim) {
    std::vector<std::string> tokens;
    std::stringstream ss(line);
    std::string token;
    
    while (std::getline(ss, token, delim)) {
        tokens.push_back(trim(token));
    }
    
    return tokens;
}

bool ComtradeParser::load(const std::string& cfgPath, const std::string& datPath) {
    clear();
    
    // Parse configuration file
    if (!parseCfg(cfgPath)) {
        return false;
    }
    
    // Determine .dat file path if not provided
    std::string datFile = datPath;
    if (datFile.empty()) {
        // Replace .cfg extension with .dat
        size_t dotPos = cfgPath.find_last_of('.');
        if (dotPos != std::string::npos) {
            datFile = cfgPath.substr(0, dotPos) + ".dat";
        } else {
            datFile = cfgPath + ".dat";
        }
    }
    
    // Parse data file based on format
    bool success = false;
    switch (config_.dataFormat) {
        case DataFormat::ASCII:
            success = parseDatAscii(datFile);
            break;
        case DataFormat::BINARY:
            success = parseDatBinary(datFile);
            break;
        case DataFormat::BINARY32:
            success = parseDatBinary32(datFile);
            break;
        default:
            setError("Unknown data format");
            return false;
    }
    
    if (success) {
        loaded_ = true;
    }
    
    return success;
}

bool ComtradeParser::parseCfg(const std::string& cfgPath) {
    std::ifstream file(cfgPath);
    if (!file.is_open()) {
        setError("Failed to open .cfg file: " + cfgPath);
        return false;
    }
    
    std::string line;
    int lineNum = 0;
    
    try {
        // Line 1: Station name, recording device ID, revision year
        if (!std::getline(file, line)) {
            setError("Empty .cfg file");
            return false;
        }
        lineNum++;
        
        auto tokens = splitLine(line);
        if (tokens.size() >= 2) {
            config_.stationName = tokens[0];
            config_.recDeviceId = tokens[1];
            config_.revisionYear = tokens.size() >= 3 ? std::stoi(tokens[2]) : 1991;
        } else {
            setError("Invalid line 1 format");
            return false;
        }
        
        // Line 2: Total channels, analog count, digital count
        if (!std::getline(file, line)) {
            setError("Missing line 2");
            return false;
        }
        lineNum++;
        
        tokens = splitLine(line);
        if (tokens.size() >= 3) {
            config_.totalChannels = std::stoi(tokens[0]);
            
            // Handle format like "16A" or just "16"
            std::string analogStr = tokens[1];
            if (std::isalpha(static_cast<unsigned char>(analogStr.back()))) {
                analogStr = analogStr.substr(0, analogStr.length() - 1);
            }
            config_.numAnalogChannels = std::stoi(analogStr);
            
            std::string digitalStr = tokens[2];
            if (std::isalpha(static_cast<unsigned char>(digitalStr.back()))) {
                digitalStr = digitalStr.substr(0, digitalStr.length() - 1);
            }
            config_.numDigitalChannels = std::stoi(digitalStr);
        } else {
            setError("Invalid line 2 format");
            return false;
        }
        
        // Parse analog channel lines
        config_.analogChannels.reserve(static_cast<size_t>(config_.numAnalogChannels));
        for (int i = 0; i < config_.numAnalogChannels; i++) {
            if (!std::getline(file, line)) {
                setError("Missing analog channel line");
                return false;
            }
            lineNum++;
            
            AnalogChannel channel;
            if (!parseAnalogChannelLine(line, channel)) {
                setError("Invalid analog channel line " + std::to_string(lineNum));
                return false;
            }
            config_.analogChannels.push_back(channel);
        }
        
        // Parse digital channel lines
        config_.digitalChannels.reserve(static_cast<size_t>(config_.numDigitalChannels));
        for (int i = 0; i < config_.numDigitalChannels; i++) {
            if (!std::getline(file, line)) {
                setError("Missing digital channel line");
                return false;
            }
            lineNum++;
            
            DigitalChannel channel;
            if (!parseDigitalChannelLine(line, channel)) {
                setError("Invalid digital channel line " + std::to_string(lineNum));
                return false;
            }
            config_.digitalChannels.push_back(channel);
        }
        
        // Line frequency
        if (!std::getline(file, line)) {
            setError("Missing line frequency");
            return false;
        }
        lineNum++;
        config_.lineFreq = std::stod(trim(line));
        
        // Sample rate information
        if (!std::getline(file, line)) {
            setError("Missing sample rate line");
            return false;
        }
        lineNum++;
        config_.numSampleRates = std::stoi(trim(line));
        
        config_.sampleRates.reserve(static_cast<size_t>(config_.numSampleRates));
        for (int i = 0; i < config_.numSampleRates; i++) {
            if (!std::getline(file, line)) {
                setError("Missing sample rate entry");
                return false;
            }
            lineNum++;
            
            tokens = splitLine(line);
            if (tokens.size() >= 2) {
                SampleRate sr;
                sr.rate = std::stod(tokens[0]);
                sr.endSample = std::stoi(tokens[1]);
                config_.sampleRates.push_back(sr);
            }
        }
        
        // Date and time of first data point
        if (!std::getline(file, line)) {
            setError("Missing start date");
            return false;
        }
        lineNum++;
        tokens = splitLine(line);
        if (tokens.size() >= 2) {
            config_.startDate = tokens[0];
            config_.startTime = tokens[1];
        }
        
        // Trigger date and time
        if (!std::getline(file, line)) {
            setError("Missing trigger date");
            return false;
        }
        lineNum++;
        tokens = splitLine(line);
        if (tokens.size() >= 2) {
            config_.triggerDate = tokens[0];
            config_.triggerTime = tokens[1];
        }
        
        // Data file type
        if (!std::getline(file, line)) {
            setError("Missing data file type");
            return false;
        }
        lineNum++;
        std::string formatStr = trim(line);
        if (formatStr == "ASCII") {
            config_.dataFormat = DataFormat::ASCII;
        } else if (formatStr == "BINARY") {
            config_.dataFormat = DataFormat::BINARY;
        } else if (formatStr == "BINARY32") {
            config_.dataFormat = DataFormat::BINARY32;
        } else {
            setError("Unknown data format: " + formatStr);
            return false;
        }
        
        // Time multiplication factor (optional)
        if (std::getline(file, line)) {
            lineNum++;
            std::string trimmed = trim(line);
            if (!trimmed.empty()) {
                config_.timeFactor = std::stod(trimmed);
            } else {
                config_.timeFactor = 1.0;
            }
        } else {
            config_.timeFactor = 1.0;
        }
        
    } catch (const std::exception& e) {
        setError("Error parsing .cfg file at line " + std::to_string(lineNum) + ": " + e.what());
        return false;
    }
    
    return true;
}

bool ComtradeParser::parseAnalogChannelLine(const std::string& line, AnalogChannel& channel) {
    auto tokens = splitLine(line);
    
    if (tokens.size() < 13) {
        return false;
    }
    
    try {
        channel.index = std::stoi(tokens[0]) - 1;  // Convert to 0-based
        channel.name = tokens[1];
        channel.phase = tokens[2];
        channel.ccbm = tokens[3];
        channel.units = tokens[4];
        channel.a = std::stod(tokens[5]);
        channel.b = std::stod(tokens[6]);
        channel.skew = std::stod(tokens[7]);
        channel.min = std::stod(tokens[8]);
        channel.max = std::stod(tokens[9]);
        channel.primary = std::stod(tokens[10]);
        channel.secondary = std::stod(tokens[11]);
        channel.ps = tokens[12][0];
    } catch (const std::exception&) {
        return false;
    }
    
    return true;
}

bool ComtradeParser::parseDigitalChannelLine(const std::string& line, DigitalChannel& channel) {
    auto tokens = splitLine(line);
    
    if (tokens.size() < 5) {
        return false;
    }
    
    try {
        channel.index = std::stoi(tokens[0]) - 1;  // Convert to 0-based
        channel.name = tokens[1];
        channel.phase = tokens[2];
        channel.ccbm = tokens[3];
        channel.normalState = std::stoi(tokens[4]);
    } catch (const std::exception&) {
        return false;
    }
    
    return true;
}

bool ComtradeParser::parseDatAscii(const std::string& datPath) {
    std::ifstream file(datPath);
    if (!file.is_open()) {
        setError("Failed to open .dat file: " + datPath);
        return false;
    }
    
    std::string line;
    int lineNum = 0;
    
    samples_.clear();
    
    while (std::getline(file, line)) {
        lineNum++;
        
        auto tokens = splitLine(line);
        
        // Expected: sample_number, timestamp, analog_values..., digital_values...
        size_t expectedTokens = 2 + static_cast<size_t>(config_.numAnalogChannels) + 
                               (config_.numDigitalChannels > 0 ? 1 : 0);
        
        if (tokens.size() < expectedTokens) {
            continue;  // Skip incomplete lines
        }
        
        try {
            ComtradeSample sample;
            sample.sampleNumber = std::stoi(tokens[0]);
            sample.timestamp = static_cast<uint64_t>(std::stod(tokens[1]));
            
            // Parse analog values with scaling: primary = a * secondary + b
            sample.analogValues.reserve(static_cast<size_t>(config_.numAnalogChannels));
            for (int i = 0; i < config_.numAnalogChannels; i++) {
                double rawValue = std::stod(tokens[static_cast<size_t>(2 + i)]);
                const auto& channel = config_.analogChannels[static_cast<size_t>(i)];
                double scaledValue = channel.a * rawValue + channel.b;
                sample.analogValues.push_back(scaledValue);
            }
            
            // Parse digital values (packed as bits)
            if (config_.numDigitalChannels > 0) {
                uint32_t digitalWord = static_cast<uint32_t>(std::stoul(tokens[static_cast<size_t>(2 + config_.numAnalogChannels)]));
                sample.digitalValues.reserve(static_cast<size_t>(config_.numDigitalChannels));
                for (int i = 0; i < config_.numDigitalChannels; i++) {
                    bool bitValue = (digitalWord & (1u << static_cast<unsigned>(i))) != 0;
                    sample.digitalValues.push_back(bitValue);
                }
            }
            
            samples_.push_back(sample);
            
        } catch (const std::exception& e) {
            setError("Error parsing .dat line " + std::to_string(lineNum) + ": " + e.what());
            return false;
        }
    }
    
    config_.totalSamples = static_cast<int>(samples_.size());
    return true;
}

bool ComtradeParser::parseDatBinary(const std::string& datPath) {
    std::ifstream file(datPath, std::ios::binary);
    if (!file.is_open()) {
        setError("Failed to open binary .dat file: " + datPath);
        return false;
    }
    
    samples_.clear();
    
    // Each record: 4 bytes sample#, 4 bytes timestamp, 2 bytes per analog, 2 bytes per 16 digitals
    size_t recordSize = 8;  // sample# + timestamp
    recordSize += static_cast<size_t>(config_.numAnalogChannels) * 2;  // 16-bit integers
    recordSize += ((static_cast<size_t>(config_.numDigitalChannels) + 15) / 16) * 2;  // 16-bit words
    
    std::vector<char> buffer(recordSize);
    
    while (file.read(buffer.data(), static_cast<std::streamsize>(recordSize))) {
        ComtradeSample sample;
        
        // Read sample number (32-bit)
        uint32_t sampleNum;
        std::memcpy(&sampleNum, buffer.data(), 4);
        sample.sampleNumber = static_cast<int>(sampleNum);
        
        // Read timestamp (32-bit microseconds)
        uint32_t timestamp;
        std::memcpy(&timestamp, buffer.data() + 4, 4);
        sample.timestamp = static_cast<uint64_t>(timestamp);
        
        // Read analog values (16-bit signed integers)
        sample.analogValues.reserve(static_cast<size_t>(config_.numAnalogChannels));
        for (int i = 0; i < config_.numAnalogChannels; i++) {
            int16_t rawValue;
            std::memcpy(&rawValue, buffer.data() + 8 + i * 2, 2);
            
            const auto& channel = config_.analogChannels[static_cast<size_t>(i)];
            double scaledValue = channel.a * static_cast<double>(rawValue) + channel.b;
            sample.analogValues.push_back(scaledValue);
        }
        
        // Read digital values (packed in 16-bit words)
        size_t digitalOffset = 8 + static_cast<size_t>(config_.numAnalogChannels) * 2;
        int numDigitalWords = (config_.numDigitalChannels + 15) / 16;
        
        sample.digitalValues.reserve(static_cast<size_t>(config_.numDigitalChannels));
        for (int w = 0; w < numDigitalWords; w++) {
            uint16_t digitalWord;
            std::memcpy(&digitalWord, buffer.data() + digitalOffset + w * 2, 2);
            
            for (int b = 0; b < 16 && (w * 16 + b) < config_.numDigitalChannels; b++) {
                bool bitValue = (digitalWord & (1u << static_cast<unsigned>(b))) != 0;
                sample.digitalValues.push_back(bitValue);
            }
        }
        
        samples_.push_back(sample);
    }
    
    config_.totalSamples = static_cast<int>(samples_.size());
    return true;
}

bool ComtradeParser::parseDatBinary32(const std::string& datPath) {
    std::ifstream file(datPath, std::ios::binary);
    if (!file.is_open()) {
        setError("Failed to open binary32 .dat file: " + datPath);
        return false;
    }
    
    samples_.clear();
    
    // Each record: 4 bytes sample#, 4 bytes timestamp, 4 bytes per analog, 4 bytes per 32 digitals
    size_t recordSize = 8;  // sample# + timestamp
    recordSize += static_cast<size_t>(config_.numAnalogChannels) * 4;  // 32-bit integers
    recordSize += ((static_cast<size_t>(config_.numDigitalChannels) + 31) / 32) * 4;  // 32-bit words
    
    std::vector<char> buffer(recordSize);
    
    while (file.read(buffer.data(), static_cast<std::streamsize>(recordSize))) {
        ComtradeSample sample;
        
        // Read sample number (32-bit)
        uint32_t sampleNum;
        std::memcpy(&sampleNum, buffer.data(), 4);
        sample.sampleNumber = static_cast<int>(sampleNum);
        
        // Read timestamp (32-bit microseconds)
        uint32_t timestamp;
        std::memcpy(&timestamp, buffer.data() + 4, 4);
        sample.timestamp = static_cast<uint64_t>(timestamp);
        
        // Read analog values (32-bit signed integers)
        sample.analogValues.reserve(static_cast<size_t>(config_.numAnalogChannels));
        for (int i = 0; i < config_.numAnalogChannels; i++) {
            int32_t rawValue;
            std::memcpy(&rawValue, buffer.data() + 8 + i * 4, 4);
            
            const auto& channel = config_.analogChannels[static_cast<size_t>(i)];
            double scaledValue = channel.a * static_cast<double>(rawValue) + channel.b;
            sample.analogValues.push_back(scaledValue);
        }
        
        // Read digital values (packed in 32-bit words)
        size_t digitalOffset = 8 + static_cast<size_t>(config_.numAnalogChannels) * 4;
        int numDigitalWords = (config_.numDigitalChannels + 31) / 32;
        
        sample.digitalValues.reserve(static_cast<size_t>(config_.numDigitalChannels));
        for (int w = 0; w < numDigitalWords; w++) {
            uint32_t digitalWord;
            std::memcpy(&digitalWord, buffer.data() + digitalOffset + w * 4, 4);
            
            for (int b = 0; b < 32 && (w * 32 + b) < config_.numDigitalChannels; b++) {
                bool bitValue = (digitalWord & (1u << static_cast<unsigned>(b))) != 0;
                sample.digitalValues.push_back(bitValue);
            }
        }
        
        samples_.push_back(sample);
    }
    
    config_.totalSamples = static_cast<int>(samples_.size());
    return true;
}

bool ComtradeParser::loadCSV(const std::string& csvPath,
                              double sampleRate,
                              const std::vector<std::string>& channelNames,
                              const std::vector<double>& scalingFactors) {
    clear();
    
    // Initialize basic config
    config_.stationName = "CSV Import";
    config_.recDeviceId = "VTS";
    config_.revisionYear = 1999;
    config_.lineFreq = 60.0;
    config_.dataFormat = DataFormat::ASCII;
    config_.timeFactor = 1.0;
    config_.numSampleRates = 1;
    config_.numDigitalChannels = 0;
    
    SampleRate sr;
    sr.rate = sampleRate;
    sr.endSample = 0;  // Will be updated after parsing
    config_.sampleRates.push_back(sr);
    
    // Parse CSV to determine channel count
    if (!parseCSVData(csvPath)) {
        return false;
    }
    
    if (samples_.empty()) {
        setError("No data in CSV file");
        return false;
    }
    
    // Set channel count from first sample
    config_.numAnalogChannels = static_cast<int>(samples_[0].analogValues.size());
    config_.totalChannels = config_.numAnalogChannels;
    
    // Create channel configs
    config_.analogChannels.reserve(static_cast<size_t>(config_.numAnalogChannels));
    for (int i = 0; i < config_.numAnalogChannels; i++) {
        AnalogChannel channel;
        channel.index = i;
        
        if (i < static_cast<int>(channelNames.size())) {
            channel.name = channelNames[static_cast<size_t>(i)];
        } else {
            channel.name = "CH" + std::to_string(i + 1);
        }
        
        channel.phase = "";
        channel.ccbm = "";
        channel.units = "V";
        
        // Apply scaling factors if provided (a, b pairs)
        if (i * 2 + 1 < static_cast<int>(scalingFactors.size())) {
            channel.a = scalingFactors[static_cast<size_t>(i * 2)];
            channel.b = scalingFactors[static_cast<size_t>(i * 2 + 1)];
        } else {
            channel.a = 1.0;
            channel.b = 0.0;
        }
        
        channel.skew = 0.0;
        channel.min = -1000000.0;
        channel.max = 1000000.0;
        channel.primary = 1.0;
        channel.secondary = 1.0;
        channel.ps = 'P';
        
        config_.analogChannels.push_back(channel);
    }
    
    config_.sampleRates[0].endSample = config_.totalSamples;
    loaded_ = true;
    
    return true;
}

bool ComtradeParser::parseCSVData(const std::string& csvPath) {
    std::ifstream file(csvPath);
    if (!file.is_open()) {
        setError("Failed to open CSV file: " + csvPath);
        return false;
    }
    
    std::string line;
    int lineNum = 0;
    
    // Skip header line if present
    if (std::getline(file, line)) {
        lineNum++;
        // Try to detect if it's a header (contains non-numeric data)
        auto tokens = splitLine(line);
        bool isHeader = false;
        for (const auto& token : tokens) {
            if (!token.empty() && !std::isdigit(static_cast<unsigned char>(token[0])) && 
                token[0] != '-' && token[0] != '+' && token[0] != '.') {
                isHeader = true;
                break;
            }
        }
        
        // If not a header, rewind
        if (!isHeader) {
            file.clear();
            file.seekg(0);
            lineNum = 0;
        }
    }
    
    samples_.clear();
    
    while (std::getline(file, line)) {
        lineNum++;
        
        if (line.empty() || line[0] == '#') {
            continue;  // Skip empty lines and comments
        }
        
        auto tokens = splitLine(line);
        if (tokens.empty()) {
            continue;
        }
        
        try {
            ComtradeSample sample;
            sample.sampleNumber = lineNum - 1;  // 0-based sample number
            sample.timestamp = static_cast<uint64_t>((lineNum - 1) * 1000000.0 / config_.sampleRates[0].rate);
            
            // All tokens are analog values
            sample.analogValues.reserve(tokens.size());
            for (const auto& token : tokens) {
                double value = std::stod(token);
                sample.analogValues.push_back(value);
            }
            
            samples_.push_back(sample);
            
        } catch (const std::exception& e) {
            setError("Error parsing CSV line " + std::to_string(lineNum) + ": " + e.what());
            return false;
        }
    }
    
    config_.totalSamples = static_cast<int>(samples_.size());
    return true;
}

bool ComtradeParser::getSample(int index, ComtradeSample& sample) const {
    if (index < 0 || index >= static_cast<int>(samples_.size())) {
        return false;
    }
    
    sample = samples_[static_cast<size_t>(index)];
    return true;
}

std::vector<ComtradeSample> ComtradeParser::getAllSamples() const {
    return samples_;
}

double ComtradeParser::getSampleRate(int sampleIndex) const {
    for (const auto& sr : config_.sampleRates) {
        if (sampleIndex < sr.endSample) {
            return sr.rate;
        }
    }
    
    // Return last sample rate if beyond all ranges
    if (!config_.sampleRates.empty()) {
        return config_.sampleRates.back().rate;
    }
    
    return 0.0;
}

const AnalogChannel* ComtradeParser::getAnalogChannel(const std::string& name) const {
    for (const auto& channel : config_.analogChannels) {
        if (channel.name == name) {
            return &channel;
        }
    }
    return nullptr;
}

const DigitalChannel* ComtradeParser::getDigitalChannel(const std::string& name) const {
    for (const auto& channel : config_.digitalChannels) {
        if (channel.name == name) {
            return &channel;
        }
    }
    return nullptr;
}

uint64_t ComtradeParser::calculateTimestamp(int sampleNumber) const {
    // Calculate timestamp in microseconds based on sample rate
    double rate = getSampleRate(sampleNumber);
    if (rate <= 0.0) {
        return 0;
    }
    
    return static_cast<uint64_t>((sampleNumber * 1000000.0) / rate);
}

} // namespace io
} // namespace vts
