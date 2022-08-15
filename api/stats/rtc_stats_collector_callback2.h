#ifndef API_STATS_RTC_STATS_COLLECTOR_CALLBACK_H2_
#define API_STATS_RTC_STATS_COLLECTOR_CALLBACK_H2_
#include "api/stats/rtc_stats_collector_callback.h"
#include "api/stats/rtcstats_objects.h"
#include <map>
#include <string>
#include <mutex>

enum NetworkQuality { 
  kPoor,                  // 质量很差
  kNormal,                // 质量一般
  kGood,                  // 资料不错
  kExcellent,             // 质量很好
  kUnknown                // 状态未知
};

// 编码器参数
struct RTCCodec {
  uint32_t payload_type = 0;			   // 有效载荷类型
  std::string mime_type = "";			   // 编码器类型 h264、vp9...    opus...
  uint32_t clock_rate = 0;				   // 时钟频率
  uint32_t channels = 0;				   // 通道类型
};

// 上行质量评估
struct RTCOutBoundNetworkQuality {
  double round_trip_time = 0;			   // rtt
  double fraction_lost = 0;				   // 丢包率
};

// 音频上行
struct RTCAudioOutBandStats {
  // RTCOutboundRTPStreamStats
  uint32_t ssrc = 0;                       // ssrc
  uint64_t packets_sent = 0;			   // 发包总数
  uint64_t bytes_sent = 0;				   // 发送比特率总数
  uint64_t retransmitted_packets_sent = 0; // 重发包数量
  uint64_t header_bytes_sent = 0;		   // RTP 标头和填充字节的总数
  uint64_t retransmitted_bytes_sent = 0;   // 重传的字节总数，仅包括有效载荷字节
  uint32_t nack_count;					   // NACK总数
  double audio_volume = 0;                 //  [0、1]

  // RTCAudioSourceStats
  double audio_level = 0;				   // 音频等级
  double total_audio_energy = 0;		   // 音频能量
  double echo_return_loss = 0;			   // 回声返回损失,仅当 MediaStreamTrack 来自应用了回声消除的麦克风时才存在。 以分贝计算
  double echo_return_loss_enhancement = 0; // 回波回波损耗增强,仅当 MediaStreamTrack 来自应用了回声消除的麦克风时才存在。 以分贝计算

  // RTCCodecStats 
  RTCCodec audio_codec;					    // 编码器

  // RTCRemoteInboundRtpStreamStats
  RTCOutBoundNetworkQuality quailty_parameter;// 网络质量

  double jitter = 0;						  // 对端接收抖动
  double total_round_trip_time = 0;			  // 总计RTT
  double round_trip_time_measurements = 0;    // RTT计算次数

  // calculate
  NetworkQuality network_quality = kNormal;		
  double bitrate_send = 0;					 // 发送总数
  double retransmitted_bitrate_send = 0;	 // 重发码率
};

// 视频上行
struct RTCVideoOutBandStats {
  // RTCOutboundRTPStreamStats
  uint32_t ssrc = 0;                         // ssrc
  uint32_t frame_width = 0;                  // 宽
  uint32_t frame_height = 0;                 // 高
  uint32_t frames_encoded = 0;               // 编码帧数
  uint32_t frames_sent = 0;                  // 发送帧数
  uint32_t fir_count = 0;					 // 接收到fir关键帧请求总数			
  uint32_t pli_count = 0;					 // 接收到pli关键帧请求总数
  uint32_t nack_count = 0;					 // nack总数
  uint64_t packets_sent = 0;				 // 发包总数
  uint64_t bytes_sent = 0;					 // 发送字节数
  uint64_t qp_sum = 0;						 // qp和
  uint64_t retransmitted_packets_sent = 0;   // 重发包数量
  uint64_t header_bytes_sent = 0;			 // RTP 标头和填充字节的总数
  uint64_t retransmitted_bytes_sent = 0;	 // 重传的字节总数，仅包括有效载荷字节
  uint32_t key_frames_encoded;				 // 关键帧编码数
  uint32_t huge_frames_sent;				 // huge帧总数,huge帧编码大小至少是帧平均大小的2.5倍
  double total_packet_send_delay = 0;          // 总计发包延迟
  double frames_per_second = 0;                // 发送帧率
  std::string quality_limitation_reason = "";  // 影响质量的原因
  std::string encoder_implementation = "";     // 编码器实现

