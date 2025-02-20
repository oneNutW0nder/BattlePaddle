/*
 * RawSocketHelper
 *
 * Helps the main RawSocket class setup the actual socket.
 */

#ifndef RAWSOCKETHELPER_H
#define RAWSOCKETHELPER_H
#ifdef __unix__

#include <net/ethernet.h>
#include <arpa/inet.h>
#include <iostream>
#include <net/if.h>
#include <cstring>
#include <sys/ioctl.h>
#include <netpacket/packet.h>
#include <ifaddrs.h>
#include <vector>

#elif defined(_WIN32) || defined(WIN32)
#define OS_Windows
#include <iostream>
#include <string.h>
#include <windows.h>
#include <iphlpapi.h>
#include "windivert.h"
#endif

#if __unix__
struct arp {
    uint8_t dst_mac[6]{};
    uint8_t src_mac[6]{};
    uint16_t type{};
    uint16_t hardware_type{};
    uint16_t protocol_type{};
    uint8_t hardware_len{};
    uint8_t protocol_len{};
    uint16_t opcode{};
    uint8_t sender_mac[6]{};
    uint8_t sender_ip[4]{};
    uint8_t target_mac[6]{};
    uint8_t target_ip[4]{};
};

class RawSocketHelper {
public:
    int interfaceIndex;
    int sockFd;
    struct sockaddr_ll addr;
    std::vector<uint8_t> macAddress;
    uint32_t ipAddress;

    int getInterfaceIndexAndInfo(const char *inter);

    int createAddressStruct();

    int createSocket();

    int setSocketOptions();

    int bindSocket();

    int findOutwardFacingNIC(uint32_t destination_address);

    std::vector<uint8_t> getMacOfIP(uint32_t targetIP);
};

#elif defined(OS_Windows)
class RawSocketHelper{
    public: 
        HANDLE handle;
        INT16 priority = 0;
        WINDIVERT_ADDRESS address;
        const char* err_str;
        uint32_t ipAddress;

        int setup();

        int findOutwardFacingNIC(uint32_t destination_address);
};
#endif
#endif