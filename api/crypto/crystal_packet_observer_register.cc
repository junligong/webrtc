#include "crystal_packet_observer_register.h"

crystal::rtc::PacketObserver *packet_observer = nullptr;
extern void registerPacketObserver(crystal::rtc::PacketObserver *packetObserver) {
    packet_observer = packetObserver;
}

extern void unRegisterPacketObserver() {
  packet_observer = NULL;
}
