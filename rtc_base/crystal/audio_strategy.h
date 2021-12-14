#pragma once
#include <unordered_map>
#include  <memory>

class AudioStrategy {
public:
	enum AudioStrategyState
	{
		kNone = 0, // do nothing
		kLow,      // resend 1
		kMedium,   // resend 2
		kHigh,     // resend 3
		kHighHigh, // resend 4
	};

	enum DetectionStatus
	{
		kDetectionLow = -1,		// demotion
		kDetectingNormal,		// unchanging
		kDetectingHigh,			// escalate
	};

public:
	static AudioStrategy* getInstance();
		
public:
  void UpdatePacketsLost(double fraction_loss);
  int GetAudioResendCount() const;
  void SetAudioSsrc(uint32_t ssrc);
  bool IsAudioSsrc(uint32_t ssrc);

private:
  AudioStrategy();

 private:
	AudioStrategyState state;
	DetectionStatus dection_status;
  uint32_t audio_ssrc;
};
