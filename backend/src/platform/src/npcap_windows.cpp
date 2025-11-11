#ifdef VTS_PLATFORM_WINDOWS

#include "npcap_windows.hpp"
#include <iostream>
#include <cstring>

// Npcap/WinPcap headers
#include <pcap.h>
#include <winsock2.h>
#include <iphlpapi.h>
#include <windows.h>

#pragma comment(lib, "wpcap.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

namespace vts {
namespace platform {

NpcapSocket::NpcapSocket()
    : pcap_handle_(nullptr)
    , timeout_ms_(1000)
    , promiscuous_(false) {
}

NpcapSocket::~NpcapSocket() {
    close();
}

NpcapSocket::NpcapSocket(NpcapSocket&& other) noexcept
    : pcap_handle_(other.pcap_handle_)
    , interface_(std::move(other.interface_))
    , timeout_ms_(other.timeout_ms_)
    , promiscuous_(other.promiscuous_) {
    other.pcap_handle_ = nullptr;
}

NpcapSocket& NpcapSocket::operator=(NpcapSocket&& other) noexcept {
    if (this != &other) {
        close();
        pcap_handle_ = other.pcap_handle_;
        interface_ = std::move(other.interface_);
        timeout_ms_ = other.timeout_ms_;
        promiscuous_ = other.promiscuous_;
        other.pcap_handle_ = nullptr;
    }
    return *this;
}

bool NpcapSocket::configureNpcap() {
    if (!pcap_handle_) {
        return false;
    }

    // Set non-blocking mode for read operations
    if (pcap_setnonblock(pcap_handle_, 1, nullptr) == -1) {
        std::cerr << "[Npcap] Warning: Failed to set non-blocking mode" << std::endl;
    }

    // Buffer size is set during pcap_open_live()
    std::cout << "[Npcap] Npcap device configured successfully" << std::endl;
    return true;
}

bool NpcapSocket::open(const std::string& interface) {
    if (isOpen()) {
        std::cerr << "[Npcap] Error: Device already open" << std::endl;
        return false;
    }

    // Check if Npcap is installed
    if (!isNpcapInstalled()) {
        std::cerr << "[Npcap] Error: Npcap is not installed!" << std::endl;
        std::cerr << "[Npcap] Please install Npcap from https://npcap.com/" << std::endl;
        return false;
    }

    interface_ = interface;
    char errbuf[PCAP_ERRBUF_SIZE];

    // Open device for capture
    // Parameters: device, snaplen, promisc, timeout_ms, errbuf
    pcap_handle_ = pcap_open_live(
        interface_.c_str(),     // Device name
        65536,                  // Snapshot length (max packet size)
        promiscuous_ ? 1 : 0,   // Promiscuous mode
        static_cast<int>(timeout_ms_),  // Read timeout
        errbuf                  // Error buffer
    );

    if (!pcap_handle_) {
        std::cerr << "[Npcap] Error: Failed to open device " << interface_ 
                  << ": " << errbuf << std::endl;
        return false;
    }

    std::cout << "[Npcap] Opened device: " << interface_ << std::endl;

    // Check data link type (should be Ethernet)
    int datalink = pcap_datalink(pcap_handle_);
    if (datalink != DLT_EN10MB) {
        std::cerr << "[Npcap] Warning: Data link type is not Ethernet (DLT_EN10MB)" << std::endl;
        std::cerr << "[Npcap] Got type: " << datalink << std::endl;
    }

    if (!configureNpcap()) {
        close();
        return false;
    }

    std::cout << "[Npcap] Successfully opened and configured Npcap on " << interface_ << std::endl;
    return true;
}

void NpcapSocket::close() {
    if (pcap_handle_) {
        pcap_close(pcap_handle_);
        pcap_handle_ = nullptr;
        interface_.clear();
        std::cout << "[Npcap] Closed Npcap device" << std::endl;
    }
}

std::vector<uint8_t> NpcapSocket::read() {
    if (!isOpen()) {
        return {};
    }

    struct pcap_pkthdr* header;
    const u_char* packet_data;

    // Non-blocking read (returns immediately if no packet)
    int result = pcap_next_ex(pcap_handle_, &header, &packet_data);

    if (result == 1) {
        // Packet captured successfully
        return std::vector<uint8_t>(packet_data, packet_data + header->caplen);
    } else if (result == 0) {
        // Timeout (no packet available)
        return {};
    } else if (result == -1) {
        // Error
        char* err = pcap_geterr(pcap_handle_);
        std::cerr << "[Npcap] Error reading packet: " << err << std::endl;
        return {};
    } else if (result == -2) {
        // EOF (should not happen in live capture)
        return {};
    }

    return {};
}

ssize_t NpcapSocket::write(const uint8_t* data, size_t length) {
    if (!isOpen()) {
        std::cerr << "[Npcap] Error: Cannot write, device not open" << std::endl;
        return -1;
    }

    if (length < 14) {
        std::cerr << "[Npcap] Error: Packet too short (must be at least 14 bytes for Ethernet)" << std::endl;
        return -1;
    }

    // Send raw packet
    if (pcap_sendpacket(pcap_handle_, data, static_cast<int>(length)) != 0) {
        char* err = pcap_geterr(pcap_handle_);
        std::cerr << "[Npcap] Error sending packet: " << err << std::endl;
        return -1;
    }

    return static_cast<ssize_t>(length);
}

void NpcapSocket::setTimeout(uint32_t timeout_ms) {
    timeout_ms_ = timeout_ms;
    // Note: Timeout is set during pcap_open_live(), so we'd need to reopen
    // For simplicity, we'll just store it for next open() call
}

bool NpcapSocket::setPromiscuous(bool enable) {
    promiscuous_ = enable;
    
    // If already open, we need to reopen with new settings
    if (isOpen()) {
        std::string iface = interface_;
        close();
        return open(iface);
    }
    
    return true;
}

bool NpcapSocket::setFilter(const std::string& filter_expression) {
    if (!isOpen()) {
        std::cerr << "[Npcap] Error: Cannot set filter, device not open" << std::endl;
        return false;
    }

    struct bpf_program filter;
    
    // Compile filter
    if (pcap_compile(pcap_handle_, &filter, filter_expression.c_str(), 1, PCAP_NETMASK_UNKNOWN) == -1) {
        char* err = pcap_geterr(pcap_handle_);
        std::cerr << "[Npcap] Error compiling filter '" << filter_expression 
                  << "': " << err << std::endl;
        return false;
    }

    // Apply filter
    if (pcap_setfilter(pcap_handle_, &filter) == -1) {
        char* err = pcap_geterr(pcap_handle_);
        std::cerr << "[Npcap] Error setting filter: " << err << std::endl;
        pcap_freecode(&filter);
        return false;
    }

    pcap_freecode(&filter);
    std::cout << "[Npcap] Filter set: " << filter_expression << std::endl;
    return true;
}

std::vector<uint8_t> NpcapSocket::getMacAddress() const {
    if (interface_.empty()) {
        return {};
    }

    // Use Windows IP Helper API to get MAC address
    ULONG bufferSize = 15000;
    std::vector<BYTE> buffer(bufferSize);
    PIP_ADAPTER_ADDRESSES pAddresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data());

