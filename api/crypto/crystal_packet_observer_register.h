#include "api/crypto/crystal_packet_observer.h"
#include "rtc_base/system/rtc_export.h"

extern "C" void registerPacketObserver(crystal::rtc::PacketObserver *packetObserver);

extern "C" void unRegisterPacketObserver();
