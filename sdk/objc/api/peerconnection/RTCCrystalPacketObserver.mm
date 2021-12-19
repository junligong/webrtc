//
//  RTCCrystalPacketObserver.m
//  AFNetworking
//
//  Created by 符吉胜 on 2020/9/13.
//

#import "RTCCrystalPacketObserver.h"
#import "RTCCrystalPacketObserver+Private.h"

@implementation RTCPacket
@end


namespace crystal {

namespace rtc {
    CrystalPacketDelegateAdapter::CrystalPacketDelegateAdapter(RTCCrystalPacketObserver *packetObserver) {
        packetObserver_ = packetObserver;
    }
    
    CrystalPacketDelegateAdapter::~CrystalPacketDelegateAdapter() {
        packetObserver_ = nil;
    }
    
    bool CrystalPacketDelegateAdapter::onSendAudioPacket(Packet &packet) {
        //此处转成RTCPacket
        RTCPacket *rtcPacket = [[RTCPacket alloc] init];
        rtcPacket->buffer = &packet.buffer;
        rtcPacket->size = &packet.size;
        
        return [packetObserver_.delegate onSendAudioPacket:rtcPacket];
    }
    
    bool CrystalPacketDelegateAdapter::onSendVideoPacket(Packet &packet) {
        RTCPacket *rtcPacket = [[RTCPacket alloc] init];
        rtcPacket->buffer = &packet.buffer;
        rtcPacket->size = &packet.size;
        
        return [packetObserver_.delegate onSendVideoPacket:rtcPacket];
    }
    
    bool CrystalPacketDelegateAdapter::onReceiveAudioPacket(Packet &packet) {
        RTCPacket *rtcPacket = [[RTCPacket alloc] init];
        rtcPacket->buffer = &packet.buffer;
        rtcPacket->size = &packet.size;
       
        return [packetObserver_.delegate onReceiveAudioPacket:rtcPacket];
    }
    
    bool CrystalPacketDelegateAdapter::onReceiveVideoPacket(Packet &packet) {
        RTCPacket *rtcPacket = [[RTCPacket alloc] init];
        rtcPacket->buffer = &packet.buffer;
        rtcPacket->size = &packet.size;
        
        return [packetObserver_.delegate onReceiveVideoPacket:rtcPacket];
    }
    
} //end rtc
} //end crystal


@implementation RTCCrystalPacketObserver {
    crystal::rtc::CrystalPacketDelegateAdapter *_observer;
}

@synthesize delegate = _delegate;

- (void)dealloc {
    unRegisterPacketObserver();
  
    delete _observer;
    _observer = nil;
  
    NSLog(@"RTCCrystalPacketObserver dealloc");
}

- (void)addPacketObserverDelegate:(id<RTCCrystalPacketObserverDelegate>)delegate {
    _delegate = delegate;
    
    //初始化CrystalPacketDelegateAdapter
    _observer = new crystal::rtc::CrystalPacketDelegateAdapter(self);
    
    //注册_observer
    registerPacketObserver(_observer);
}

@end
