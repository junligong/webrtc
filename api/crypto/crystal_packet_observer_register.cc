#include "crystal_packet_observer_register.h"

crystal::rtc::PacketObserver *packet_observer = nullptr;

#if defined(WEBRTC_WIN)
extern "C" void registerPacketObserver(crystal::rtc::PacketObserver *packetObserver) {
      packet_observer = packetObserver;
}
#else
extern void registerPacketObserver(crystal::rtc::PacketObserver *packetObserver) {
      packet_observer = packetObserver;
}
#endif

#if defined(WEBRTC_WIN)
extern "C" void unRegisterPacketObserver() {
  packet_observer = NULL;
}
#else
extern void unRegisterPacketObserver() {
  packet_observer = NULL;
}
#endif