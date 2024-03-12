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
#define LOG_TAG "tstThermalHal"

#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/timerfd.h>
#include <sys/types.h>

#include<iostream>

#include <android/hardware/thermal/2.0/IThermal.h>
#include <android/hardware/thermal/2.0/IThermalChangedCallback.h>
#include <android/hardware/thermal/2.0/types.h>
#include <android-base/logging.h>
#include <hidl/HidlTransportSupport.h>
#include <hidl/ServiceManagement.h>

#include "Thermal.h"

#define SKIN_POLLING_SEC 2

using ::android::sp;
using ::android::Looper;
using ::android::hardware::hidl_enum_range;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::thermal::V1_0::ThermalStatus;
using ::android::hardware::thermal::V1_0::ThermalStatusCode;
using ::android::hardware::thermal::V2_0::CoolingDevice;
using ::android::hardware::thermal::V2_0::CoolingType;
using ::android::hardware::thermal::V2_0::IThermal;
using ::android::hardware::thermal::V2_0::IThermalChangedCallback;
using ::android::hardware::thermal::V2_0::Temperature;
using ::android::hardware::thermal::V2_0::TemperatureThreshold;
using ::android::hardware::thermal::V2_0::TemperatureType;
using ::android::hardware::thermal::V2_0::ThrottlingSeverity;

using ::android::hardware::setupTransportPolling;
using ::android::hardware::handleTransportPoll;

class ThermalCallback : public IThermalChangedCallback {
   public:
	Return<void> notifyThrottling(const Temperature& temperature) override {
		/*
		 * ThermalCallbackArgs args;
		 * args.temperature = temperature;
		 * NotifyFromCallback(kCallbackNameNotifyThrottling, args);
		 */
		std::cout << "---------- Thermal throttling notification ----------" << std::endl;

		std::cout << "name=" << temperature.name
			  << ", type=" << toString(temperature.type)
			  << ", value=" << temperature.value
			  << ", throttling severity=" << toString(temperature.throttlingStatus)
			  << std::endl;

		return Void();
	}
};

static int getTemperatureThresholds(sp<IThermal> thermal)
{
	hidl_vec<TemperatureThreshold> thermal_thresholds;
	ThermalStatus thermal_status;
	Return<void> ret;

	ret = thermal->getTemperatureThresholds(false, TemperatureType::UNKNOWN,
						[&](ThermalStatus status, hidl_vec<TemperatureThreshold> thresholds) {
							thermal_status = status;
							thermal_thresholds = thresholds;
						});

	std::cout << "---------- Thresholds ----------" << std::endl;

	if (ret.isOk() && thermal_status.code == ThermalStatusCode::SUCCESS) {
		for (auto threshold : thermal_thresholds) {
			std::cout << "name=" << threshold.name
				  << ", type=" << toString(threshold.type)
				  << ", hot throttling=" << toString(threshold.hotThrottlingThresholds)
				  << ", cold throttling=" << toString(threshold.coldThrottlingThresholds)
				  << ", VR throttling=" << android::hardware::toString(threshold.vrThrottlingThreshold)
				  << std::endl;
		}
	}

	return 0;
}

static int getCurrentTemperature(sp<IThermal> thermal)
{
	hidl_vec<Temperature> thermal_temperatures;
	ThermalStatus thermal_status;
	Return<void> ret;

	ret = thermal->getCurrentTemperatures(false, TemperatureType::UNKNOWN,
					      [&](ThermalStatus status, hidl_vec<Temperature> temperatures) {
						      thermal_status = status;
						      thermal_temperatures = temperatures;
					      });

	std::cout << "---------- Temperatures ----------" << std::endl;

	if (ret.isOk() && thermal_status.code == ThermalStatusCode::SUCCESS) {
		for (auto temperature : thermal_temperatures) {
			std::cout << "name=" << temperature.name
				  << ", type=" << toString(temperature.type)
				  << ", value=" << temperature.value
				  << ", throttling severity=" << toString(temperature.throttlingStatus)
				  << std::endl;
		}
	}

	return 0;
}

static int getCurrentCoolingDevices(sp<IThermal> thermal)
{
	hidl_vec<CoolingDevice> thermal_coolingdevices;
	ThermalStatus thermal_status;
	Return<void> ret;

	ret = thermal->getCurrentCoolingDevices(false, CoolingType::CPU,
						[&](ThermalStatus status, hidl_vec<CoolingDevice> coolingdevices) {
						      thermal_status = status;
						      thermal_coolingdevices = coolingdevices;
					      });

	std::cout << "---------- Cooling types ----------" << std::endl;

	if (ret.isOk() && thermal_status.code == ThermalStatusCode::SUCCESS) {
		for (auto coolingdevice : thermal_coolingdevices) {
			std::cout << "name=" << coolingdevice.name
				  << ", type=" << toString(coolingdevice.type)
				  << ", value=" << coolingdevice.value
				  << std::endl;
		}
	}

	return 0;
}

