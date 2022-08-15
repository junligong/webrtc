/*
 *  Copyright 2012 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef EXAMPLES_PEERCONNECTION_CLIENT_CONDUCTOR_H_
#define EXAMPLES_PEERCONNECTION_CLIENT_CONDUCTOR_H_

#include <deque>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "api/media_stream_interface.h"
#include "api/peer_connection_interface.h"
#include "examples/peerconnection/client/main_wnd.h"
#include "examples/peerconnection/client/peer_connection_client.h"
#include "rtc_base/thread.h"

#include <functional>
#include <chrono>
#include <thread>
#include <atomic>
#include <memory>
#include <mutex>
#include <condition_variable>
#include "api/stats/rtc_stats_collector_callback.h"
#include "api/stats/rtc_stats_collector_callback2.h"

class Timer {
 private:
  std::atomic<bool> expired_;
  std::atomic<bool> try_to_expire_;
  std::mutex mtx;
  std::condition_variable cv;

 public:
  Timer() : expired_(true), try_to_expire_(false) {}

  Timer(const Timer& timer) {
    expired_ = timer.expired_.load();
    try_to_expire_ = timer.try_to_expire_.load();
  }

  ~Timer() { stop(); }

  void start(int interval, std::function<void()> task) {
    if (!expired_)
      return;

    expired_ = false;
    std::thread([this, interval, task]() {
      while (!try_to_expire_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(interval));
        task();
      }

      std::lock_guard<std::mutex> lock(mtx);
      expired_ = true;
      cv.notify_one();
    }).detach();
  }

  void start_once(int delay, std::function<void()> task) {
    std::thread([delay, task]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(delay));
      task();
    }).detach();
  }

  void stop() {
    if (expired_)
      return;

    if (try_to_expire_)
      return;

    try_to_expire_ = true;

    //     std::unique_lock<std::mutex> lock(mtx);
    //     cv.wait(lock, [this] { return expired_ == true; });

    if (expired_)
      try_to_expire_ = false;
  }
};

namespace webrtc {
class VideoCaptureModule;

class StatsCollectorCallback2 : public RTCInBoundStatsCollectorCallBack {
 public:
  StatsCollectorCallback2() {}
  ~StatsCollectorCallback2() override {}

  /* Virtual methods inherited from webrtc::RTCStatsCollectorCallback. */
 public:
  void OnStatsDelivered(
      const rtc::scoped_refptr<const webrtc::RTCStatsReport>& report) override {
    std::string str_report = report->ToJson();
    RTC_LOG(LS_INFO) << report->ToJson();

    RTCInBoundStatsCollectorCallBack::OnStatsDelivered(report);
  }
};
class StatsCollectorCallback : public RTCOutBoundStatsCollectorCallBack {
 public:
  StatsCollectorCallback() {}
  ~StatsCollectorCallback() override {}

  /* Virtual methods inherited from webrtc::RTCStatsCollectorCallback. */
 public:
  void OnStatsDelivered(
      const rtc::scoped_refptr<const webrtc::RTCStatsReport>& report) override {
    std::string str_report = report->ToJson();
    RTC_LOG(LS_INFO) << report->ToJson();

    RTCOutBoundStatsCollectorCallBack::OnStatsDelivered(report);
  }
};
}  // namespace webrtc

namespace cricket {
class VideoRenderer;
}  // namespace cricket

class Conductor : public webrtc::PeerConnectionObserver,
                  public webrtc::CreateSessionDescriptionObserver,
                  public PeerConnectionClientObserver,
                  public MainWndCallback {
 public:
  enum CallbackID {
    MEDIA_CHANNELS_INITIALIZED = 1,
    PEER_CONNECTION_CLOSED,
    SEND_MESSAGE_TO_PEER,
    NEW_TRACK_ADDED,
    TRACK_REMOVED,
  };

  Conductor(PeerConnectionClient* client, MainWindow* main_wnd);

  bool connection_active() const;

  void Close() override;

 protected:
  ~Conductor();
  bool InitializePeerConnection();
  bool ReinitializePeerConnectionForLoopback();
  bool CreatePeerConnection();
  void DeletePeerConnection();
  void EnsureStreamingUI();
  void AddTracks();

  //
  // PeerConnectionObserver implementation.
  //

  void OnSignalingChange(
      webrtc::PeerConnectionInterface::SignalingState new_state) override {}
  void OnAddTrack(
      rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
      const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>&
          streams) override;
  void OnRemoveTrack(
      rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) override;
  void OnDataChannel(
      rtc::scoped_refptr<webrtc::DataChannelInterface> channel) override {}
  void OnRenegotiationNeeded() override {}
  void OnIceConnectionChange(
      webrtc::PeerConnectionInterface::IceConnectionState new_state) override {}
  void OnIceGatheringChange(
      webrtc::PeerConnectionInterface::IceGatheringState new_state) override {}
  void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override;
  void OnIceConnectionReceivingChange(bool receiving) override {}

  //
  // PeerConnectionClientObserver implementation.
  //

  void OnSignedIn() override;

  void OnDisconnected() override;

  void OnPeerConnected(int id, const std::string& name) override;

  void OnPeerDisconnected(int id) override;

  void OnMessageFromPeer(int peer_id, const std::string& message) override;

  void OnMessageSent(int err) override;

  void OnServerConnectionFailure() override;

  //
  // MainWndCallback implementation.
  //

  void StartLogin(const std::string& server, int port) override;

  void DisconnectFromServer() override;

  void ConnectToPeer(int peer_id) override;

  void DisconnectFromCurrentPeer() override;

  void UIThreadCallback(int msg_id, void* data) override;

  // CreateSessionDescriptionObserver implementation.
  void OnSuccess(webrtc::SessionDescriptionInterface* desc) override;
  void OnFailure(webrtc::RTCError error) override;

 protected:
  // Send a message to the remote peer.
  void SendMessage(const std::string& json_object);

  int peer_id_;
  bool loopback_;
  std::unique_ptr<rtc::Thread> signaling_thread_;
  rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection_;
  rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface>
      peer_connection_factory_;
  PeerConnectionClient* client_;
  MainWindow* main_wnd_;
  std::deque<std::string*> pending_messages_;
  std::string server_;

  Timer timer_;
  rtc::scoped_refptr<webrtc::StatsCollectorCallback> stats_callback;
  rtc::scoped_refptr<webrtc::StatsCollectorCallback2> stats_callback2;
};

#endif  // EXAMPLES_PEERCONNECTION_CLIENT_CONDUCTOR_H_
