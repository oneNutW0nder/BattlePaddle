#include "RawSocket.hpp"

using namespace std;

#ifdef __unix__

RawSocket::~RawSocket() {}

RawSocket::RawSocket(const char* intNameOrIP, bool debug, bool isIP /* =false */) : debugMode(debug) {
    rawSocketHelper = RawSocketHelper();
    rawSocketHelper.createSocket();
    if(isIP){
        rawSocketHelper.findOutwardFacingNIC(intNameOrIP);
    }else{
        rawSocketHelper.getInterfaceIndex(intNameOrIP);
    }
    rawSocketHelper.createAddressStruct();
    rawSocketHelper.setSocketOptions();
    rawSocketHelper.bindSocket();
}

RawSocket::RawSocket() = default;

Packet RawSocket::getPacket() {
    return packet;
}

int RawSocket::recieve(){
    packet.clear();
    unsigned char buf[PACKET_SIZE];
    int packetLen = recv(rawSocketHelper.sockFd, buf, PACKET_SIZE, 0);
    if(packetLen < 0){
        if ((errno != EAGAIN) && (errno != EWOULDBLOCK)){
            perror("Error in reading received packet:");
            return -1;
        }
        return -2;
    }
    packet.insert(packet.begin(), buf, buf+packetLen);
    if (debugMode) {
        cout << "From socket: " << rawSocketHelper.sockFd << endl;
        for( int i = 0; i < packet.size(); i++ ){
            cout << hex << int(packet.at(i)) << " ";
        }
        cout << endl;
    }

    return 0;
}

int RawSocket::send(Packet dataframe) {
    struct sockaddr_ll socket_address;
    socket_address.sll_ifindex = rawSocketHelper.interfaceIndex;
    socket_address.sll_halen = ETH_ALEN;

    if(dataframe.size() < 14){
        if(debugMode){
            cout << "Packet must be atleast 14 bytes long" << endl;
        }
    }else if (sendto(rawSocketHelper.sockFd, dataframe.data(), dataframe.size(), 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll)) < 0){
        perror("Send failed::");
        return -1;
    }
    return 0;
}

#elif defined(OS_Windows)

RawSocket::~RawSocket() {}

RawSocket::RawSocket(bool debug) : debugMode(debug) {
    rawSocketHelper = RawSocketHelper();
    rawSocketHelper.setup();
}

RawSocket::RawSocket() = default;

Packet* RawSocket::getPacket() {
    return packet;
}

int RawSocket::recieve() {
    if (!WinDivertRecv(rawSocketHelper.handle, packet.data, PACKET_SIZE, (UINT *)(&packet.size()), &address))
    {
        fprintf(stderr, "warning: failed to read packet (%d)\n",
            GetLastError());
        return -1;
    }
    if (debugMode) {
        SetConsoleTextAttribute(rawSocketHelper.console,
            FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | BACKGROUND_BLUE);
        cout << "RAW BYTES" << endl;
        for (int i = 0; i < packet.size(); i++) {
            cout << hex << (int)packet.at(i) << " ";
        }
        cout << endl;
    }
    return 0;
}

int RawSocket::send(Packet dataframe) {
    if (debugMode) {
        cout << "Sending currently not supported for Windows" << endl;
    }
    return 0;
}
#endif

