/*
 * PacketParse
 *
 * Contains functions to help the user decode raw bytes off the wire.
 */

#ifndef PACKETPARSE_H
#define PACKETPARSE_H

#include <climits>
#include <cstring>
#include <iomanip>
#include <ios>
#include <istream>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

#ifdef __unix__

#include <arpa/inet.h>

#elif defined(_WIN32) || defined(WIN32)
#include "windivert.h"
#define ntohs(x) WinDivertHelperNtohs(x)
#define ntohl(x) WinDivertHelperNtohl(x)
#define htons(x) WinDivertHelperHtons(x)
#define htonl(x) WinDivertHelperHtonl(x)
#endif

typedef std::vector<uint8_t> Packet;

namespace PacketParse {
const uint32_t MAGIC_BYTES = 0x42503c33;  // BP<3

#pragma pack(push, 1)
struct ether_header_t {
    uint8_t dst_mac[6];
    uint8_t src_mac[6];
    uint16_t type;
};

struct ip_header_t {
    uint8_t ver_ihl;  // 4 bits version and 4 bits internet header length
    uint8_t tos;
    uint16_t total_length;
    uint16_t id;
    uint16_t flags_fo;  // 3 bits flags and 13 bits fragment-offset
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint32_t src_addr;
    uint32_t dst_addr;

    uint8_t ihl() const;

    size_t size() const;
};

struct udp_header_t {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t length;
    uint16_t checksum;
};

struct bp_header_t {
    uint32_t magic_bytes = htonl(MAGIC_BYTES);
    uint8_t header_type{};
};

struct bp_command_request_t {
    uint8_t target_OS{};  // 0x01 Linux | 0x02 Windows
    uint32_t command_num{};
    uint32_t host_ip{};
};

struct bp_raw_command_t {
    uint8_t target_OS{};     // 0x01 Linux | 0x02 Windows
    uint32_t command_num{};  // For metrics
    uint32_t host_ip{};
    uint16_t cmd_len{};
    uint8_t raw_command[1200]{};  // The custom command
};

struct bp_response_t {
    uint32_t command_num{};  // For metrics
    uint32_t host_ip{};      // When relaying becomes a thing
    uint16_t data_len{};
    uint8_t data[1200]{};  // Other metrics about command ran or health of host
};

struct bp_keep_alive_t {
    uint8_t target_OS{};
    uint32_t command_num{};
    uint32_t host_ip{};
};

// BP proxy protocol structs
enum class proxy_code_t {
    noop_e = 0x00,        // A default/null proxy status code
    proxy_req_e = 0x20,   // Proxy request message ID
    proxy_resp_e = 0x30,  // Proxy request response message ID
    serv_info_e = 0x40,   // Let's the C2 server know this is data from a proxied session; Can also be used
                          // to identify message as part of a proxy session
    proxy_end_e = 0x50    // Signifies the end of a proxy session
};

// ! BP header will always follow proxy type
struct bp_proxy_t {
    proxy_code_t msg_type{};  // Proxy protocol code
    uint32_t session_id{};    // Unique session ID for the life of the proxy session
};

struct info_t {
    ether_header_t etherHeader{};
    ip_header_t ipHeader{};
    udp_header_t udpHeader{};
    bp_header_t bpHeader{};
    bp_command_request_t bpCommandRequest{};
    bp_raw_command_t bpRawCommand{};
    bp_response_t bpResponse{};
    bp_keep_alive_t bpKeepAlive{};
    bp_proxy_t bpProxy{};
    // bp_header_t ogBPHeader{};
    // char leftOvers[sizeof(bp_raw_command_t)];  // ! Testing for leftover proxy data
    std::string encData;
};

#pragma pack(pop)

template <typename T>
T load(std::istream &stream, bool ntoh = true);

template <>
ether_header_t load(std::istream &stream, bool ntoh);

template <>
ip_header_t load(std::istream &stream, bool ntoh);

template <>
udp_header_t load(std::istream &stream, bool ntoh);

template <>
bp_header_t load(std::istream &stream, bool ntoh);

template <>
bp_command_request_t load(std::istream &stream, bool ntoh);

template <>
bp_raw_command_t load(std::istream &stream, bool ntoh);

template <>
bp_response_t load(std::istream &stream, bool ntoh);

template <>
bp_keep_alive_t load(std::istream &stream, bool ntoh);

template <>
bp_proxy_t load(std::istream &stream, bool ntoh);

std::ostream &operator<<(std::ostream &o, const ether_header_t &a);

std::ostream &operator<<(std::ostream &o, const ip_header_t &a);

std::ostream &operator<<(std::ostream &o, const udp_header_t &a);

std::ostream &operator<<(std::ostream &o, const bp_header_t &a);

std::ostream &operator<<(std::ostream &o, const bp_command_request_t &a);

std::ostream &operator<<(std::ostream &o, const bp_raw_command_t &a);

std::ostream &operator<<(std::ostream &o, const bp_response_t &a);

std::ostream &operator<<(std::ostream &o, const bp_keep_alive_t &a);

std::ostream &operator<<(std::ostream &o, std::unique_ptr<info_t> a);

std::ostream &operator<<(std::ostream &o, const bp_proxy_t &a);

std::unique_ptr<info_t> parsePacket(Packet packet);
}  // namespace PacketParse
#endif