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
#ifndef ANDROID_HARDWARE_THERMAL_V2_0_THERMAL_H
#define ANDROID_HARDWARE_THERMAL_V2_0_THERMAL_H

#include <android/hardware/thermal/2.0/IThermal.h>
#include <android/hardware/thermal/2.0/IThermalChangedCallback.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

#include <utils/Looper.h>

#include "Config.h"
#include "CpuInfo.h"
#include "LibThermal.h"

namespace android {
namespace hardware {
namespace thermal {
namespace V2_0 {
namespace implementation {

using ::android::sp;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::thermal::V1_0::CpuUsage;
using ::android::hardware::thermal::V2_0::CoolingType;
using ::android::hardware::thermal::V2_0::IThermal;
using CoolingDevice_1_0 = ::android::hardware::thermal::V1_0::CoolingDevice;
using CoolingDevice_2_0 = ::android::hardware::thermal::V2_0::CoolingDevice;
using TemperatureType_1_0 = ::android::hardware::thermal::V1_0::Temperature;
using TemperatureType_2_0 = ::android::hardware::thermal::V2_0::Temperature;
using ::android::hardware::thermal::V2_0::IThermalChangedCallback;
using ::android::hardware::thermal::V2_0::TemperatureThreshold;
using ::android::hardware::thermal::V2_0::TemperatureType;

struct CallbackSetting {
    CallbackSetting(sp<IThermalChangedCallback> callback, bool is_filter_type, TemperatureType type)
	    : callback(std::move(callback)), is_filter_type(is_filter_type), type(type) {}
	sp<IThermalChangedCallback> callback;
	bool is_filter_type;
	TemperatureType type;
};

class ThermalLooperCallback : public LooperCallback {

public:
	int handleEvent(int fd, int events, void* data);
};

class Thermal : public LibThermal, public IThermal {
   public:
	// Methods from ::android::hardware::thermal::V1_0::IThermal follow.
	Return<void> getTemperatures(getTemperatures_cb _hidl_cb) override;

	Return<void> getCpuUsages(getCpuUsages_cb _hidl_cb) override;

	Return<void> getCoolingDevices(getCoolingDevices_cb _hidl_cb) override;

	// Methods from ::android::hardware::thermal::V2_0::IThermal follow.
	Return<void> getCurrentTemperatures(bool filterType, TemperatureType type,
					    getCurrentTemperatures_cb _hidl_cb) override;

	Return<void> getTemperatureThresholds(bool filterType, TemperatureType type,
					      getTemperatureThresholds_cb _hidl_cb) override;

	Return<void> registerThermalChangedCallback(const sp<IThermalChangedCallback> &callback,
						    bool filterType, TemperatureType type,
						    registerThermalChangedCallback_cb _hidl_cb) override;

	Return<void> unregisterThermalChangedCallback(const sp<IThermalChangedCallback> &callback,
						      unregisterThermalChangedCallback_cb _hidl_cb) override;

	Return<void> getCurrentCoolingDevices(bool filterType, CoolingType type,
					      getCurrentCoolingDevices_cb _hidl_cb) override;

	int handleThermalEvents(void);

	Thermal(Looper *);

private:
	static int thermalZoneDelete(int, void *);
	static int thermalZoneCreate(const char *, int, void *);
	static int thermalZoneEnable(int tz_id, void *arg);
	static int thermalZoneDisable(int tz_id, void *arg);
	static int tripHigh(int tz_id, int trip_id, int temp, void *arg);
	static int tripLow(int tz_id, int trip_id, int temp, void *arg);
	static int tripAdd(int tz_id, int trip_id, int type,
			   int temp, int hyst, void *arg);

	static int tripChange(int tz_id, int trip_id, int type,
			      int temp, int hyst, void *arg);
	static int tripDelete(int tz_id, int trip_id, void *arg);
	static int cdevAdd(const char *name, int cdev_id, int max_state, void *arg);
	static int cdevDelete(int cdev_id, void *arg);
	static int cdevUpdate(int cdev_id, int state, void *arg);
	static int govChange(int tz_id, const char *name, void *arg);

	sp<ThermalLooperCallback> m_thermalLooperCallback;

	void thermalChangedCallback(Temperature_2_0 &temperature);

	ThrottlingSeverity throttlingSeverity(const std::string &name, float temperature);

	int tripCrossed(int tz_id, int trip_id, int temp, Thermal *thermal, bool up);

	Config m_config;
	CpuInfo m_cpuInfo;

	std::mutex m_callback_mutex;
	std::vector<CallbackSetting> m_callbacks;
};

}  // namespace implementation
}  // namespace V2_0
}  // namespace thermal
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_THERMAL_V2_0_THERMAL_H
