#include "RawSocket.hpp"
#include "PacketParse.hpp"
#include <sstream>
#include <csignal>
#include <vector>

using namespace std;
using namespace PacketParse;

/*
**************************************************************************
THIS IS FOR TESING OF THE RAWSOCKET LIBRARY NOT AN ACTUAL USEFUL PROGRAM.
**************************************************************************
*/


vector<RawSocket> socks;

void cleanup(int i){
    cout << "Safely shutting down..." << endl;
    exit(i);
}

int main(){
    signal(SIGINT, cleanup);

    #ifdef __unix__
    socks.emplace_back(RawSocket("127.0.0.1", false, true));
    #elif defined(OS_Windows)
    socks.push_back(RawSocket(false));
    #endif
    for (int m = 0;m < 10000; m++) {
        for(auto sock: socks){
            sock.receive();
            Packet pack = sock.getPacket();
            unique_ptr<info_t> info = parsePacket(pack);
            cout << move(info) << endl;

            ether_header_t ether_header{};
            ip_header_t ip_header{};
            udp_header_t udp_header{};
            bp_header_t bpHeader{};
            bp_command_request_t bp_command_request_header;

            bpHeader.header_type = 0x01;

            bp_command_request_header.target_OS = 0x01;

            uint16_t udp_len = htons(sizeof(bp_command_request_header) + sizeof(udp_header));
            udp_header.length = udp_len;
            udp_header.dst_port = htons(80);
            udp_header.src_port = htons(80);
            udp_header.checksum = 0xBC;

            ip_header.ver_ihl = 0x45;
            ip_header.total_length = udp_len + htons(20);
            ip_header.id = 0xda80;
            ip_header.flags_fo = 0x4000;
            ip_header.ttl = 0x01;
            ip_header.protocol = 0x11;
            ip_header.checksum = htons(0x1337);
            ip_header.tos = 0x01;
            ip_header.src_addr = 0x81515640;
            ip_header.dst_addr = 0xeffffffa;
            memcpy(ether_header.dst_mac, (const uint8_t []){0xFF, 0xF8, 0xFF, 0xFF, 0xFF, 0xFF}, 6);
            memcpy(ether_header.src_mac, (const uint8_t []){0x12, 0x34, 0x00, 0x78, 0xAC, 0xDC}, 6);
            ether_header.type = 0x0800;      

            auto ether_ptr = reinterpret_cast<unsigned char *>(&ether_header);     
            auto ip_ptr = reinterpret_cast<unsigned char *>(&ip_header);     
            auto udp_ptr = reinterpret_cast<unsigned char *>(&udp_header);
            auto bp_header_ptr = reinterpret_cast<unsigned  char *>(&bpHeader);
            auto bp_ptr = reinterpret_cast<unsigned char *>(&bp_command_request_header);     

            Packet meep(ether_ptr, ether_ptr + sizeof(ether_header));
            meep.insert(meep.end(), ip_ptr, ip_ptr + sizeof(ip_header));
            meep.insert(meep.end(), udp_ptr, udp_ptr + sizeof(udp_header));
            meep.insert(meep.end(), bp_header_ptr, bp_header_ptr + sizeof(bpHeader));
            meep.insert(meep.end(), bp_ptr, bp_ptr + sizeof(bp_command_request_header));

            sock.send(meep);
        }
    }
}


/*
**************************************************************************
THIS IS FOR TESING OF THE RAWSOCKET LIBRARY NOT AN ACTUAL USEFUL PROGRAM.
**************************************************************************
*/
