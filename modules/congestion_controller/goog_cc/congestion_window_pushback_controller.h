/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef MODULES_CONGESTION_CONTROLLER_GOOG_CC_CONGESTION_WINDOW_PUSHBACK_CONTROLLER_H_
#define MODULES_CONGESTION_CONTROLLER_GOOG_CC_CONGESTION_WINDOW_PUSHBACK_CONTROLLER_H_

#include <stddef.h>
#include <stdint.h>

#include "absl/types/optional.h"
#include "api/transport/webrtc_key_value_config.h"
#include "api/units/data_size.h"

namespace webrtc {

// 此类支持从拥塞窗口直接推送到视频编码器。
// 当拥塞窗口被填满时，视频编码器目标比特率将相应降低以适应网络变化
// 为了避免过于频繁地暂停视频，使用最小编码器目标比特率阈值来防止视频由于完全拥塞窗口而暂停。

// 拥塞窗口控制器
class CongestionWindowPushbackController {
 public:
  explicit CongestionWindowPushbackController(
      const WebRtcKeyValueConfig* key_value_config);
  void UpdateOutstandingData(int64_t outstanding_bytes);
  void UpdatePacingQueue(int64_t pacing_bytes);
  uint32_t UpdateTargetBitrate(uint32_t bitrate_bps);
  void SetDataWindow(DataSize data_window);

 private:
  const bool add_pacing_;
  const uint32_t min_pushback_target_bitrate_bps_;
  absl::optional<DataSize> current_data_window_;
  int64_t outstanding_bytes_ = 0;
  int64_t pacing_bytes_ = 0;
  double encoding_rate_ratio_ = 1.0;
};

}  // namespace webrtc

#endif  // MODULES_CONGESTION_CONTROLLER_GOOG_CC_CONGESTION_WINDOW_PUSHBACK_CONTROLLER_H_