  // RTCCodecStats
  RTCCodec video_codec;						 // 视频编码器

  // RTCRemoteInboundRtpStreamStats
  RTCOutBoundNetworkQuality quailty_parameter;// 质量参数

  double jitter = 0;                        // 对端接收抖动
  double total_round_trip_time = 0;         // 总计RTT
  double round_trip_time_measurements = 0;  // RTT计算次数
 
 // calculate
  double bitrate_send = 0;                   // 上行码率
  double qp = 0;
  uint64_t target_delay_ms = 0;              // totalPacketSendDelay差 / packetsSent差
  NetworkQuality network_quality = kNormal;
  double retransmitted_bitrate = 0;			 // 重发比特率
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
  double bitrate_send = 0;								// 上行码率KB/s
  uint64_t packets_sent = 0;							// audio + video
  uint64_t packets_lost = 0;							// audio lost + video lost
  RTCOutBoundNetworkQuality quailty_parameter;			// 质量参数
  NetworkQuality network_quality = kNormal;				// 质量评级

  std::map<std::string, RTCAudioOutBandStats> audios; 
  std::map<std::string, RTCVideoOutBandStats> videos;  
};

// 下行质量评估
struct RTCInBoundNetworkQuality {
  double round_trip_time = 0;                                // 下行拿不到真实的rtt，所以拿candidate_pair-》currentRoundTripTime
  double fraction_lost = 0;									 // rtt
};

struct RTCAudioInBandStats {
  // RTCInboundRTPStreamStats
  uint32_t ssrc = 0;										// ssrc
  uint32_t packets_received = 0;							// 收到包总数
  uint64_t fec_packets_received = 0;						// FEC包总数
  uint64_t fec_packets_discarded = 0;						// FEC数据包总数,其中应用程序丢弃了纠错有效负载
  uint64_t bytes_received = 0;								// 接收的总字节数
  uint64_t jitter_buffer_emitted_count = 0;					// 抖动缓冲区的音频样本总数
  uint64_t total_samples_received = 0;						// 样本总数
  uint64_t concealed_samples = 0;							// 隐藏样本的样本总数
  uint64_t silent_concealed_samples = 0;					// 插入的“无声”隐藏样本总数,播放静音样本会导致静音或舒适噪
  uint64_t concealment_events = 0;							// 隐藏事件的数量
  uint64_t inserted_samples_for_deceleration = 0;			// 插入样本总数，视频减速
  uint64_t removed_samples_for_acceleration = 0;			// 删除样本总数，视频加速
  uint64_t header_bytes_received = 0;						// RTP标头和填充字节的总数。这不包括传输层标头的大小
  uint64_t packets_discarded = 0;							// 由于迟到或早到而被抖动缓冲区丢弃的RTP数据包的累积数量
  double jitter = 0;										// 数据包抖动
  double jitter_buffer_delay = 0;							// 抖动延迟总和
  double packets_lost = 0;									// 丢包数
  double delay_estimate_ms = 0;								// 延迟（抖动缓冲延迟+播出延迟
  double audio_level = 0;									// 音频数据获取到的最大振幅，换算出0~9 共10个等级
  double total_audio_energy = 0;							// 总音频能量
  double total_samples_duration = 0;						// 接收音轨的音频持续时间，已接收的所有样本的总持续时间
  double audio_volume = 0; 

  // RTCCodecStats
  RTCCodec audio_codec;

  // Remote Audio Stream
  uint32_t remote_packets_sent = 0;							 // 远端发包数
  uint64_t remote_bytes_sent = 0;							 // 远端发送字节数
  uint64_t remote_reports_sent = 0;						     // 远端发送RTCP SR报告数

  // RTCTrackStats
  uint64_t jitter_buffer_flushes = 0;						// 抖动缓冲区刷新
  uint64_t delayed_packet_outage_samples = 0;				// 延迟数据包中断样本
  uint32_t interruption_count = 0;							// 中断计数
  double relative_packet_arrival_delay = 0;					// 包到达相对延迟
  double jitter_buffer_target_delay = 0;					// jitter buffer相对
  double total_interruption_duration = 0;					// 总中断持续时间

