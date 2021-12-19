//
//  RTCCrystalPacketObserver.h
//  AFNetworking
//
//  Created by 符吉胜 on 2020/9/13.
//

#import <Foundation/Foundation.h>
#import "RTCMacros.h"

NS_ASSUME_NONNULL_BEGIN

RTC_OBJC_EXPORT
@interface RTC_OBJC_TYPE(RTCPacket) : NSObject
{
    @public
    const unsigned char **buffer;
    size_t *size;
}

@end

@protocol RTC_OBJC_TYPE(RTCCrystalPacketObserverDelegate)<NSObject>

- (BOOL)onSendAudioPacket:(RTC_OBJC_TYPE(RTCPacket) *)packet;
- (BOOL)onSendVideoPacket:(RTC_OBJC_TYPE(RTCPacket) *)packet;
- (BOOL)onReceiveAudioPacket:(RTC_OBJC_TYPE(RTCPacket) *)packet;
- (BOOL)onReceiveVideoPacket:(RTC_OBJC_TYPE(RTCPacket) *)packet;

@end

RTC_OBJC_EXPORT
@interface RTC_OBJC_TYPE(RTCCrystalPacketObserver) : NSObject

@property(nonatomic, weak) id<RTC_OBJC_TYPE(RTCCrystalPacketObserverDelegate)> delegate;

- (void)addPacketObserverDelegate:(id<RTCCrystalPacketObserverDelegate>)delegate;

@end

NS_ASSUME_NONNULL_END
