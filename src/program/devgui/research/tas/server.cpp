#include "al/util.hpp"
#include "packet.h"
#include "nn/init.h"
#include "nn/os.h"
#include "types.h"
#include <nn/nifm.h>
#include <nn/socket.h>
#include <nn/socket.hpp>
#include <basis/seadNew.h>

#define AF_INET 2

#define SOCK_STREAM 1 /* stream socket */
#define SOCK_DGRAM 2 /* datagram socket */
#define SOCK_RAW 3
#define SOCK_RDM 4
#define SOCK_SEQPACKET 5

#define FRAGMENT_SIZE 6000

#define IN_PACKET(TYPE) case smo::InPacketType::TYPE: {\
                InPacket##TYPE p;\
                p.parse(buf + 1, len - 1);\
                p.on(*this);\
                break;}

void threadFunc(void* args)
{
    smo::Server* server = (smo::Server*) args;
    nn::TimeSpan w = nn::TimeSpan::FromNanoSeconds(1000000);
    u8* buf = (u8*) nn::init::GetAllocator()->Allocate(30720);
    while (true)
    {
        server->handlePacket(buf, 30720);
        nn::os::YieldThread();
        nn::os::SleepThread(w);
    }
    nn::init::GetAllocator()->Free(buf);
}

namespace smo
{
    void Server::sendInit(const char* ipS) {
        in_addr ip = {0};
        nn::socket::InetAton(ipS, &ip);

        server.port = nn::socket::InetHtons(SERVER_PORT);
        server.family = 2;
        server.address = ip;

        OutPacketType dummy = OutPacketType::DummyInit;
        nn::socket::SendTo(socket, &dummy, 1, 0, (struct sockaddr*) &server, sizeof(server));
        dummy = OutPacketType::Init;
        nn::socket::SendTo(socket, &dummy, 1, 0, (struct sockaddr*) &server, sizeof(server));
        
        connected = true;
    }

    void Server::start() {
        nn::nifm::Initialize();
        nn::nifm::SubmitNetworkRequest();

        while (nn::nifm::IsNetworkRequestOnHold()) {}

        if ((socket = nn::socket::Socket(AF_INET, SOCK_DGRAM, 0)) < 0) return;

        int timeout = 100;
        nn::socket::SetSockOpt(socket, 0xffff, 0x1006, (const char*)&timeout, sizeof(timeout));

        sockaddr client = {0};
        client.port = nn::socket::InetHtons(CLIENT_PORT);
        client.family = 2;
        nn::socket::Bind(socket, &client, sizeof(client));

        if (!thread)
        {
            thread = (nn::os::ThreadType*) nn::init::GetAllocator()->Allocate(sizeof(nn::os::ThreadType));
            threadStack = aligned_alloc(0x1000, 0x15000);
            nn::os::CreateThread(thread, threadFunc, this, threadStack, 0x15000, 16, 0);
            nn::os::SetThreadName(thread, "UDP Thread");
            nn::os::StartThread(thread);
        }

        started = true;
    }

    void Server::connect(const char* ipS)
    {
        if (!started)
            start();

        sendInit(ipS);
    }

    void Server::disconnect()
    {
        if (thread)
        {
            nn::os::SuspendThread(thread);
            nn::os::DestroyThread(thread);
            /*free(thread);
            if (threadStack) free(threadStack);
            thread = nullptr;
            threadStack = nullptr;*/
        }
        if (socket != -1)
        {
            nn::socket::Close(socket);
            socket = -1;
        }
        connected = false;
    }

    void Server::sendPacket(const OutPacket& packet, OutPacketType type)
    {
        u32 len = packet.calcLen();
        
        if(len > FRAGMENT_SIZE) {
            sendFragmentedPacket(packet, type);
        } else {
            u8 data[len + 1];
            data[0] = type;
            packet.construct(data + 1);
            nn::socket::SendTo(socket, data, len + 1, 0, (struct sockaddr*) &server, sizeof(server));
        }
    }

    void Server::sendFragmentedPacket(const OutPacket& packet, OutPacketType type) {
        u32 len = packet.calcLen();
        u32 packetCount = len / FRAGMENT_SIZE;
        if(len % FRAGMENT_SIZE != 0) packetCount++;
        log("Sending fragmented packet with %d packets, total: %d", packetCount, len);
        u32 packetSize = len / packetCount;
        u32 packetIndex = 0;
        u32 packetOffset = 0;
        while(packetIndex < packetCount) {
            u8 packetData[packetSize + 2];
            u32 packetLen = packetSize;
            packetData[0] = OutPacketType::Fragmented;
            packetData[1] = packetIndex != packetCount - 1 ? (packetIndex % 128) : (u8)(-1);
            if(packetIndex == 0) packetData[2] = type;
            log("fragment from %d, len %d", packetOffset, packetIndex == packetCount-1 ? (len < packetOffset ? 0 : len-packetOffset) : 0);//  : (packetLen - (packetIndex == 0 ? 1 : 0)));
            //packet.constructFragment(packetData + (packetIndex == 0 ? 3 : 2), packetOffset, packetIndex == packetCount-1 ? (len < packetOffset ? 0 : len-packetOffset) : (packetLen - (packetIndex == 0 ? 1 : 0)));
            //nn::socket::SendTo(socket, packetData, packetLen + 2, 0, (struct sockaddr*) &server, sizeof(server));
            packetIndex++;
            packetOffset += packetLen;
        }
    }

    void Server::log(const char* fmt, ...) {
        char buf[0x100];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        const auto& packet = OutPacketLog(buf);
        sendPacket(packet, OutPacketType::Log);
    }

    void Server::handlePacket(u8* buf, size_t bufSize)
    {
        if (!connected) return;
        static int i = 0;
        i++;
        u32 size = sizeof(server);
        u32 len = nn::socket::RecvFrom(socket, buf, bufSize, 0, &server, &size);
        switch ((InPacketType) buf[0])
        {
            case 0: break; //timeout
            IN_PACKET(PlayerScriptInfo);
            IN_PACKET(PlayerTeleport);
            IN_PACKET(PlayerGo);
            IN_PACKET(PlayerScriptData);
            default: break;
        }
    }

    bool Server::isConnected()
    {
        return connected;
    }
}

#undef IN_PACKET