  // calculate
  double bitrate_recv = 0;									 // 下行码率
  double remote_bitrate_send = 0;								 // 远端发送码率
  RTCInBoundNetworkQuality quailty_parameter;				 // 质量参数
  NetworkQuality network_quality = kNormal;					 // 质量
  double audio_caton_ms = 0;								 // 音频卡顿时长
  double audio_delay = 0;									 // 音频延迟
};

struct RTCVideoInBandStats {
  // RTCInboundRTPStreamStats
  uint32_t ssrc = 0;										 // ssrc	
  uint32_t packets_received = 0;							 // 接收总数
  uint32_t frames_received = 0;								 // 接收帧数
  uint32_t frames_dropped = 0;								 // 丢弃帧数
  uint32_t frames_decoded = 0;								 // 解码帧数
  uint32_t frame_width = 0;									 // 宽
  uint32_t frame_height = 0;								 // 高
  uint32_t fir_count = 0;									 // 发送fir关键帧总数
  uint32_t pli_count = 0;									 // 发送pli关键帧总数
  uint32_t nack_count = 0;									 // 发送Nack总数
  uint32_t qp_sum = 0;										 // qp总数
  uint32_t key_frames_decoded = 0;							 // 关键帧编码总数
 
  uint32_t render_delay_ms = 0;								 // 渲染延迟
  uint32_t target_delay_ms = 0;							     // 目标延迟
  uint64_t bytes_received = 0;								 // 接收字节数
  uint64_t header_bytes_received = 0;						 // RTP标头和填充字节的总数。这不包括传输层标头的大小
  uint64_t jitter_buffer_emitted_count = 0;					 // 缓冲区的视频帧的总数
  double jitter = 0;										 // 数据包抖动
  double jitter_buffer_delay = 0;							 // 抖动延迟总和
  double packets_lost = 0;									 // 丢包总数
  double frames_per_second = 0;								 // 最后一秒帧率
  double total_decode_time = 0;								 // framesDecoded 帧所花费的总秒数
  double total_inter_frame_delay = 0;						 // 连续解码的帧之间的帧间延迟总和
  double total_squared_inter_frame_delay = 0;				 // 连续解码的帧之间的平方帧间延迟的总和
  double total_caton_count = 0;								 // 视频卡顿计数
  double total_caton_delay_ms = 0;							 // 视频时长
  std::string decoder_implementation;						 // 解码器实现

  // RTCCodecStats 
  RTCCodec video_codec;										 // 解码器参数

  // RTCMediaStreamTrackStats
  uint32_t freeze_count = 0;								 // 冻结总数
  uint32_t pause_count = 0;									 // 暂停总数
  double total_freezes_duration = 0;					     // 冻结时长
  double total_pauses_duration = 0;							 // 暂停时长
  double total_frames_duration = 0;							 // 总计帧时长
  double sum_squared_frame_durations = 0;                    // 帧持续时间平方和

  // calculate
  double bitrate_recv = 0;									 // 下行码率
  double qp = 0;											 // qp值
  double video_caton_ms = 0;								 // 视频卡顿
  RTCInBoundNetworkQuality quailty_parameter;				 // 质量参数
  NetworkQuality network_quality = kNormal;					 // 质量
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
  mutable std::mutex mutex_;
}; 

// 下行统计
class RTCInBoundStatsCollectorCallBack : public RTCStatsCollectorCallback {
 public:
   RTCInBoundStatsCollectorCallBack() = default;
	 ~RTCInBoundStatsCollectorCallBack() override = default;

   void SetProbatorSsrc(uint32_t probator_ssrc);
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
  uint32_t probator_ssrc_;
  mutable std::mutex mutex_;

};

RTC_EXPORT rtc::scoped_refptr<webrtc::RTCOutBoundStatsCollectorCallBack>
CreatOutBoundStatsCollectorCallBack();

RTC_EXPORT rtc::scoped_refptr<webrtc::RTCInBoundStatsCollectorCallBack>
CreatInBoundStatsCollectorCallBack();

}

#endif