static ThermalCallback *registerThermalChangedCallback(sp<IThermal> thermal)
{
	Return<void> ret;
	ThermalStatus thermal_status;
	ThermalCallback *thermalCallback;

	std::cout << "Setting thermal change callback" << std::endl;

	thermalCallback = new(ThermalCallback);
	if (!thermalCallback)
		return NULL;

	ret = thermal->registerThermalChangedCallback(thermalCallback, false, TemperatureType::UNKNOWN,
						      [&](ThermalStatus status) { thermal_status = status; });
	if (!ret.isOk() || (thermal_status.code != ThermalStatusCode::SUCCESS)) {
		std::cout << "Failed to register thermal change callback" << std::endl;
		return NULL;
	}

	return thermalCallback;
}

static int unregisterThermalChangedCallback(sp<IThermal> thermal, ThermalCallback *thermalCallback)
{
	Return<void> ret;
	ThermalStatus thermal_status;

	ret = thermal->unregisterThermalChangedCallback(thermalCallback,
							[&](ThermalStatus status) { thermal_status = status; });
	if (!ret.isOk() || (thermal_status.code != ThermalStatusCode::SUCCESS)) {
		std::cout << "Failed to unregister thermal change callback" << std::endl;
		return 1;
	}

	return 0;
}

static int transportCallback(int fd, __attribute__((unused)) int event,
			     __attribute__((unused)) void *data)
{
	handleTransportPoll(fd);
	return 1;
}

static int setupTransportCallback(Looper *looper)
{
	int fd;

	fd = setupTransportPolling();
	if (fd < 0)
		return -1;

	if (!looper->addFd(fd, 0, Looper::EVENT_INPUT, transportCallback, NULL))
		return -1;

	return 0;
}

int skinTemperatureCallback(int fd, __attribute__((unused)) int event, void *data)
{
	sp<IThermal> *thermal = (typeof(thermal))data;
	hidl_vec<Temperature> thermal_temperatures;
	ThermalStatus thermal_status;
	Return<void> ret;
	uint64_t expirations;

	if (read(fd, &expirations, sizeof(expirations)) < 0) {
		std::cout << "Failed to read signal data" << std::endl;
		return 0;
	}

	ret = (*thermal)->getCurrentTemperatures(true, TemperatureType::SKIN,
						 [&](ThermalStatus status, hidl_vec<Temperature> temperatures) {
							 thermal_status = status;
							 thermal_temperatures = temperatures;
						 });

	std::cout << "---------- Skin temperatures ----------" << std::endl;

	if (ret.isOk() && thermal_status.code == ThermalStatusCode::SUCCESS) {
		for (auto temperature : thermal_temperatures) {
			std::cout << "name=" << temperature.name
				  << ", type=" << toString(temperature.type)
				  << ", value=" << temperature.value
				  << ", throttling severity=" << toString(temperature.throttlingStatus)
				  << std::endl;
		}
	}

	return 1;
}

static int setupSkinTempMonitoring(Looper *looper, sp<IThermal> *thermal)
{
	struct itimerspec iti = {
		.it_interval = { SKIN_POLLING_SEC, 0 },
		.it_value = { SKIN_POLLING_SEC, 0 },
	};

	int timerfd;

	timerfd = timerfd_create(CLOCK_MONOTONIC, 0);
	if (timerfd < 0) {
		std::cout << "Failed to create timer" << std::endl;
		return 1;
	}

	if (timerfd_settime(timerfd, 0, &iti, NULL) < 0) {
		std::cout << "Failed to set timer time" << std::endl;
		return 1;
	}

	if (!looper->addFd(timerfd, 0, Looper::EVENT_INPUT, skinTemperatureCallback, thermal)) {
		std::cout << "Failed to add skin temperature callback to mainloop" << std::endl;
		return 1;
	}

	return 0;
}

int main(int, char *[])
{
	ThermalCallback *thermalCallback;
	class Looper *looper;

	sp<IThermal> thermal;

	looper = new Looper(false);
	if (!looper)
		return 1;

	thermal = IThermal::getService("default");
	if (!thermal) {
		std::cout << "Failed to get thermal service" << std::endl;
		return 1;
	}

	if (setupTransportCallback(looper)) {
		std::cout << "Failed to setup transport callback" << std::endl;
		return 1;
	}

	if (setupSkinTempMonitoring(looper, &thermal)) {
		std::cout << "Failed to setup skin monitoring" << std::endl;
		return 1;
	}

	if (getCurrentTemperature(thermal)) {
		std::cout << "Failed to get temperatures" << std::endl;
		return 1;
	}

	if (getTemperatureThresholds(thermal)) {
		std::cout << "Failed to get thresholds" << std::endl;
		return 1;
	}

	if (getCurrentCoolingDevices(thermal)) {
		std::cout << "Failed to get cooling devices" << std::endl;
		return 1;
	}

	thermalCallback = registerThermalChangedCallback(thermal);
	if (!thermalCallback) {
		std::cout << "Failed to register changed callback" << std::endl;
		return 1;
	}

	std::cout << "Successfully registered thermal change callback" << std::endl;

	std::cout << "Entering mainloop..." << std::endl;

	looper->pollAll(-1);

	std::cout << "Exiting mainloop..." << std::endl;

	if (unregisterThermalChangedCallback(thermal, thermalCallback)) {
		std::cout << "Failed to unregister changed callback" << std::endl;
		return 1;
	}
	std::cout << "Successfully unregistered thermal change callback" << std::endl;

	return 0;
}
