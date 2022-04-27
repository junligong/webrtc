#include "api/stats/rtc_stats_collector_callback2.h"
#include "api/scoped_refptr.h"
#include "api/stats/rtc_stats_report.h"
#include "rtc_base/ref_count.h"
#include "rtc_base/logging.h"
#include "system_wrappers/include/clock.h"

namespace webrtc {

 template <typename T>
 bool division_operation( double molecular,
                          double denominator,
                          T& result) {
  if (denominator == 0) {
    return false;
  }
  result = std::abs(molecular / denominator);
  return true;
 }

 template <typename T>
 int division_operation(double molecular,
                        double denominator,
                        T& result,
                        double range) {
   if (denominator == 0) {
     return -1;
   }
   T _result = std::abs(molecular / denominator);

   if (_result > range) {
     return -2;
   } else {
     result = _result;
   }
   return 0;
 }

RTCOutBandStats RTCOutBoundStatsCollectorCallBack::GetOutBandStats() const {
  std::lock_guard<std::mutex> guard(mutex_);
  return stats_;
}

void RTCOutBoundStatsCollectorCallBack::OnStatsDelivered(const rtc::scoped_refptr<const webrtc::RTCStatsReport>& report) {

  Initialization();

  // 上行流
  auto outbounds = report->GetStatsOfType<webrtc::RTCOutboundRTPStreamStats>();
  for(const webrtc::RTCOutboundRTPStreamStats* outbound : outbounds) {
    if (*outbound->kind == "video") {
      outbound_video_map.push_back(outbound);
      remote_inbound_video_map[*outbound->remote_id] = nullptr;

    } else {
      outbound_audio_map.push_back(outbound);
      remote_inbound_audio_map[*outbound->remote_id] = nullptr;
    }
  }

  // 编码器
  auto codecs = report->GetStatsOfType<webrtc::RTCCodecStats>();
  for (auto codec : codecs) {
    codec_map[codec->id()] = codec;
  }

  bool hasAudio = !outbound_audio_map.empty();
  bool hasVideo = !outbound_video_map.empty();

  // 不存在视音频无需处理
  if (hasAudio || hasVideo) {

    // 音频流信息
    auto audio_mediasources = report->GetStatsOfType<webrtc::RTCAudioSourceStats>();
    for (auto audio_mediasource : audio_mediasources) {
      audio_mediasource_map[audio_mediasource->id()] = audio_mediasource;
    }

     // 视频流信息
    auto video_mediasources = report->GetStatsOfType<webrtc::RTCVideoSourceStats>();
    for (auto video_mediasource : video_mediasources) {
      video_mediasource_map[video_mediasource->id()] = video_mediasource;
    }

    // 下行数据
    auto remote_outbounds = report->GetStatsOfType<webrtc::RTCRemoteInboundRtpStreamStats>();
    for (auto remote_outbound : remote_outbounds) {
      if (*remote_outbound->kind == "video") {
        auto iter = remote_inbound_video_map.find(remote_outbound->id());
        if (iter != remote_inbound_video_map.end()) {
          iter->second = remote_outbound;
        }
      } else {
        auto iter = remote_inbound_audio_map.find(remote_outbound->id());
        if (iter != remote_inbound_audio_map.end()) {
          iter->second = remote_outbound;
        }
      }
    }

    // 通道信息
    auto transports = report->GetStatsOfType<webrtc::RTCTransportStats>();
    for (auto transport : transports) {
      transport_map[transport->id()] = transport;
    }

    // candidate_pair 能获取到rtt、带宽评估
    auto candidate_pairs = report->GetStatsOfType<webrtc::RTCIceCandidatePairStats>();
    for (auto candidate_pair : candidate_pairs) {
      if (*candidate_pair->state == "succeeded"){
        candidate_pair_map[candidate_pair->id()] = candidate_pair;
      }
    }

    // 媒体轨道
    auto media_tracks = report->GetStatsOfType<webrtc::RTCMediaStreamTrackStats>();
    for (auto media_track : media_tracks) {
      track_map[media_track->id()] = media_track;
    }
  }
  CalcStats();
}

void RTCOutBoundStatsCollectorCallBack::Initialization() {
  outbound_audio_map.clear();
  outbound_video_map.clear();
  remote_inbound_audio_map.clear();
  remote_inbound_video_map.clear();
  audio_mediasource_map.clear();
  video_mediasource_map.clear();
  codec_map.clear();
  transport_map.clear();
  candidate_pair_map.clear();
  track_map.clear();
}

void RTCOutBoundStatsCollectorCallBack::CalcStats() {

  RTCOutBandStats stats;
  // 读取带宽、rtt
  if (!candidate_pair_map.empty() && candidate_pair_map.begin()->second) {
    stats.available_outgoing_bitrate = candidate_pair_map.begin()->second->available_outgoing_bitrate.ValueOrDefault(0) / 1000.0;
    stats.quailty_parameter.round_trip_time = candidate_pair_map.begin()->second->current_round_trip_time.ValueOrDefault(0);
  }
  // 遍历上行音频流
  for (auto audio_outbound : outbound_audio_map) {
    stats.timestamp = audio_outbound->timestamp_us();
    
    // 这里是为了上层的producer、consumer能对应起来
    if ((*audio_outbound->track_id).empty()) {
      continue;
    }
    std::string track_identifier;
    auto track_iter = track_map.find(*audio_outbound->track_id);
    if (track_iter == track_map.end()) {
      continue;
    }
    track_identifier = *track_iter->second->track_identifier;

    // 初始化常规参数
    RTCAudioOutBandStats audio_outband_stats;
    audio_outband_stats.ssrc = audio_outbound->ssrc.ValueOrDefault(0);

    audio_outband_stats.packets_sent = audio_outbound->packets_sent.ValueOrDefault(0);
    audio_outband_stats.bytes_sent = audio_outbound->bytes_sent.ValueOrDefault(0);
    audio_outband_stats.audio_volume = audio_outbound->audio_volume.ValueOrDefault(0);
    //  媒体信息
    if (audio_outbound->media_source_id.is_defined()) {
      auto media_iter =
          audio_mediasource_map.find(*audio_outbound->media_source_id);
      if (media_iter != audio_mediasource_map.end()) {
        audio_outband_stats.audio_level =
            media_iter->second->audio_level.ValueOrDefault(0);
        audio_outband_stats.total_audio_energy =
            media_iter->second->total_audio_energy.ValueOrDefault(0);
      }
    }

    // 编码器
    auto audio_codec = codec_map.find(*audio_outbound->codec_id);
    if (audio_codec != codec_map.end() && audio_codec->second) {
      audio_outband_stats.audio_codec.payload_type = audio_codec->second->payload_type.ValueOrDefault(0);
      audio_outband_stats.audio_codec.mime_type =
          *audio_codec->second->mime_type;
      audio_outband_stats.audio_codec.clock_rate =
          audio_codec->second->clock_rate.ValueOrDefault(0);
      audio_outband_stats.audio_codec.channels =
          audio_codec->second->channels.ValueOrDefault(0);
    }

    // 远端信息 rtt等
    if (audio_outbound->remote_id.is_defined()) {
      auto remote_iter =
          remote_inbound_audio_map.find(*audio_outbound->remote_id);
      if (remote_iter != remote_inbound_audio_map.end()) {
        audio_outband_stats.quailty_parameter.fraction_lost =
            remote_iter->second->fraction_lost.ValueOrDefault(0);
        audio_outband_stats.quailty_parameter.round_trip_time =
            remote_iter->second->round_trip_time.ValueOrDefault(0);
      }
    }

    // 计算发送码率
    if (stats_.timestamp > 0) {
      auto audio_iter = stats_.audios.find(track_identifier);
      if (audio_iter != stats_.audios.end()) {

        division_operation( (audio_outband_stats.bytes_sent - audio_iter->second.bytes_sent) *1000.0,  
                             stats.timestamp - stats_.timestamp,
                             audio_outband_stats.bitrate_send );
      }
      // 丢包 = 发包差 * 丢包率
      stats.packets_lost += std::abs(int(audio_outband_stats.packets_sent - audio_iter->second.packets_sent)) *
                            audio_outband_stats.quailty_parameter.fraction_lost;
    }
    stats.packets_sent += audio_outband_stats.packets_sent;
    stats.audios[track_identifier] = audio_outband_stats;
  }

   for (auto video_outbound : outbound_video_map) {
    stats.timestamp = video_outbound->timestamp_us();

    // 这里是为了上层的producer、consumer能对应起来
    if ((*video_outbound->track_id).empty()) {
      continue;
    }
    std::string track_identifier;
    auto track_iter = track_map.find(*video_outbound->track_id);
    if (track_iter == track_map.end()) {
      continue;
    }
    track_identifier = *track_iter->second->track_identifier;

    if (!(*video_outbound->rid).empty()) {
      track_identifier += *video_outbound->rid;
    }

    // 读取常规参数
    RTCVideoOutBandStats video_outband_stats;
    video_outband_stats.ssrc = video_outbound->ssrc.ValueOrDefault(0);
    video_outband_stats.frame_width = video_outbound->frame_width.ValueOrDefault(0);
    video_outband_stats.frame_height = video_outbound->frame_height.ValueOrDefault(0);
    video_outband_stats.frames_encoded = video_outbound->frames_encoded.ValueOrDefault(0);
    video_outband_stats.frames_sent = video_outbound->frames_sent.ValueOrDefault(0);
    video_outband_stats.fir_count = video_outbound->fir_count.ValueOrDefault(0);
    video_outband_stats.pli_count = video_outbound->pli_count.ValueOrDefault(0);
    video_outband_stats.nack_count = video_outbound->nack_count.ValueOrDefault(0);
    video_outband_stats.packets_sent = video_outbound->packets_sent.ValueOrDefault(0);
    video_outband_stats.bytes_sent = video_outbound->bytes_sent.ValueOrDefault(0);
    video_outband_stats.qp_sum = video_outbound->qp_sum.ValueOrDefault(0);
    video_outband_stats.total_packet_send_delay = video_outbound->total_packet_send_delay.ValueOrDefault(0);
    video_outband_stats.retransmitted_bytes_sent = video_outbound->retransmitted_bytes_sent.ValueOrDefault(0);
    video_outband_stats.frames_per_second = video_outbound->frames_per_second.ValueOrDefault(0);
    video_outband_stats.quality_limitation_reason = video_outbound->quality_limitation_reason.ValueOrDefault("");
    video_outband_stats.encoder_implementation = video_outbound->encoder_implementation.ValueOrDefault("");
    
    // 编码器
    auto video_codec = codec_map.find(*video_outbound->codec_id);
    if (video_codec != codec_map.end() && video_codec->second) {
      video_outband_stats.video_codec.payload_type =
          video_codec->second->payload_type.ValueOrDefault(0);
      video_outband_stats.video_codec.mime_type =
          *video_codec->second->mime_type;
      video_outband_stats.video_codec.clock_rate =
          video_codec->second->clock_rate.ValueOrDefault(0);
      video_outband_stats.video_codec.channels =
          video_codec->second->channels.ValueOrDefault(0);
    }

    // 计算rtt、丢包
    if (video_outbound->remote_id.is_defined()) {
      auto romote_iter = remote_inbound_video_map.find(*video_outbound->remote_id);
      if (romote_iter != remote_inbound_video_map.end()) {
        video_outband_stats.quailty_parameter.fraction_lost = romote_iter->second->fraction_lost.ValueOrDefault(0);
        video_outband_stats.quailty_parameter.round_trip_time = romote_iter->second->round_trip_time.ValueOrDefault(0);
      }
    }
    //  计算发送码率、发包延迟、qp值
    video_outband_stats.target_delay_ms = 0;
    if (stats_.timestamp) {
      auto video_iter = stats_.videos.find(track_identifier);
      if (video_iter != stats_.videos.end()) {
        division_operation( (video_outband_stats.bytes_sent - video_iter->second.bytes_sent) *1000.0, 
                            (stats.timestamp - stats_.timestamp),
                             video_outband_stats.bitrate_send);
        
        division_operation( video_outband_stats.qp_sum - video_iter->second.qp_sum,
                            video_outband_stats.frames_encoded - video_iter->second.frames_encoded,
                            video_outband_stats.qp );


        division_operation( (video_outband_stats.retransmitted_bytes_sent - video_iter->second.retransmitted_bytes_sent) * 1000.0, 
                             stats.timestamp - stats_.timestamp, 
                             video_outband_stats.retransmitted_bitrate);

        division_operation( (video_outband_stats.total_packet_send_delay - video_iter->second.total_packet_send_delay) * 1000.0, 
                            video_outband_stats.packets_sent - video_iter->second.packets_sent, 
                            video_outband_stats.target_delay_ms);

        // 计算发送帧率
        if (video_outband_stats.frames_per_second == 0) {
          division_operation( (video_outband_stats.frames_sent - video_iter->second.frames_sent) * 1000000.0,
                               stats.timestamp - stats_.timestamp,
                               video_outband_stats.frames_per_second);

          video_outband_stats.frames_per_second = round(video_outband_stats.frames_per_second);
        }

        // 丢包 = 发包差 * 丢包率
        stats.packets_lost = stats.packets_lost + std::abs(int(video_outband_stats.packets_sent - video_iter->second.packets_sent)) *
            video_outband_stats.quailty_parameter.fraction_lost;
      }
    }

    stats.packets_sent += video_outband_stats.packets_sent;
    stats.videos[track_identifier] = video_outband_stats;
  }

   // 计算transport发送比特率、视音频总计丢包率
   for (auto iter = transport_map.begin(); iter != transport_map.end(); ++iter){
     stats.bytes_sent += iter->second->bytes_sent.ValueOrDefault(0);
   }

   stats.bitrate_send = 0;
   if (stats_.bytes_sent > 0) {

     division_operation( (stats.bytes_sent - stats_.bytes_sent) * 1000.0, 
                         stats.timestamp - stats_.timestamp, 
                         stats.bitrate_send );

     int flag = division_operation( stats.packets_lost,
                                    std::abs(double(stats.packets_sent - stats_.packets_sent)),
                                    stats.quailty_parameter.fraction_lost,
                                    1);
     if (flag < 0) {
       RTC_LOG(LERROR) << "calc outboundstats fraction_lost error, error code:" << flag;
     }
   }
  std::lock_guard<std::mutex> guard(mutex_);
  stats_ = stats;
}

void RTCInBoundStatsCollectorCallBack::SetProbatorSsrc(uint32_t probator_ssrc) {
  probator_ssrc_ = probator_ssrc;
}

RTCInBandStats RTCInBoundStatsCollectorCallBack::GetInBandStats() const {
  std::lock_guard<std::mutex> guard(mutex_);
  return stats_;
}

void RTCInBoundStatsCollectorCallBack::OnStatsDelivered(const rtc::scoped_refptr<const webrtc::RTCStatsReport>& report) {

  Initialization();
  // 下行
  auto intbounds = report->GetStatsOfType<webrtc::RTCInboundRTPStreamStats>();
  for (const webrtc::RTCInboundRTPStreamStats* intbound : intbounds) {
    if (*intbound->kind == "audio") {
      inbound_audio_map.push_back(intbound);
      remote_outbound_audio_map.insert(std::make_pair(*intbound->remote_id, nullptr));
    } else {
      inbound_video_map.push_back(intbound);
      remote_outbound_video_map.insert(std::make_pair(*intbound->remote_id, nullptr));
    }
  }
  // 编码器
  auto codecs = report->GetStatsOfType<webrtc::RTCCodecStats>();
  for (auto codec : codecs) {
    codec_map[codec->id()] = codec;
  }

  bool hasAudio = !inbound_audio_map.empty();
  bool hasVideo = !inbound_video_map.empty();
  if (hasAudio || hasVideo){

    // 音频资源
    auto audio_mediasources =
        report->GetStatsOfType<webrtc::RTCAudioSourceStats>();
    for (auto audio_mediasource : audio_mediasources) {
      audio_mediasource_map[audio_mediasource->id()] = audio_mediasource;
    }

    // 视频资源
    auto video_mediasources =
        report->GetStatsOfType<webrtc::RTCVideoSourceStats>();
    for (auto video_mediasource : video_mediasources) {
      video_mediasource_map[video_mediasource->id()] = video_mediasource;
    }

    // 远端上行
    auto remote_outbounds = report->GetStatsOfType<webrtc::RTCRemoteOutboundRtpStreamStats>();
    for (auto remote_outbound : remote_outbounds) {
      if (*remote_outbound->kind == "video") {
        auto iter = remote_outbound_video_map.find(remote_outbound->id());
        if (iter != remote_outbound_video_map.end()) {
          iter->second = remote_outbound;
        }
      } else {
        auto iter = remote_outbound_audio_map.find(remote_outbound->id());
        if (iter != remote_outbound_audio_map.end()) {
          iter->second = remote_outbound;
        }
      }
    }
    // 通道
    auto transports = report->GetStatsOfType<webrtc::RTCTransportStats>();
    for (auto transport : transports) {
      transport_map[transport->id()] = transport;
    }
  }
  // candidate_pair 能找到rtt，下行没有rtt
  auto candidate_pairs = report->GetStatsOfType<webrtc::RTCIceCandidatePairStats>();
  for (auto candidate_pair : candidate_pairs) {
    if (*candidate_pair->state == "succeeded") {
      candidate_pair_map[candidate_pair->id()] = candidate_pair;
    }
  }
  // 媒体轨道
  auto media_tracks =  report->GetStatsOfType<webrtc::RTCMediaStreamTrackStats>();
  for (auto media_track : media_tracks) {
    track_map.insert(std::make_pair(media_track->id(), media_track));
  }

  CalcStats();
}

void RTCInBoundStatsCollectorCallBack::Initialization() {
  inbound_audio_map.clear();
  inbound_video_map.clear();
  remote_outbound_audio_map.clear();
  remote_outbound_video_map.clear();
  audio_mediasource_map.clear();
  video_mediasource_map.clear();
  transport_map.clear();
  track_map.clear();
  codec_map.clear();
}

void RTCInBoundStatsCollectorCallBack::CalcStats() {
  RTCInBandStats stats;
  // 读取rtt
  if (!candidate_pair_map.empty() && candidate_pair_map.begin()->second) {
    stats.quailty_parameter.round_trip_time = candidate_pair_map.begin()->second->current_round_trip_time.ValueOrDefault(0);
  }

  // 下行音频
  for (auto inbound_audio : inbound_audio_map) {
    stats.timestamp = inbound_audio->timestamp_us();

    // 不能匹配producer、consumer则跳过
    if ((*inbound_audio->track_id).empty()) {
      continue;
    }
    std::string track_identifier;
    auto track_iter = track_map.find(*inbound_audio->track_id);
    if (track_iter == track_map.end()) {
      continue;
    }
    track_identifier = *track_iter->second->track_identifier;

    // 常规参数
    RTCAudioInBandStats audio_stats;
    audio_stats.ssrc = inbound_audio->ssrc.ValueOrDefault(0);
    if (probator_ssrc_ == audio_stats.ssrc) {
      continue;
    }
    audio_stats.packets_received = inbound_audio->packets_received.ValueOrDefault(0);
    audio_stats.fec_packets_received = inbound_audio->fec_packets_received.ValueOrDefault(0);
    audio_stats.fec_packets_discarded = inbound_audio->fec_packets_discarded.ValueOrDefault(0);
    audio_stats.bytes_received = inbound_audio->bytes_received.ValueOrDefault(0);
    audio_stats.jitter_buffer_emitted_count = inbound_audio->jitter_buffer_emitted_count.ValueOrDefault(0);
    audio_stats.total_samples_received = inbound_audio->total_samples_received.ValueOrDefault(0);
    audio_stats.concealed_samples = inbound_audio->concealed_samples.ValueOrDefault(0);
    audio_stats.silent_concealed_samples = inbound_audio->silent_concealed_samples.ValueOrDefault(0);
    audio_stats.concealment_events = inbound_audio->concealment_events.ValueOrDefault(0);
    audio_stats.inserted_samples_for_deceleration = inbound_audio->inserted_samples_for_deceleration.ValueOrDefault(0);
    audio_stats.removed_samples_for_acceleration = inbound_audio->removed_samples_for_acceleration.ValueOrDefault(0);

    audio_stats.jitter = inbound_audio->jitter.ValueOrDefault(0);
    audio_stats.jitter_buffer_delay = inbound_audio->jitter_buffer_delay.ValueOrDefault(0);
    audio_stats.packets_lost = inbound_audio->packets_lost.ValueOrDefault(0);
    audio_stats.delay_estimate_ms = inbound_audio->target_delay_ms.ValueOrDefault(0);

    audio_stats.audio_level = inbound_audio->audio_level.ValueOrDefault(0);
    audio_stats.total_audio_energy = inbound_audio->total_audio_energy.ValueOrDefault(0);
    audio_stats.total_samples_duration = inbound_audio->total_samples_duration.ValueOrDefault(0);
    audio_stats.audio_volume = inbound_audio->audio_volume.ValueOrDefault(0);
    audio_stats.quailty_parameter.round_trip_time = stats.quailty_parameter.round_trip_time;

    // 编码器
    auto audio_codec = codec_map.find(*inbound_audio->codec_id);
    if (audio_codec != codec_map.end() && audio_codec->second) {
      audio_stats.audio_codec.payload_type =
          audio_codec->second->payload_type.ValueOrDefault(0);
      audio_stats.audio_codec.mime_type = *audio_codec->second->mime_type;
      audio_stats.audio_codec.clock_rate =
          audio_codec->second->clock_rate.ValueOrDefault(0);
      audio_stats.audio_codec.channels =
          audio_codec->second->channels.ValueOrDefault(0);
    }

    // tarck
    auto audio_track = track_map.find(*inbound_audio->track_id);
    if (audio_track != track_map.end() && audio_track->second) {
      audio_stats.relativePacketArrivalDelay =
          audio_track->second->relative_packet_arrival_delay.ValueOrDefault(0);
    }

    auto audio_item = stats_.audios.find(track_identifier);
    if (audio_item != stats_.audios.end()) {
      //已有数据则合并，相邻数据
      int flag = division_operation( (audio_stats.packets_lost - audio_item->second.packets_lost),
                                      audio_stats.packets_received - audio_item->second.packets_received,
                                      audio_stats.quailty_parameter.fraction_lost,
                                      1);
      if (flag < 0) {
        RTC_LOG(LERROR) << "calc audioinband ssrc:" 
                        << audio_stats.ssrc
                        << " fraction_lost error, error code:" 
                        << flag;
      }

      //码率
      division_operation( (audio_stats.bytes_received - audio_item->second.bytes_received) * 1000,
                           stats.timestamp - stats_.
                           timestamp, audio_stats.bitrate_recv);

      // 音频卡顿----Todo
      audio_stats.audio_caton_ms = audio_item->second.audio_caton_ms;

      division_operation( (audio_stats.total_samples_duration - audio_item->second.total_samples_duration) * 1000.0,
                           audio_stats.total_samples_received - audio_item->second.total_samples_received,
                           audio_stats.audio_caton_ms);

      division_operation( (audio_stats.relativePacketArrivalDelay - audio_item->second.relativePacketArrivalDelay) * 1000.0,
                           audio_stats.packets_received - audio_item->second.packets_received,
                           audio_stats.audio_delay );
    }
    // 统计整体丢包数
    stats.packets_lost += inbound_audio->packets_lost.ValueOrDefault(0);
    stats.packets_received += inbound_audio->packets_received.ValueOrDefault(0);
    stats.audios.insert(std::make_pair(track_identifier, audio_stats));
  }

  // 下行视频
  for (auto inbound_video : inbound_video_map){
    stats.timestamp = inbound_video->timestamp_us();
    // 不能匹配producer、consumer则跳过
    if ((*inbound_video->track_id).empty()) {
      continue;
    }
    std::string track_identifier;
    auto track_iter = track_map.find(*inbound_video->track_id);
    if (track_iter == track_map.end()) {
      continue;
    }
    track_identifier = *track_iter->second->track_identifier;

    // 常规参数
    RTCVideoInBandStats video_stats;
    video_stats.ssrc = inbound_video->ssrc.ValueOrDefault(0);
    if (probator_ssrc_ == video_stats.ssrc) {
      continue;
    }
    video_stats.packets_received = inbound_video->packets_received.ValueOrDefault(0);
    video_stats.frames_received = inbound_video->frames_received.ValueOrDefault(0);
    video_stats.fir_count = inbound_video->fir_count.ValueOrDefault(0);
    video_stats.pli_count = inbound_video->pli_count.ValueOrDefault(0);
    video_stats.nack_count = inbound_video->nack_count.ValueOrDefault(0);
    video_stats.qp_sum = inbound_video->qp_sum.ValueOrDefault(0);
    video_stats.frame_width = inbound_video->frame_width.ValueOrDefault(0);
    video_stats.frame_height = inbound_video->frame_height.ValueOrDefault(0);
    video_stats.frames_decoded = inbound_video->frames_decoded.ValueOrDefault(0);
    video_stats.key_frames_decoded = inbound_video->key_frames_decoded.ValueOrDefault(0);
    video_stats.frames_dropped = inbound_video->frames_dropped.ValueOrDefault(0);
    video_stats.render_delay_ms = inbound_video->render_delay_ms.ValueOrDefault(0);
    video_stats.target_delay_ms = inbound_video->target_delay_ms.ValueOrDefault(0);
    video_stats.bytes_received = inbound_video->bytes_received.ValueOrDefault(0);

    video_stats.jitter = inbound_video->jitter.ValueOrDefault(0);
    video_stats.jitter_buffer_delay =
        inbound_video->jitter_buffer_delay.ValueOrDefault(0);
    video_stats.packets_lost = inbound_video->packets_lost.ValueOrDefault(0);
    video_stats.frames_per_second = inbound_video->frames_per_second.ValueOrDefault(0);
    video_stats.total_decode_time = inbound_video->total_decode_time.ValueOrDefault(0);
    video_stats.total_inter_frame_delay = inbound_video->total_inter_frame_delay.ValueOrDefault(0);
    video_stats.total_squared_inter_frame_delay = inbound_video->total_squared_inter_frame_delay.ValueOrDefault(0);
    video_stats.total_caton_count =inbound_video->total_caton_count.ValueOrDefault(0);
    video_stats.total_caton_delay_ms = inbound_video->total_caton_delay_ms.ValueOrDefault(0);

    // 编码器
    auto video_codec = codec_map.find(*inbound_video->codec_id);
    if (video_codec != codec_map.end() && video_codec->second) {
      video_stats.video_codec.payload_type =
          video_codec->second->payload_type.ValueOrDefault(0);
      video_stats.video_codec.mime_type = *video_codec->second->mime_type;
      video_stats.video_codec.clock_rate =
          video_codec->second->clock_rate.ValueOrDefault(0);
      video_stats.video_codec.channels =
          video_codec->second->channels.ValueOrDefault(0);
    }

    // track信息
    if (track_iter->second) {
      video_stats.freeze_count =
          track_iter->second->freeze_count.ValueOrDefault(0);
      video_stats.total_freezes_duration =
          track_iter->second->total_freezes_duration.ValueOrDefault(0);
      video_stats.total_pauses_duration =
          track_iter->second->total_pauses_duration.ValueOrDefault(0);
      video_stats.total_frames_duration =
          track_iter->second->total_frames_duration.ValueOrDefault(0);
      video_stats.sum_squared_frame_durations =
          track_iter->second->sum_squared_frame_durations.ValueOrDefault(0);
    }
    // 计算
    auto video_item = stats_.videos.find(track_identifier);
    if (video_item != stats_.videos.end()) {

      division_operation( (video_stats.bytes_received - video_item->second.bytes_received) * 1000,
                           stats.timestamp - stats_.timestamp, 
                           video_stats.bitrate_recv);


      division_operation( video_stats.qp_sum - video_item->second.qp_sum, 
                          video_stats.frames_decoded - video_item->second.frames_decoded,
                          video_stats.qp);

      video_stats.video_caton_ms = video_stats.total_caton_delay_ms - video_item->second.total_caton_delay_ms;
      // 计算丢包
      int flag = division_operation( (video_stats.packets_lost - video_item->second.packets_lost),
                                      video_stats.packets_received - video_item->second.packets_received,
                                      video_stats.quailty_parameter.fraction_lost,
                                      1);
      if (flag < 0) {
        RTC_LOG(LERROR) << "calc videoinband ssrc:" 
                        << video_stats.ssrc
                        << " fraction_lost error, error code:" 
                        << flag;
      }
      
    }
    // 统计整体丢包数
    stats.packets_received += inbound_video->packets_received.ValueOrDefault(0);
    stats.packets_lost += inbound_video->packets_lost.ValueOrDefault(0);
    stats.videos.insert(std::make_pair(track_identifier, video_stats));
  }

  for (auto iter = transport_map.begin(); iter != transport_map.end(); ++iter) {
    stats.bytes_recv += iter->second->bytes_received.ValueOrDefault(0);
  }

  stats.bitrate_recv = 0;
  if (stats_.bytes_recv > 0) {

    division_operation( (stats.bytes_recv - stats_.bytes_recv) * 1000.0, 
                         stats.timestamp - stats_.
                         timestamp, stats.bitrate_recv);

    int flag = division_operation( stats.packets_lost - stats_.packets_lost, 
                                   stats.packets_received - stats_.packets_received,
                                   stats.quailty_parameter.fraction_lost,
                                   1);
    if (flag < 0) {
      RTC_LOG(LERROR) << "calc inband fraction_lost error, error code:" << flag;
    }
  }
  std::lock_guard<std::mutex> guard(mutex_);
  stats_ = stats;
}

RTC_EXPORT rtc::scoped_refptr<webrtc::RTCOutBoundStatsCollectorCallBack>
CreatOutBoundStatsCollectorCallBack() {
  return new rtc::RefCountedObject<webrtc::RTCOutBoundStatsCollectorCallBack>();
}

RTC_EXPORT rtc::scoped_refptr<webrtc::RTCInBoundStatsCollectorCallBack>
CreatInBoundStatsCollectorCallBack() {
  return new rtc::RefCountedObject<webrtc::RTCInBoundStatsCollectorCallBack>();
}

}  // namespace webrtc