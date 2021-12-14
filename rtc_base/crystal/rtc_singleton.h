#pragma once
#include <iostream>
#include <memory>

namespace webrtc {
template <typename T>
class rtc_singleton {
 private:
  rtc_singleton() {}

 public:
  static T* getInstance() {
    static T t;
    return &t;
  }
};

}// namespace webrtc
