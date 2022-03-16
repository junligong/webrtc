#include "api/stats/rtc_stats_collector_callback2.h"
#include "api/scoped_refptr.h"
#include "api/stats/rtc_stats_report.h"
#include "rtc_base/ref_count.h"
#include "rtc_base/logging.h"
#include "system_wrappers/include/clock.h"

namespace webrtc {

RTCOutBandStats RTCOutBoundStatsCollectorCallBack::GetOutBandStats() const {
  return stats_;
}

void RTCOutBoundStatsCollectorCallBack::OnStatsDelivered(const rtc::scoped_refptr<const webrtc::RTCStatsReport>& report) {

  //std::string str_report = report->ToJson();
  
  Initialization();

  auto outbounds = report->GetStatsOfType<webrtc::RTCOutboundRTPStreamStats>();
  for(const webrtc::RTCOutboundRTPStreamStats* outbound : outbounds) {
    if (*outbound->kind == "video") {

      outbound_video_map.push_back(outbound);
      //占位
      video_codec_map[*outbound->codec_id] = nullptr;
      remote_inbound_video_map[*outbound->remote_id] = nullptr;

    } else {
      outbound_audio_map.push_back(outbound);
      //占位
      audio_codec_map[*outbound->codec_id] = nullptr;
      remote_inbound_audio_map[*outbound->remote_id] = nullptr;
    }
  }
  bool hasAudio = !outbound_audio_map.empty();
  bool hasVideo = !outbound_video_map.empty();

  if (hasAudio || hasVideo){

    // 目前认为视频统一使用一种编码器，音频统一使用一种编码器
    auto iter_audio = audio_codec_map.begin();
    auto iter_video = video_codec_map.begin();

    auto codecs = report->GetStatsOfType<webrtc::RTCCodecStats>();
    for (auto codec : codecs) {
      if ((!hasAudio || iter_audio->second) && (!hasVideo || iter_video->second)) {
        break;
      }
      //如果有占位数据，更新codec
      if (hasAudio && !iter_audio->second && iter_audio->first == codec->id()) {
        iter_audio->second = codec;
      } else if (hasVideo  && !iter_video->second && iter_video->first == codec->id()) {
        iter_video->second = codec;
      }
    }

    // audio mediasource
    auto audio_mediasources = report->GetStatsOfType<webrtc::RTCAudioSourceStats>();
    for (auto audio_mediasource : audio_mediasources) {
      audio_mediasource_map[audio_mediasource->id()] = audio_mediasource;
    }

     // video mediasource
    auto video_mediasources =
        report->GetStatsOfType<webrtc::RTCVideoSourceStats>();
    for (auto video_mediasource : video_mediasources) {
      video_mediasource_map[video_mediasource->id()] = video_mediasource;
    }

    //下行数据
    auto remote_outbounds = report->GetStatsOfType<webrtc::RTCRemoteInboundRtpStreamStats>();
    for (auto remote_outbound : remote_outbounds){
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

    auto transports = report->GetStatsOfType<webrtc::RTCTransportStats>();
    for (auto transport : transports) {
      transport_map[transport->id()] = transport;
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
  audio_codec_map.clear();
  video_codec_map.clear();
  transport_map.clear();
}

void RTCOutBoundStatsCollectorCallBack::CalcStats() {

  RTCOutBandStats stats;
  //audio_outbound
  for (auto audio_outbound : outbound_audio_map) {
    stats.timestamp = audio_outbound->timestamp_us();
    
    RTCAudioOutBandStats audio_outband_stats;
    audio_outband_stats.ssrc = audio_outbound->ssrc.ValueOrDefault(0);
    audio_outband_stats.bitrate_send = 0;

    audio_outband_stats.packets_sent = audio_outbound->packets_sent.ValueOrDefault(0);
    audio_outband_stats.bytes_sent = audio_outbound->bytes_sent.ValueOrDefault(0);

    if (audio_outbound->remote_id.is_defined()) {
      auto remote_iter =
          remote_inbound_audio_map.find(*audio_outbound->remote_id);
      if (remote_iter != remote_inbound_audio_map.end()) {
        audio_outband_stats.fraction_lost =
            remote_iter->second->fraction_lost.ValueOrDefault(0);
        audio_outband_stats.round_trip_time =
            remote_iter->second->round_trip_time.ValueOrDefault(0);
      }
    }

    if (audio_outbound->media_source_id.is_defined()) {
      auto media_iter = audio_mediasource_map.find(*audio_outbound->media_source_id);
      if (media_iter != audio_mediasource_map.end()){
        audio_outband_stats.audio_level = media_iter->second->audio_level.ValueOrDefault(0);
        audio_outband_stats.total_audio_energy = media_iter->second->total_audio_energy.ValueOrDefault(0);
      }
    }

    if (stats_.timestamp > 0) {
      auto audio_iter = stats_.audios.find(audio_outband_stats.ssrc);
      if (audio_iter != stats_.audios.end()) {
        audio_outband_stats.bitrate_send =
            (audio_outband_stats.bytes_sent - audio_iter->second.bytes_sent) * 1000.0 /
            (stats.timestamp - stats_.timestamp);
      }
    }
    stats.audios[audio_outband_stats.ssrc] = audio_outband_stats;
  }

   for (auto video_outbound : outbound_video_map) {
    stats.timestamp = video_outbound->timestamp_us();

    RTCVideoOutBandStats video_outband_stats;
    video_outband_stats.ssrc = video_outbound->ssrc.ValueOrDefault(0);
    video_outband_stats.bitrate_send = 0;
    video_outband_stats.frame_width = video_outbound->frame_width.ValueOrDefault(0);
    video_outband_stats.frame_height = video_outbound->frame_height.ValueOrDefault(0);

    video_outband_stats.frames_encoded = video_outbound->frames_encoded.ValueOrDefault(0);
    video_outband_stats.frames_sent = video_outbound->frames_sent.ValueOrDefault(0);
    video_outband_stats.total_packet_send_delay = video_outbound->total_packet_send_delay.ValueOrDefault(0);

    video_outband_stats.quality_limitation_reason = video_outbound->quality_limitation_reason.ValueOrDefault("");
    video_outband_stats.encoder_implementation = video_outbound->encoder_implementation.ValueOrDefault("");
    video_outband_stats.fir_count = video_outbound->fir_count.ValueOrDefault(0);
    video_outband_stats.pli_count = video_outbound->pli_count.ValueOrDefault(0);
    video_outband_stats.nack_count = video_outbound->nack_count.ValueOrDefault(0);
    video_outband_stats.qp_sum = video_outbound->qp_sum.ValueOrDefault(0);

    video_outband_stats.retransmitted_bytes_sent = video_outbound->retransmitted_bytes_sent.ValueOrDefault(0);

    video_outband_stats.packets_sent = video_outbound->packets_sent.ValueOrDefault(0);
    video_outband_stats.bytes_sent = video_outbound->bytes_sent.ValueOrDefault(0);

    if (video_outbound->frames_per_second.is_defined()) {
      video_outband_stats.frames_per_second =
          video_outbound->frames_per_second.ValueOrDefault(0);
    } else if (video_outbound->media_source_id.is_defined()) {
      auto media_source = video_mediasource_map.find(*video_outbound->media_source_id);
      if (media_source != video_mediasource_map.end()) {
        video_outband_stats.frames_per_second =
            media_source->second->frames_per_second.ValueOrDefault(0);
      }
    }

    if (video_outbound->remote_id.is_defined() ) { 
      auto romote_iter =
          remote_inbound_video_map.find(*video_outbound->remote_id);
      if (romote_iter != remote_inbound_video_map.end()) {
        video_outband_stats.fraction_lost = romote_iter->second->fraction_lost.ValueOrDefault(0);
        video_outband_stats.round_trip_time = romote_iter->second->round_trip_time.ValueOrDefault(0);
      }
    }
    video_outband_stats.target_delay_ms = 0;
    if (stats_.timestamp) {
      auto video_iter = stats_.videos.find(video_outband_stats.ssrc);
      if (video_iter != stats_.videos.end()) {
        video_outband_stats.bitrate_send =
            (video_outband_stats.bytes_sent - video_iter->second.bytes_sent) * 1000.0 /
            (stats.timestamp - stats_.timestamp);

        video_outband_stats.retransmitted_bitrate =
            (video_outband_stats.retransmitted_bytes_sent - video_iter->second.retransmitted_bytes_sent) * 1000.0 / (stats.timestamp - stats_.timestamp);

        if (video_outband_stats.packets_sent != video_iter->second.packets_sent) {
          video_outband_stats.target_delay_ms =
              (video_outband_stats.total_packet_send_delay - video_iter->second.total_packet_send_delay) * 1000.0 /
              ( video_outband_stats.packets_sent - video_iter->second.packets_sent);
        }

        if (video_iter->second.frames_encoded != video_outband_stats.frames_encoded) {
          video_outband_stats.qp = double(video_outband_stats.qp_sum - video_iter->second.qp_sum) /
              (video_outband_stats.frames_encoded - video_iter->second.frames_encoded);
        }
      }
    }
    stats.videos[video_outband_stats.ssrc] = video_outband_stats;
  }

   for (auto iter = transport_map.begin(); iter != transport_map.end(); ++iter){
    stats.bytes_sent += iter->second->bytes_sent.ValueOrDefault(0);
    stats.packets_sent += iter->second->packets_sent.ValueOrDefault(0);
   }

   stats.bitrate_send = 0;
   if (stats_.bytes_sent > 0 && stats.timestamp != stats_.timestamp) {

     stats.bitrate_send = (stats.bytes_sent - stats_.bytes_sent) * 1000.0 /
                          (stats.timestamp - stats_.timestamp) ;
   }

  stats_ = stats;

}

RTCInBandStats RTCInBoundStatsCollectorCallBack::GetInBandStats() const {
  return stats_;
}

void RTCInBoundStatsCollectorCallBack::OnStatsDelivered(const rtc::scoped_refptr<const webrtc::RTCStatsReport>& report) {
  std::string str_report = report->ToJson();
  Initialization();
 
  auto intbounds = report->GetStatsOfType<webrtc::RTCInboundRTPStreamStats>();
  for (const webrtc::RTCInboundRTPStreamStats* intbound : intbounds) {
    if (*intbound->kind == "audio") {
      inbound_audio_map.push_back(intbound);
      remote_outbound_audio_map.insert(std::make_pair(*intbound->remote_id, nullptr));
      audio_codec_map.insert(std::make_pair(*intbound->codec_id, nullptr));
    } else {
      inbound_video_map.push_back(intbound);
      remote_outbound_video_map.insert(std::make_pair(*intbound->remote_id, nullptr));
      video_codec_map.insert(std::make_pair(*intbound->codec_id, nullptr));
    }
  }

  bool hasAudio = !inbound_audio_map.empty();
  bool hasVideo = !inbound_video_map.empty();
  if (hasAudio || hasVideo){
    // 目前认为视频统一使用一种编码器，音频统一使用一种编码器
    auto iter_audio = audio_codec_map.begin();
    auto iter_video = video_codec_map.begin();

    auto codecs = report->GetStatsOfType<webrtc::RTCCodecStats>();
    for (auto codec : codecs) {
      if ((!hasAudio || iter_audio->second) &&
          (!hasVideo || iter_video->second)) {
        break;
      }
      if (hasAudio && !iter_audio->second && iter_audio->first == codec->id()) {
        iter_audio->second = codec;
      } else if (hasVideo && !iter_video->second &&
                 iter_video->first == codec->id()) {
        iter_video->second = codec;
      }
    }

    // audio mediasource
    auto audio_mediasources =
        report->GetStatsOfType<webrtc::RTCAudioSourceStats>();
    for (auto audio_mediasource : audio_mediasources) {
      audio_mediasource_map[audio_mediasource->id()] = audio_mediasource;
    }

    // video mediasource
    auto video_mediasources =
        report->GetStatsOfType<webrtc::RTCVideoSourceStats>();
    for (auto video_mediasource : video_mediasources) {
      video_mediasource_map[video_mediasource->id()] = video_mediasource;
    }

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

    auto transports = report->GetStatsOfType<webrtc::RTCTransportStats>();
    for (auto transport : transports) {
      transport_map[transport->id()] = transport;
    }
  }

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
  audio_codec_map.clear();
  video_codec_map.clear();
  transport_map.clear();
}

void RTCInBoundStatsCollectorCallBack::CalcStats() {
  RTCInBandStats stats;

  //inbound_audio
  for (auto inbound_audio : inbound_audio_map) {
    stats.timestamp = inbound_audio->timestamp_us();
    RTCAudioInBandStats audio_stats;
    audio_stats.ssrc = inbound_audio->ssrc.ValueOrDefault(0);
    audio_stats.jitter = inbound_audio->jitter.ValueOrDefault(0);
    audio_stats.packets_lost = inbound_audio->packets_lost.ValueOrDefault(0);
    audio_stats.packets_received = inbound_audio->packets_received.ValueOrDefault(0);
    audio_stats.fec_packets_received = inbound_audio->fec_packets_received.ValueOrDefault(0);
    audio_stats.fec_packets_discarded = inbound_audio->fec_packets_discarded.ValueOrDefault(0);
    audio_stats.bytes_received = inbound_audio->bytes_received.ValueOrDefault(0);
    audio_stats.delay_estimate_ms = inbound_audio->target_delay_ms.ValueOrDefault(0);

    audio_stats.jitter_buffer_delay = inbound_audio->jitter_buffer_delay.ValueOrDefault(0);
    audio_stats.jitter_buffer_emitted_count = inbound_audio->jitter_buffer_emitted_count.ValueOrDefault(0);
    audio_stats.total_samples_received = inbound_audio->total_samples_received.ValueOrDefault(0);
    audio_stats.concealed_samples = inbound_audio->concealed_samples.ValueOrDefault(0);
    audio_stats.silent_concealed_samples = inbound_audio->silent_concealed_samples.ValueOrDefault(0);
    audio_stats.concealment_events = inbound_audio->concealment_events.ValueOrDefault(0);
    audio_stats.inserted_samples_for_deceleration = inbound_audio->inserted_samples_for_deceleration.ValueOrDefault(0);
    audio_stats.removed_samples_for_acceleration = inbound_audio->removed_samples_for_acceleration.ValueOrDefault(0);
    audio_stats.audio_level = inbound_audio->audio_level.ValueOrDefault(0);
    audio_stats.total_audio_energy = inbound_audio->total_audio_energy.ValueOrDefault(0);
    audio_stats.total_samples_duration = inbound_audio->total_samples_duration.ValueOrDefault(0);

    // 计算丢包
    audio_stats.fraction_lost = 0; 
    audio_stats.bitrate_recv = 0;;

    auto audio_item = stats_.audios.find(audio_stats.ssrc);
    if (audio_item != stats_.audios.end()) {
      //已有数据则合并，相邻数据
      if (audio_item->second.packets_received != audio_stats.packets_received) {
        //丢包率
        audio_stats.fraction_lost =(audio_stats.packets_lost - audio_item->second.packets_lost) /
            (double)(audio_stats.packets_received - audio_item->second.packets_received);
      }
      //码率
      audio_stats.bitrate_recv = (audio_stats.bytes_received - audio_item->second.bytes_received) * 1000/
          (double)(stats.timestamp - stats_.timestamp);

      audio_stats.audio_caton_ms = audio_item->second.audio_caton_ms;
      if (audio_stats.total_samples_received != audio_item->second.total_samples_received){
        audio_stats.audio_caton_ms =
            (audio_stats.total_samples_duration - audio_item->second.total_samples_duration) *
            1000.0 / (double)(audio_stats.total_samples_received - audio_item->second.total_samples_received);
      }

    }
    stats.audios.insert(std::make_pair(audio_stats.ssrc, audio_stats));

  }

  //inbound_video
  for (auto inbound_video : inbound_video_map){
    stats.timestamp = inbound_video->timestamp_us();
    RTCVideoInBandStats video_stats;
    video_stats.ssrc = inbound_video->ssrc.ValueOrDefault(0);
    video_stats.jitter = inbound_video->jitter.ValueOrDefault(0);
    video_stats.packets_lost = inbound_video->packets_lost.ValueOrDefault(0);
    video_stats.packets_received = inbound_video->packets_received.ValueOrDefault(0);
    video_stats.bytes_received = inbound_video->bytes_received.ValueOrDefault(0);
    video_stats.frames_received = inbound_video->frames_received.ValueOrDefault(0);
    video_stats.frame_width = inbound_video->frame_width.ValueOrDefault(0);
    video_stats.frame_height = inbound_video->frame_height.ValueOrDefault(0);
    video_stats.frames_per_second = inbound_video->frames_per_second.ValueOrDefault(0);
    video_stats.frames_decoded = inbound_video->frames_decoded.ValueOrDefault(0);
    video_stats.key_frames_decoded = inbound_video->key_frames_decoded.ValueOrDefault(0);
    video_stats.frames_dropped = inbound_video->frames_dropped.ValueOrDefault(0);
    video_stats.total_decode_time = inbound_video->total_decode_time.ValueOrDefault(0);
    video_stats.total_inter_frame_delay = inbound_video->total_inter_frame_delay.ValueOrDefault(0);
    video_stats.total_squared_inter_frame_delay = inbound_video->total_squared_inter_frame_delay.ValueOrDefault(0);
    video_stats.fir_count = inbound_video->fir_count.ValueOrDefault(0);
    video_stats.pli_count = inbound_video->pli_count.ValueOrDefault(0);
    video_stats.nack_count = inbound_video->nack_count.ValueOrDefault(0);
    video_stats.qp_sum = inbound_video->qp_sum.ValueOrDefault(0);
    video_stats.render_delay_ms = inbound_video->render_delay_ms.ValueOrDefault(0);
    video_stats.target_delay_ms = inbound_video->target_delay_ms.ValueOrDefault(0);
    video_stats.total_caton_count =inbound_video->total_caton_count.ValueOrDefault(0);
    video_stats.total_caton_delay_ms = inbound_video->total_caton_delay_ms.ValueOrDefault(0);

    std::string str_track_id = *inbound_video->track_id;
    if (!str_track_id.empty()) {
      auto video_track = track_map.find(str_track_id);
      if (video_track != track_map.end()) {
        //track信息
        if (video_track->second)
        {
          video_stats.freeze_count =
              video_track->second->freeze_count.ValueOrDefault(0);
          video_stats.total_freezes_duration =
              video_track->second->total_freezes_duration.ValueOrDefault(0);
          video_stats.total_pauses_duration =
              video_track->second->total_pauses_duration.ValueOrDefault(0);
          video_stats.total_frames_duration =
              video_track->second->total_frames_duration.ValueOrDefault(0);
          video_stats.sum_squared_frame_durations =
              video_track->second->sum_squared_frame_durations.ValueOrDefault(
                  0);
        }
      }
    }

    video_stats.fraction_lost = 0;
    video_stats.bitrate_recv = 0;
    video_stats.video_caton_ms = 0; 
    auto video_item = stats_.videos.find(video_stats.ssrc);
    if (video_item != stats_.videos.end()) {
      // 计算丢包
      if (video_item->second.packets_received != video_stats.packets_received) {
        video_stats.fraction_lost =
            (video_stats.packets_lost - video_item->second.packets_lost) /
            (double)(video_stats.packets_received -
                     video_item->second.packets_received);
      }
      video_stats.bitrate_recv =
          (video_stats.bytes_received - video_item->second.bytes_received) *
          1000 / (double)(stats.timestamp - stats_.timestamp);

      if (video_item->second.frames_decoded != video_stats.frames_decoded) {
        video_stats.qp = double(video_stats.qp_sum - video_item->second.qp_sum) /
            (video_stats.frames_decoded - video_item->second.frames_decoded);
      }

      video_stats.video_caton_ms = video_stats.total_caton_delay_ms - video_item->second.total_caton_delay_ms;
    }
    stats.videos.insert(std::make_pair(video_stats.ssrc, video_stats));
  }

    for (auto iter = transport_map.begin(); iter != transport_map.end(); ++iter) {
    stats.bytes_recv += iter->second->bytes_received.ValueOrDefault(0);
    stats.packets_recv += iter->second->packets_received.ValueOrDefault(0);
  }

  stats.bitrate_recv = 0;
  if (stats_.bytes_recv > 0 && stats.timestamp != stats_.timestamp) {
    stats.bitrate_recv = (stats.bytes_recv - stats_.bytes_recv) * 1000.0 /
                         (stats.timestamp - stats_.timestamp);
  }

  stats_ = stats;
}
}  // namespace webrtc