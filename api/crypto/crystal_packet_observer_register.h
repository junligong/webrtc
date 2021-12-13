#include "api/crypto/crystal_packet_observer.h"
#include "rtc_base/system/rtc_export.h"

extern void RTC_EXPORT registerPacketObserver(crystal::rtc::PacketObserver *packetObserver);

extern void unRegisterPacketObserver();