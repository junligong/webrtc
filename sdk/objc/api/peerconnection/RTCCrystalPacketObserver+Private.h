//
//  RTCCrystalPacketObserver+Private.h
//  AFNetworking
//
//  Created by 符吉胜 on 2020/9/13.
//

#import "RTCCrystalPacketObserver.h"
#include "api/crypto/crystal_packet_observer.h"
#include "api/crypto/crystal_packet_observer_register.h"
//todo include 虚拟接口头文件crystal_packet_observer

NS_ASSUME_NONNULL_BEGIN

namespace crystal {

namespace rtc {
    class CrystalPacketDelegateAdapter: public PacketObserver {
    public:
        CrystalPacketDelegateAdapter(RTCCrystalPacketObserver *packetObserver);
        ~CrystalPacketDelegateAdapter() override;
        
        bool onSendAudioPacket(Packet &packet) override;
        bool onSendVideoPacket(Packet &packet) override;
        bool onReceiveAudioPacket(Packet &packet) override;
        bool onReceiveVideoPacket(Packet &packet) override;
        
        private:
        __weak RTCCrystalPacketObserver *packetObserver_;
    };
} //end rtc
} //end crystal

NS_ASSUME_NONNULL_END
