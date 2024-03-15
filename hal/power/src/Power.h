/*
 * Copyright (C) 2023 Linaro Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <aidl/android/hardware/power/BnPower.h>
#include <map>
#include <functional>

#include "Timer.h"

struct performance_handler;

namespace aidl::android::hardware::power::impl::linaro {

class Power : public BnPower {

public:
	ndk::ScopedAStatus setMode(Mode type, bool enabled) override;
	ndk::ScopedAStatus isModeSupported(Mode type, bool* _aidl_return) override;
	ndk::ScopedAStatus setBoost(Boost type, int32_t durationMs) override;
	ndk::ScopedAStatus isBoostSupported(Boost type, bool* _aidl_return) override;
	ndk::ScopedAStatus createHintSession(int32_t tgid, int32_t uid,
					     const std::vector<int32_t>& threadIds,
					     int64_t durationNanos,
					     std::shared_ptr<IPowerHintSession>* _aidl_return) override;
	ndk::ScopedAStatus getHintSessionPreferredRate(int64_t* outNanoseconds) override;
	Power(Looper *looper);

	struct performance_handler *getPerfHandler(Power *power);

private:
	class BoostAction {
	public:
		BoostAction(bool (Power::*action)(bool), int defaultDurationMs, Looper *looper, Power *power):
			mAction(action), mDefaultDurationMs(defaultDurationMs), mPower(power)
		{
			mTimer = new Timer(looper, false);
			if (!mTimer)
				throw("Failed to create timer");
		};

		Timer *mTimer;
		bool (Power::*mAction)(bool);
		int mDefaultDurationMs;
		Power *mPower;
	};

	class CancelBoostAction : public TimerCallback {
	public:
		virtual int handleTimer(void *data) {
			BoostAction *boostAction = (typeof(boostAction))data;
			std::invoke(boostAction->mAction, boostAction->mPower, false);
			return 1;
		}
	};

	bool setMode_DoubleTapToWake(bool enabled);
	bool setMode_LowPower(bool enabled);
	bool setMode_SustainedPerformance(bool enabled);
	bool setMode_FixedPerformance(bool enabled);
	bool setMode_VR(bool enabled);
	bool setMode_Launch(bool enabled);
	bool setMode_ExpensiveRendering(bool enabled);
	bool setMode_Interactive(bool enabled);
	bool setMode_DeviceIdle(bool enabled);
	bool setMode_DisplayInactive(bool enabled);
	bool setMode_AudioStreamingLowLatency(bool enabled);
	bool setMode_CameraStreamingSecure(bool enabled);
	bool setMode_CameraStreamingLow(bool enabled);
	bool setMode_CameraStreamingMid(bool enabled);
	bool setMode_CameraStreamingHigh(bool enabled);
	bool setMode_Game(bool enabled);
	bool setMode_GameLoading(bool enabled);

	bool setBoost_Interaction(bool enabled);
	bool setBoost_DisplayUpdateImminent(bool enabled);
	bool setBoost_MlAcc(bool enabled);
	bool setBoost_AudioLaunch(bool enabled);
	bool setBoost_CameraLaunch(bool enabled);
	bool setBoost_CameraShot(bool enabled);

	std::string getUuid(void);

	std::map<int32_t, std::shared_ptr<IPowerHintSession>> mPowerHintSessions;
	std::map<Mode, bool(Power::*)(bool)> mModeActions;
	std::map<Boost, BoostAction *> mBoostActions;

	CancelBoostAction *mCancelBoostAction;

	Mode mCurrentMode;
	Boost mCurrentBoost;

	struct performance_handler *perf_handler;
};

} // namespace aidl::android::hardware::power::impl::linaro
