#include "api/crypto/crystal_packet_observer.h"
#include "rtc_base/system/rtc_export.h"

#if defined(WEBRTC_WIN)
extern "C" void registerPacketObserver(crystal::rtc::PacketObserver *packetObserver);
#else
extern void RTC_EXPORT registerPacketObserver(crystal::rtc::PacketObserver *packetObserver);
#endif

#if defined(WEBRTC_WIN)
extern "C" void unRegisterPacketObserver();
#else
extern void RTC_EXPORT unRegisterPacketObserver();
#endif