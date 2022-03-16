#include "crystal_packet_observer_register.h"

crystal::rtc::PacketObserver *packet_observer = nullptr;
extern "C" void registerPacketObserver(crystal::rtc::PacketObserver *packetObserver) {
    packet_observer = packetObserver;
}

extern "C" void unRegisterPacketObserver() {
  packet_observer = NULL;
}
