#ifndef API_STATS_RTC_STATS_COLLECTOR_CALLBACK_H2_
#define API_STATS_RTC_STATS_COLLECTOR_CALLBACK_H2_
#include "api/stats/rtc_stats_collector_callback.h"
#include "api/stats/rtcstats_objects.h"
#include <map>
#include <string>

struct RTCAudioOutBandStats {
  uint32_t ssrc = 0;                        // ssrc
  double bitrate_send = 0;                  // 上行码率

  uint64_t packets_sent = 0;
  uint64_t bytes_sent = 0;

  double audio_level = 0;
  double total_audio_energy = 0;

  double fraction_lost = 0;                 // 上行丢包率
  double round_trip_time = 0;               // rtt
};

struct RTCVideoOutBandStats {
  uint32_t ssrc = 0;                        // ssrc
  double bitrate_send = 0;                  // 上行码率
  double retransmitted_bitrate = 0;         // 重发比特率
  double frames_per_second = 0;             // 发送帧率
  uint32_t frame_width = 0;                 // 宽
  uint32_t frame_height = 0;                // 高

  uint32_t frames_encoded = 0;              // 编码帧数
  uint32_t frames_sent = 0;                 // 发送帧数
  double total_packet_send_delay = 0;       // 总计发包延迟
  std::string encoder_implementation = "";
  std::string quality_limitation_reason = ""; // 影响质量的原因
  uint32_t fir_count = 0;
  uint32_t pli_count = 0;
  uint32_t nack_count = 0;
  uint64_t qp_sum = 0;
  double qp;
  uint64_t target_delay_ms = 0;              // totalPacketSendDelay差 / packetsSent差
  uint64_t retransmitted_bytes_sent = 0;
  uint64_t packets_sent = 0;
  uint64_t bytes_sent = 0;

  double fraction_lost = 0;                  // 上行丢包率
  double round_trip_time = 0;                // rtt
};
// 上行
struct RTCOutBandStats {
  int64_t timestamp = 0;
  double bitrate_send = 0;                   // 上行码率KB/s

  uint64_t bytes_sent = 0;                   // transport bytesSent
  uint64_t packets_sent = 0;                 // transport packetsSent

  std::map<uint32_t, RTCAudioOutBandStats> audios;  // bitrate
  std::map<uint32_t, RTCVideoOutBandStats> videos;  //

};

struct RTCAudioInBandStats {
  uint32_t ssrc = 0;
  double bitrate_recv = 0;                    // 下行码率
  double fraction_lost = 0;                   // 丢包率
  double delay_estimate_ms;  
  double jitter = 0;
  double packets_lost = 0;
  uint32_t packets_received = 0;
  uint64_t fec_packets_received = 0;
  uint64_t fec_packets_discarded = 0;
  uint64_t bytes_received = 0;
  double jitter_buffer_delay = 0;
  uint64_t jitter_buffer_emitted_count = 0;
  uint64_t total_samples_received = 0;
  uint64_t concealed_samples = 0;
  uint64_t silent_concealed_samples = 0;
  uint64_t concealment_events = 0;
  uint64_t inserted_samples_for_deceleration = 0;
  uint64_t removed_samples_for_acceleration = 0;
  double audio_level = 0;
  double total_audio_energy = 0;
  double total_samples_duration = 0;
  double audio_caton_ms = 0;
};

struct RTCVideoInBandStats {
  uint32_t ssrc = 0;
  double bitrate_recv = 0;      // 下行码率
  double fraction_lost = 0;     // 丢包率

  double jitter = 0;
  double packets_lost = 0;
  uint32_t packets_received = 0;
  uint64_t bytes_received = 0;
  int32_t frames_received = 0;
  uint32_t frame_width = 0;
  uint32_t frame_height = 0;
  double frames_per_second = 0;
  uint32_t frames_decoded = 0;
  uint32_t key_frames_decoded = 0;
  uint32_t frames_dropped = 0;
  double total_decode_time = 0;
  double total_inter_frame_delay = 0;
  double total_squared_inter_frame_delay = 0;
  uint32_t fir_count = 0;
  uint32_t pli_count = 0;
  uint32_t nack_count = 0;
  uint32_t qp_sum = 0;
  double qp = 0;

