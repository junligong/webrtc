#ifndef API_STATS_RTC_STATS_COLLECTOR_CALLBACK_H2_
#define API_STATS_RTC_STATS_COLLECTOR_CALLBACK_H2_
#include "api/stats/rtc_stats_collector_callback.h"
#include "api/stats/rtcstats_objects.h"
#include <map>
#include <string>

enum NetworkQuality { 
  kPoor,                  // 质量很差
  kNormal,                // 质量一般
  kGood,                  // 资料不错
  kExcellent,             // 质量很好
  kUnknown                // 状态未知
};

// 编码器参数
struct RTCCodec {
  uint32_t payload_type = 0;
  std::string mime_type = "";
  uint32_t clock_rate = 0;
  uint32_t channels = 0;
};

// 上行质量评估
struct RTCOutBoundNetworkQuality {
  double round_trip_time = 0;
  double fraction_lost = 0;
};

// 音频上行
struct RTCAudioOutBandStats {
  // RTCOutboundRTPStreamStats
  uint32_t ssrc = 0;                       // ssrc
  uint64_t packets_sent = 0;
  uint64_t bytes_sent = 0;
  double audio_volume = 0;                 //  [0、1]

  // RTCAudioSourceStats
  double audio_level = 0;
  double total_audio_energy = 0;

  // RTCCodecStats
  RTCCodec audio_codec;

  // RTCRemoteInboundRtpStreamStats
  RTCOutBoundNetworkQuality quailty_parameter;

  // calculate
  NetworkQuality network_quality = kNormal;
  double bitrate_send = 0;
};

// 视频上行
struct RTCVideoOutBandStats {
  // RTCOutboundRTPStreamStats
  uint32_t ssrc = 0;                          // ssrc
  uint32_t frame_width = 0;                   // 宽
  uint32_t frame_height = 0;                  // 高
  uint32_t frames_encoded = 0;                // 编码帧数
  uint32_t frames_sent = 0;                   // 发送帧数
  uint32_t fir_count = 0;
  uint32_t pli_count = 0;
  uint32_t nack_count = 0;
  uint64_t packets_sent = 0;
  uint64_t bytes_sent = 0;
  uint64_t qp_sum = 0;
  double total_packet_send_delay = 0;          // 总计发包延迟
  double retransmitted_bitrate = 0;            // 重发比特率
  double frames_per_second = 0;                // 发送帧率
  std::string quality_limitation_reason = "";  // 影响质量的原因
  std::string encoder_implementation = "";

  // RTCCodecStats
  RTCCodec video_codec;

  // RTCRemoteInboundRtpStreamStats
  RTCOutBoundNetworkQuality quailty_parameter;
 
 // calculate
  double bitrate_send = 0;                   // 上行码率
  double qp = 0;
  uint64_t target_delay_ms = 0;              // totalPacketSendDelay差 / packetsSent差
  uint64_t retransmitted_bytes_sent = 0;
  NetworkQuality network_quality = kNormal;

};

// 上行总计
struct RTCOutBandStats {
  // RTCRTPStreamStats 
  int64_t timestamp = 0;
  // RTCIceCandidatePairStats
  double available_outgoing_bitrate = 0;                // 下行带宽评估
  // RTCTransportStats
  uint64_t bytes_sent = 0;                              // transport bytesSent

  // calculate
  double bitrate_send = 0;    // 上行码率KB/s
  uint64_t packets_sent = 0;  // audio + video
  uint64_t packets_lost = 0;  // audio lost + video lost
  RTCOutBoundNetworkQuality quailty_parameter;
  NetworkQuality network_quality = kNormal;

  std::map<std::string, RTCAudioOutBandStats> audios; 
  std::map<std::string, RTCVideoOutBandStats> videos;  

};

// 下行质量评估
struct RTCInBoundNetworkQuality {
  double round_trip_time = 0;                                // 下行拿不到真实的rtt，所以拿candidate_pair-》currentRoundTripTime
  double fraction_lost = 0;
};

