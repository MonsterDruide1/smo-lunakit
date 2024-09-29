#pragma once

#include "tas.h"
#include <math/seadQuat.h>
#include <math/seadVector.h>
#include <prim/seadSafeString.h>
#include "server.h"

namespace smo
{
    class Server;
    // negative numbers for outgoing, positive for incoming
    enum OutPacketType : u8
    {
        DummyInit = (u8) -1, Init = (u8) -2, Log = (u8) -3,
        Fragmented = (u8) -128
    };

    class OutPacket
    {
    public:
        virtual u32 calcLen() const {return 0;}
        virtual void construct(u8* dst) const {}
        virtual void constructFragment(u8* dst, u32 offset, u32 len) const {construct(dst);}
    };

    enum InPacketType : u8
    {
        PlayerScriptInfo = 1, PlayerScriptData = 2, PlayerTeleport = 3, PlayerGo = 4
    };

    class InPacket
    {
    public:
        virtual void parse(const u8* data, u32 len) {}
        virtual void on(Server& server) {}
    };

    class OutPacketLog : public OutPacket
    {
    public:
        enum LogType : u8
        {
            Log = 0, Warning = 1, Error = 2, Fatal = 3
        };
        OutPacketLog(const char* msg) : message(msg), type(Log) {}
        LogType type;
        const char* message;
        u32 calcLen() const;
        void construct(u8* dst) const;
    };

    class InPacketPlayerScriptInfo : public InPacket
    {
        char* scriptName = nullptr;
    public:
        void parse(const u8* data, u32 len);
        void on(Server& server);
    };

    class InPacketPlayerTeleport : public InPacket
    {
        sead::Vector3f pos;
    public:
        void parse(const u8* data, u32 len);
        void on(Server& server);
    };

    class InPacketPlayerGo : public InPacket
    {
        bool startScript = false;
        s8 scenario = -1;
        char* stageName = nullptr;
        char* entrance = nullptr;
    public:
        void parse(const u8* data, u32 len);
        void on(Server& server);
    };

    class InPacketPlayerScriptData : public InPacket
    {
    public:
        void parse(const u8* data, u32 len);
        void on(Server& server);
    };
}
