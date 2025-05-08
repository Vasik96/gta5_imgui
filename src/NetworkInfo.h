#pragma once

#include <iostream>
#include <mutex>
#include <pcap/pcap.h>
#include <pcap/bpf.h>
#include <unordered_set>
#include <ws2tcpip.h>

namespace NetworkInfo {

    extern std::unordered_set<std::string> uniqueIPs;  // Declare the unique IP set
    extern std::mutex ipMutex;  // Declare the mutex for thread safety
    extern int playerCount;  // Declare the player count variable
    extern pcap_if_t* selectedDevice;  // User selected network device

    // Callback function for packet capture
    void packetHandler(u_char* user, const struct pcap_pkthdr* pkthdr, const u_char* packet);

    // Method to initialize packet capturing
    void startPacketCapture();
}