struct RTCAudioInBandStats {
  // RTCInboundRTPStreamStats
  uint32_t ssrc = 0;
  uint32_t packets_received = 0;
  uint64_t fec_packets_received = 0;
  uint64_t fec_packets_discarded = 0;
  uint64_t bytes_received = 0;
  uint64_t jitter_buffer_emitted_count = 0;
  uint64_t total_samples_received = 0;
  uint64_t concealed_samples = 0;
  uint64_t silent_concealed_samples = 0;
  uint64_t concealment_events = 0;
  uint64_t inserted_samples_for_deceleration = 0;
  uint64_t removed_samples_for_acceleration = 0;
  double jitter = 0;
  double jitter_buffer_delay = 0;
  double packets_lost = 0;
  double delay_estimate_ms = 0;  
  double audio_level = 0;
  double total_audio_energy = 0;
  double total_samples_duration = 0;
  double audio_volume = 0;  // [0、1]

  // RTCCodecStats
  RTCCodec audio_codec;

  // calculate
  double bitrate_recv = 0;  // 下行码率
  RTCInBoundNetworkQuality quailty_parameter;
  NetworkQuality network_quality = kNormal;
  double audio_caton_ms = 0;
};

struct RTCVideoInBandStats {
  // RTCInboundRTPStreamStats
  uint32_t ssrc = 0;
  uint32_t packets_received = 0;
  uint32_t frames_received = 0;
  uint32_t fir_count = 0;
  uint32_t pli_count = 0;
  uint32_t nack_count = 0;
  uint32_t qp_sum = 0;
  uint32_t frame_width = 0;
  uint32_t frame_height = 0;
  uint32_t frames_decoded = 0;
  uint32_t key_frames_decoded = 0;
  uint32_t frames_dropped = 0;
  uint32_t render_delay_ms = 0;
  uint32_t target_delay_ms = 0;
  uint64_t bytes_received = 0;
  double jitter = 0;
  double jitter_buffer_delay = 0;
  double packets_lost = 0;
  double frames_per_second = 0;
  double total_decode_time = 0;
  double total_inter_frame_delay = 0;
  double total_squared_inter_frame_delay = 0;
  double total_caton_count = 0;
  double total_caton_delay_ms = 0;

  // RTCCodecStats 
  RTCCodec video_codec;

  // RTCMediaStreamTrackStats
  uint32_t freeze_count = 0;
  double total_freezes_duration = 0;
  double total_pauses_duration = 0;
  double total_frames_duration = 0;
  double sum_squared_frame_durations = 0;

  // calculate
  double bitrate_recv = 0;  // 下行码率
  double qp = 0;
  double video_caton_ms = 0;
  RTCInBoundNetworkQuality quailty_parameter;
  NetworkQuality network_quality = kNormal;
};

// 下行需求
struct RTCInBandStats {
  // RTCRTPStreamStats 
  int64_t timestamp = 0;
  // RTCTransportStats
  uint64_t bytes_recv = 0;     // transport bytesSent
  // calculate
  double bitrate_recv = 0;    // 下行码率
  uint64_t packets_received = 0;  // transport packetsSent
  uint64_t packets_lost = 0;
  RTCInBoundNetworkQuality quailty_parameter;
  NetworkQuality network_quality = kNormal;

  std::map<std::string, RTCAudioInBandStats> audios;  // bitrate
  std::map<std::string, RTCVideoInBandStats> videos;  //
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

  std::map<std::string, const webrtc::RTCCodecStats*> codec_map;
  std::map<std::string, const webrtc::RTCTransportStats*> transport_map;

  std::map<std::string, const webrtc::RTCIceCandidatePairStats*> candidate_pair_map;

  std::map<std::string, const webrtc::RTCMediaStreamTrackStats*> track_map;

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

  std::map<std::string, const webrtc::RTCCodecStats*> codec_map;
  std::map<std::string, const webrtc::RTCTransportStats*> transport_map;

  std::map<std::string, const webrtc::RTCIceCandidatePairStats*> candidate_pair_map;

  std::map<std::string, const webrtc::RTCMediaStreamTrackStats*> track_map;

  RTCInBandStats stats_;
};

RTC_EXPORT rtc::scoped_refptr<webrtc::RTCOutBoundStatsCollectorCallBack>
CreatOutBoundStatsCollectorCallBack();

RTC_EXPORT rtc::scoped_refptr<webrtc::RTCInBoundStatsCollectorCallBack>
CreatInBoundStatsCollectorCallBack();



}

#endif