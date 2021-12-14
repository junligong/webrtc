#include "audio_strategy.h"
#include "rtc_base/logging.h"

static AudioStrategy::AudioStrategyState get_state(double fraction_loss)
{
	if (fraction_loss >= 0.60)
	{
		return AudioStrategy::AudioStrategyState::kHighHigh;
	} 
	else if (fraction_loss >= 0.50)
	{
		return AudioStrategy::AudioStrategyState::kHigh;
	}
	else if (fraction_loss >= 0.45)
	{
		return AudioStrategy::AudioStrategyState::kMedium;
	}
	else if (fraction_loss >= 0.2)
	{
		return AudioStrategy::AudioStrategyState::kLow;
	}
	return AudioStrategy::AudioStrategyState::kNone;
}

static AudioStrategy::DetectionStatus get_detection_state(double fraction_loss)
{
	if (fraction_loss < 0.1)
	{
		return AudioStrategy::DetectionStatus::kDetectionLow;
	}
	else if (fraction_loss > 0.2)
	{
		return AudioStrategy::DetectionStatus::kDetectingHigh;
	}
	return AudioStrategy::DetectionStatus::kDetectingNormal;
}

AudioStrategy* AudioStrategy::getInstance() {

	static AudioStrategy pInstance;
  return &pInstance;
}


void AudioStrategy::UpdatePacketsLost(double fraction_loss) {
	 // 如果什么策略都没开，则使用默认策略计算重发次数
	if (state == kNone){
		state = get_state(fraction_loss);
	} else {
  // 当开启重发策略时候，目标则是将丢包率维持在10%~20%之间,过高则提升,过低则降低，为了防止抖动，确保两次相同数据再启动
		AudioStrategy::DetectionStatus status = get_detection_state(fraction_loss);
		if (status == kDetectionLow) {
		 state = (AudioStrategy::AudioStrategyState)(state - 1);
		} else if (status == kDetectingHigh && state != kHighHigh) {
			state = (AudioStrategy::AudioStrategyState)(state + 1);
	  }
	}
	RTC_LOG(INFO) << "fraction_loss:" << fraction_loss
                << " resend count:" << (int)state;
 }

int AudioStrategy::GetAudioResendCount() const{
  return (int)state;

}

void AudioStrategy::SetAudioSsrc(uint32_t ssrc) {
	audio_ssrc = ssrc;
}

 bool AudioStrategy::IsAudioSsrc(uint32_t ssrc){
	return audio_ssrc == ssrc;
}

AudioStrategy::AudioStrategy() {
	state = kNone;
	dection_status = kDetectingNormal;
  audio_ssrc = 0;
}