    ULONG result = GetAdaptersAddresses(
        AF_UNSPEC,
        GAA_FLAG_INCLUDE_PREFIX,
        nullptr,
        pAddresses,
        &bufferSize
    );

    if (result != ERROR_SUCCESS) {
        std::cerr << "[Npcap] Error: GetAdaptersAddresses failed with error: " << result << std::endl;
        return {};
    }

    // Find matching adapter
    for (PIP_ADAPTER_ADDRESSES pCurrAddresses = pAddresses; pCurrAddresses; pCurrAddresses = pCurrAddresses->Next) {
        // Match by adapter name (GUID portion)
        std::string adapterName = pCurrAddresses->AdapterName;
        if (interface_.find(adapterName) != std::string::npos) {
            // Found the adapter, get MAC address
            if (pCurrAddresses->PhysicalAddressLength == 6) {
                std::vector<uint8_t> mac(
                    pCurrAddresses->PhysicalAddress,
                    pCurrAddresses->PhysicalAddress + 6
                );
                return mac;
            }
        }
    }

    return {};
}

std::vector<std::string> getNetworkInterfaces() {
    std::vector<std::string> interfaces;
    
    pcap_if_t* alldevs;
    char errbuf[PCAP_ERRBUF_SIZE];

    // Get list of devices
    if (pcap_findalldevs(&alldevs, errbuf) == -1) {
        std::cerr << "[Npcap] Error: pcap_findalldevs() failed: " << errbuf << std::endl;
        return interfaces;
    }

    // Extract device names
    for (pcap_if_t* d = alldevs; d != nullptr; d = d->next) {
        interfaces.push_back(d->name);
    }

    pcap_freealldevs(alldevs);
    return interfaces;
}

std::vector<std::pair<std::string, std::string>> getNetworkInterfacesWithNames() {
    std::vector<std::pair<std::string, std::string>> interfaces;
    
    pcap_if_t* alldevs;
    char errbuf[PCAP_ERRBUF_SIZE];

    // Get list of devices
    if (pcap_findalldevs(&alldevs, errbuf) == -1) {
        std::cerr << "[Npcap] Error: pcap_findalldevs() failed: " << errbuf << std::endl;
        return interfaces;
    }

    // Extract device names and descriptions
    for (pcap_if_t* d = alldevs; d != nullptr; d = d->next) {
        std::string name = d->name;
        std::string description = d->description ? d->description : "No description";
        interfaces.push_back({name, description});
    }

    pcap_freealldevs(alldevs);
    return interfaces;
}

bool isAdmin() {
    BOOL isAdmin = FALSE;
    PSID adminGroup = nullptr;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;

    if (AllocateAndInitializeSid(
        &ntAuthority,
        2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0,
        &adminGroup)) {
        
        CheckTokenMembership(nullptr, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }

    return isAdmin == TRUE;
}

bool isNpcapInstalled() {
    // Try to load wpcap.dll (Npcap provides WinPcap compatibility)
    HMODULE hModule = LoadLibraryA("wpcap.dll");
    if (hModule) {
        FreeLibrary(hModule);
        return true;
    }
    return false;
}

} // namespace platform
} // namespace vts

#endif // VTS_PLATFORM_WINDOWS
