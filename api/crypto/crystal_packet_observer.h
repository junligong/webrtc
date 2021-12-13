#ifndef CRYSTAL_PACKET_OBSERVER_H
#define CRYSTAL_PACKET_OBSERVER_H
#include <stddef.h>

namespace crystal {
    namespace rtc {
        struct Packet {
            const unsigned char *buffer;
            size_t size;
        };
        class PacketObserver {
        public:
            PacketObserver() = default;
            virtual ~PacketObserver() = default;
            virtual bool onSendAudioPacket(Packet &packet) = 0;
            virtual bool onSendVideoPacket(Packet &packet) = 0;
            virtual bool onReceiveAudioPacket(Packet &packet) = 0;
            virtual bool onReceiveVideoPacket(Packet &packet) = 0;
        };
    }
}
extern crystal::rtc::PacketObserver *packet_observer;
#endif
