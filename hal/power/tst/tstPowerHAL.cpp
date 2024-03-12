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
#define LOG_TAG "tstPowerlHal"

#include <aidl/android/hardware/power/BnPower.h>
#include <aidl/android/hardware/power/BnPowerHintSession.h>
#include <android-base/properties.h>
#include <android/binder_ibinder.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <android/binder_status.h>

#include <iostream>

#include "Power.h"
#include "Timer.h"

using ::android::sp;
using ::android::Looper;

using ::aidl::android::hardware::power::IPower;
using ::aidl::android::hardware::power::Boost;
using ::aidl::android::hardware::power::Mode;
using ::aidl::android::hardware::power::impl::linaro::Power;
using ::aidl::android::hardware::power::impl::linaro::Timer;
using ::aidl::android::hardware::power::impl::linaro::TimerCallback;

int testBoostSupported(std::shared_ptr<IPower> power)
{
	ndk::ScopedAStatus status;
	bool supported;

	Boost boosts[] = {
		Boost::INTERACTION,
		Boost::DISPLAY_UPDATE_IMMINENT,
		Boost::ML_ACC,
		Boost::AUDIO_LAUNCH,
		Boost::CAMERA_LAUNCH,
		Boost::CAMERA_SHOT,
	};

	for (int i = 0; i < sizeof(boosts) / sizeof(boosts[0]); i++) {

		status = power->isBoostSupported(boosts[i], &supported);
		if (!status.isOk()) {
			std::cout << "Failed to check if boost is supported" << std::endl;
			return -1;
		}

		std::cout << "Boost[" << toString(boosts[i]) << "]";

		if (supported)
			std::cout << " is supported";
		else
			std::cout << " is not supported";

		std::cout << std::endl;
	}

	return 0;
}

int testBoost(std::shared_ptr<IPower> power)
{
	ndk::ScopedAStatus status;
	bool supported;

	Boost boosts[] = {
		Boost::INTERACTION,
		Boost::DISPLAY_UPDATE_IMMINENT,
		Boost::ML_ACC,
		Boost::AUDIO_LAUNCH,
		Boost::CAMERA_LAUNCH,
		Boost::CAMERA_SHOT,
	};

	for (int i = 0; i < sizeof(boosts) / sizeof(boosts[0]); i++) {

		status = power->setBoost(boosts[i], 2000);
		if (!status.isOk()) {
			std::cout << "Failed to set boost" << std::endl;
			return -1;
		}

		std::cout << "Set Boost[" << toString(boosts[i])
			  << "] for 2000ms" << std::endl;
	}

	return 0;
}

int testModeSupported(std::shared_ptr<IPower> power)
{
	ndk::ScopedAStatus status;
	bool supported;

	Mode modes[] = {
		Mode::DOUBLE_TAP_TO_WAKE,
		Mode::LOW_POWER,
		Mode::SUSTAINED_PERFORMANCE,
		Mode::FIXED_PERFORMANCE,
		Mode::VR,
		Mode::LAUNCH,
		Mode::EXPENSIVE_RENDERING,
		Mode::INTERACTIVE,
		Mode::DEVICE_IDLE,
		Mode::DISPLAY_INACTIVE,
		Mode::AUDIO_STREAMING_LOW_LATENCY,
		Mode::CAMERA_STREAMING_SECURE,
		Mode::CAMERA_STREAMING_LOW,
		Mode::CAMERA_STREAMING_MID,
		Mode::CAMERA_STREAMING_HIGH,
		Mode::GAME,
		Mode::GAME_LOADING,
	};

	for (int i = 0; i < sizeof(modes) / sizeof(modes[0]); i++) {

		status = power->isModeSupported(modes[i], &supported);
		if (!status.isOk()) {
			std::cout << "Failed to check if mode is supported" << std::endl;
			return -1;
		}

		std::cout << "Mode[" << toString(modes[i]) << "]";

		if (supported)
			std::cout << " is supported";
		else
			std::cout << " is not supported";

		std::cout << std::endl;
	}

	return 0;
}

int testMode(std::shared_ptr<IPower> power)
{
	ndk::ScopedAStatus status;
	bool supported;

	Mode modes[] = {
		Mode::DOUBLE_TAP_TO_WAKE,
		Mode::LOW_POWER,
		Mode::SUSTAINED_PERFORMANCE,
		Mode::FIXED_PERFORMANCE,
		Mode::VR,
		Mode::LAUNCH,
		Mode::EXPENSIVE_RENDERING,
		Mode::INTERACTIVE,
		Mode::DEVICE_IDLE,
		Mode::DISPLAY_INACTIVE,
		Mode::AUDIO_STREAMING_LOW_LATENCY,
		Mode::CAMERA_STREAMING_SECURE,
		Mode::CAMERA_STREAMING_LOW,
		Mode::CAMERA_STREAMING_MID,
		Mode::CAMERA_STREAMING_HIGH,
		Mode::GAME,
		Mode::GAME_LOADING,
	};

	for (int i = 0; i < sizeof(modes) / sizeof(modes[0]); i++) {

		status = power->setMode(modes[i], true);
		if (!status.isOk()) {
			std::cout << "Failed to set mode " << toString(modes[i])  << std::endl;
			return -1;
		}

		std::cout << "Enable Mode[" << toString(modes[i]) << "]" << std::endl;

		sleep(1);

		status = power->setMode(modes[i], false);
		if (!status.isOk()) {
			std::cout << "Failed to unset mode " << toString(modes[i])  << std::endl;
			return -1;
		}

		std::cout << "Disable Mode[" << toString(modes[i]) << "]" << std::endl;
	}

	return 0;
}

int main(int, char *[])
{
	std::string service("android.hardware.power.IPower/default");
	std::shared_ptr<IPower> power;
	AIBinder* binder;

	binder = AServiceManager_waitForService(service.c_str());
	if (!binder) {
		std::cout << "Failed to get service: " << service << std::endl;
		return 1;
	}

	power = IPower::fromBinder(ndk::SpAIBinder(binder));
	if (!power) {
		std::cout << "Failed to get service from binder" << std::endl;
		return 1;
	}

	if (testModeSupported(power)) {
		std::cout << "Mode supported test failed" << std::endl;
		return 1;
	}

	if (testMode(power)) {
		std::cout << "Mode supported test failed" << std::endl;
		return 1;
	}

	if (testBoostSupported(power)) {
		std::cout << "Boost supported test failed" << std::endl;
		return 1;
	}

	if (testBoost(power)) {
		std::cout << "Boost supported test failed" << std::endl;
		return 1;
	}

	return 0;
}