  uint32_t render_delay_ms = 0;
  uint32_t target_delay_ms = 0;
  double video_caton_ms = 0;

  double jitter_buffer_delay = 0;
  uint64_t jitter_buffer_emitted_count = 0;

  uint32_t freeze_count = 0;
  double total_freezes_duration = 0;
  double total_pauses_duration = 0;
  double total_frames_duration = 0;
  double sum_squared_frame_durations = 0;

  double total_caton_count = 0;
  double total_caton_delay_ms = 0;
};

// 下行需求
struct RTCInBandStats {
  int64_t timestamp = 0;
  double bitrate_recv = 0;     // 下行码率

  uint64_t bytes_recv = 0;     // transport bytesSent
  uint64_t packets_recv = 0;   // transport packetsSent

  std::map<uint32_t, RTCAudioInBandStats> audios;  // bitrate
  std::map<uint32_t, RTCVideoInBandStats> videos;   //
};

namespace webrtc {

// 上行视频统计
class RTCOutBoundStatsCollectorCallBack : public RTCStatsCollectorCallback {
 public:
  RTCOutBoundStatsCollectorCallBack() = default;
  ~RTCOutBoundStatsCollectorCallBack() override = default;

  RTCOutBandStats GetOutBandStats() const;

 protected:
  void OnStatsDelivered(
      const rtc::scoped_refptr<const webrtc::RTCStatsReport>& report) override;
 
 private:
  void Initialization();
  void CalcStats();

 private:
  std::vector<const webrtc::RTCOutboundRTPStreamStats*> outbound_audio_map;
  std::vector<const webrtc::RTCOutboundRTPStreamStats*> outbound_video_map;

  std::map<std::string, const webrtc::RTCRemoteInboundRtpStreamStats*> remote_inbound_audio_map;
  std::map<std::string, const webrtc::RTCRemoteInboundRtpStreamStats*> remote_inbound_video_map;

  std::map<std::string, const webrtc::RTCAudioSourceStats*> audio_mediasource_map;
  std::map<std::string, const webrtc::RTCVideoSourceStats*> video_mediasource_map;

  std::map<std::string, const webrtc::RTCCodecStats* > audio_codec_map;
  std::map<std::string, const webrtc::RTCCodecStats* > video_codec_map;
  std::map<std::string, const webrtc::RTCTransportStats*> transport_map;

  RTCOutBandStats stats_;
}; 

// 下行统计
class RTCInBoundStatsCollectorCallBack : public RTCStatsCollectorCallback {
 public:
   RTCInBoundStatsCollectorCallBack() = default;
	 ~RTCInBoundStatsCollectorCallBack() override = default;

   RTCInBandStats GetInBandStats() const;

 protected:
  void OnStatsDelivered(const rtc::scoped_refptr<const webrtc::RTCStatsReport>& report) override;

private:
  void Initialization();
 void CalcStats();

 private:
  std::vector<const webrtc::RTCInboundRTPStreamStats*> inbound_audio_map;
  std::vector<const webrtc::RTCInboundRTPStreamStats*> inbound_video_map;

  std::map<std::string, const webrtc::RTCRemoteOutboundRtpStreamStats*> remote_outbound_audio_map;
  std::map<std::string, const webrtc::RTCRemoteOutboundRtpStreamStats*> remote_outbound_video_map;

  std::map<std::string, const webrtc::RTCAudioSourceStats*> audio_mediasource_map;
  std::map<std::string, const webrtc::RTCVideoSourceStats*> video_mediasource_map;

  std::map<std::string, const webrtc::RTCMediaStreamTrackStats*> track_map;

  std::map<std::string, const webrtc::RTCCodecStats*> audio_codec_map;
  std::map<std::string, const webrtc::RTCCodecStats*> video_codec_map;

  std::map<std::string, const webrtc::RTCTransportStats*> transport_map;

  RTCInBandStats stats_;
};

RTC_EXPORT rtc::scoped_refptr<webrtc::RTCOutBoundStatsCollectorCallBack>
CreatOutBoundStatsCollectorCallBack();

RTC_EXPORT rtc::scoped_refptr<webrtc::RTCInBoundStatsCollectorCallBack>
CreatInBoundStatsCollectorCallBack();

}

#endif