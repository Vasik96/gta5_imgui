#include "NetworkInfo.h"
#include "Logging.h"
#include "ProcessHandler.h"
#include "globals.h"

#include <thread>
#include <chrono>

namespace NetworkInfo {

    // Declare the variables as before
    std::unordered_set<std::string> uniqueIPs;
    std::mutex ipMutex;
    int playerCount = 0;
    pcap_if_t* selectedDevice = nullptr;  // User selected network device

    // Function to reset player data every 5 seconds
    void resetPlayerData() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(10));

            // Reset the unique IPs set and player count
            {
                std::lock_guard<std::mutex> lock(ipMutex);
                uniqueIPs.clear();
                playerCount = 0;
            }

            // Optionally log that data has been reset
            std::cout << "Player data reset. Current player count: " << playerCount << std::endl;
        }
    }

    // Callback function for packet capture
    void packetHandler(u_char* user, const struct pcap_pkthdr* pkthdr, const u_char* packet) {
        if (!ProcessHandler::IsProcessRunning(globals::gtaProcess.c_str())) {
            std::this_thread::sleep_for(std::chrono::seconds(3));
            return;
        }

        const int ethernetHeaderSize = 14;
        const int ipHeaderSize = 20;
        const int udpHeaderSize = 8;

        if (pkthdr->caplen < ethernetHeaderSize + ipHeaderSize + udpHeaderSize) {
            return;  // Packet too short
        }

        // Extract IP header
        const u_char* ipHeader = packet + ethernetHeaderSize;
        uint8_t ipVersion = (ipHeader[0] >> 4);
        if (ipVersion != 4) {
            return;  // Ignore non-IPv4 packets
        }

        // Extract UDP header
        const u_char* udpHeader = ipHeader + ipHeaderSize;
        uint16_t sourcePort = (udpHeader[0] << 8) | udpHeader[1];
        uint16_t destPort = (udpHeader[2] << 8) | udpHeader[3];

        // Check if it's UDP traffic on port 6672
        if (destPort != 6672 && sourcePort != 6672) {
            return;  // Ignore packets not on port 6672
        }

        // Extract source IP
        char sourceIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, ipHeader + 12, sourceIP, INET_ADDRSTRLEN);

        // Store unique IP
        {
            std::lock_guard<std::mutex> lock(ipMutex);
            uniqueIPs.insert(sourceIP);
            playerCount = uniqueIPs.size();  // Update player count
        }

        // Display captured IP and total unique count
        std::cout << "[Packet Captured] Source IP: " << sourceIP
            << " | Unique Players: " << playerCount << std::endl;
    }

    void startPacketCapture() {
        char errbuf[PCAP_ERRBUF_SIZE];
        pcap_if_t* allDevs;
        pcap_if_t* device = nullptr;

        // Get the list of available devices
        if (pcap_findalldevs(&allDevs, errbuf) == -1) {
            std::cerr << "Error finding devices: " << errbuf << std::endl;
            return;
        }

        // List available devices
        std::cout << "Available network devices:\n";
        int index = 0;
        pcap_if_t* lastDevice = nullptr;
        pcap_if_t* secondToLastDevice = nullptr;

        for (device = allDevs; device != nullptr; device = device->next) {
            std::cout << index++ << ": " << device->name << std::endl;

            // Store the second-to-last device (before NPF_Loopback)
            if (device->next && std::string(device->next->name) == "\\Device\\NPF_Loopback") {
                secondToLastDevice = device;
            }
        }

        // If second-to-last device exists, select it
        if (secondToLastDevice) {
            selectedDevice = secondToLastDevice;
            std::cout << "Automatically selected device: " << selectedDevice->name << std::endl;
        }
        else {
            std::cerr << "No valid network devices found!" << std::endl;
            pcap_freealldevs(allDevs);
            return;
        }

        // Open the selected network interface
        pcap_t* handle = pcap_open_live(selectedDevice->name, 65536, 1, 5000, errbuf);
        if (!handle) {
            std::cerr << "Error opening device " << selectedDevice->name << ": " << errbuf << std::endl;
            pcap_freealldevs(allDevs);
            return;
        }

        // Set a filter for UDP packets on port 6672
        struct bpf_program filter;
        std::string filterExp = "udp port 6672";  // Capture only UDP packets on port 6672
        if (pcap_compile(handle, &filter, filterExp.c_str(), 0, PCAP_NETMASK_UNKNOWN) == -1 ||
            pcap_setfilter(handle, &filter) == -1) {
            std::cerr << "Error setting filter on device " << selectedDevice->name << ": " << pcap_geterr(handle) << std::endl;
            pcap_close(handle);
            pcap_freealldevs(allDevs);
            return;
        }

        std::cout << "Listening for UDP packets on device: " << selectedDevice->name << "...\n";

        // Start packet capture loop in a separate thread
        std::thread resetThread(resetPlayerData);  // Start the reset timer in the background

        // Start packet capture loop
        int res = pcap_loop(handle, 0, packetHandler, nullptr);
        if (res == -1) {
            std::cerr << "Error in packet capture: " << pcap_geterr(handle) << std::endl;
        }

        // Cleanup
        pcap_close(handle);
        pcap_freealldevs(allDevs);

        // Ensure to join the reset thread
        resetThread.join();
    }
}
