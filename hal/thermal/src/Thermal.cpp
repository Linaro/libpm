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
#define LOG_TAG "android.hardware.thermal@2.0-service.linaro-generic"

#include <cmath>
#include <set>

#include <android-base/logging.h>
#include <hidl/HidlTransportSupport.h>

#include "Thermal.h"

namespace android {
namespace hardware {
namespace thermal {
namespace V2_0 {
namespace implementation {

using ::android::sp;
using ::android::hardware::interfacesEqual;
using ::android::hardware::thermal::V1_0::ThermalStatus;
using ::android::hardware::thermal::V1_0::ThermalStatusCode;

// -----------------------------------------------------------------------------
// Methods from ::android::hardware::thermal::V1_0::IThermal follow.
// -----------------------------------------------------------------------------

/**
 * Retrieves temperatures in Celsius.
 *
 * @return status Status of the operation. If status code is FAILURE,
 *         the status.debugMessage must be populated with the human-readable
 *         error message.
 * @return temperatures If status code is SUCCESS, it's filled with the
 *         current temperatures. The order of temperatures of built-in
 *         devices (such as CPUs, GPUs and etc.) in the list must be kept
 *         the same regardless the number of calls to this method even if
 *         they go offline, if these devices exist on boot. The method
 *         always returns and never removes such temperatures.
 *
 */
Return<void> Thermal::getTemperatures(getTemperatures_cb _hidl_cb)
{
	ThermalStatus status = { . code = ThermalStatusCode::SUCCESS };

	for (auto &p : this->m_config.m_temperature_1_0) {

		std::string name = p.name;
		int temperature;

		temperature = this->getThermalZonetemp(name);
		if (temperature == INT_MAX) {
			LOG(ERROR) << "Failed to read \"" << name << "\" temperature";
			continue;
		}

		p.currentValue = temperature / 1000;
	}

	_hidl_cb(status, m_config.m_temperature_1_0);

	return Void();
}

/**
 * Retrieves CPU usage information of each core: active and total times
 * in ms since first boot.
 *
 * @return status Status of the operation. If status code is FAILURE,
 *         the status.debugMessage must be populated with the human-readable
 *         error message.
 * @return cpuUsages If status code is SUCCESS, it's filled with the current
 *         CPU usages. The order and number of CPUs in the list must be kept
 *         the same regardless the number of calls to this method.
 *
 */
Return<void> Thermal::getCpuUsages(getCpuUsages_cb _hidl_cb)
{
	ThermalStatus status = { .code = ThermalStatusCode::SUCCESS };
	hidl_vec<CpuUsage> cpuUsages;

	if (m_cpuInfo.CpuUsages(cpuUsages))
		_hidl_cb(status, cpuUsages);

	return Void();
}

/**
 * Retrieves the cooling devices information.
 *
 * @return status Status of the operation. If status code is FAILURE,
 *         the status.debugMessage must be populated with the human-readable
 *         error message.
 * @return devices If status code is SUCCESS, it's filled with the current
 *         cooling device information. The order of built-in cooling
 *         devices in the list must be kept the same regardless the number
 *         of calls to this method even if they go offline, if these devices
 *         exist on boot. The method always returns and never removes from
 *         the list such cooling devices.
 *
 */
Return<void> Thermal::getCoolingDevices(getCoolingDevices_cb _hidl_cb)
{
	ThermalStatus status = { .code = ThermalStatusCode::SUCCESS };

	if (this->m_config.m_cooling_device_1_0.empty()) {
		status.code = ThermalStatusCode::FAILURE;
		status.debugMessage = "No cooling devices";
	}

	_hidl_cb(status, this->m_config.m_cooling_device_1_0);

	return Void();
}

// -----------------------------------------------------------------------------
// Methods from ::android::hardware::thermal::V2_0::IThermal follow.
// -----------------------------------------------------------------------------

/**
 * Retrieves static temperature thresholds in Celsius.
 *
 * @param filterType whether to filter the result for a given type.
 * @param type the TemperatureType such as battery or skin.
 *
 * @return status Status of the operation. If status code is FAILURE,
 *    the status.debugMessage must be populated with a human-readable error message.
 * @return temperatureThresholds If status code is SUCCESS, it's filled with the
 *    temperatures thresholds. The order of temperatures of built-in
 *    devices (such as CPUs, GPUs and etc.) in the list must be kept
 *    the same regardless of the number of calls to this method even if
 *    they go offline, if these devices exist on boot. The method
 *    always returns and never removes such temperatures. The thresholds
 *    are returned as static values and must not change across calls. The actual
 *    throttling state is determined in device thermal mitigation policy/agorithm
 *    which might not be simple thresholds so these values Thermal HAL provided
 *    may not be accurate to detemin the throttling status. To get accurate
 *    throttling status, use getCurrentTemperatures or registerThermalChangedCallback
 *    and listen to the callback.
 *
 */
Return<void> Thermal::getTemperatureThresholds(bool filterType, TemperatureType type,
                                               getTemperatureThresholds_cb _hidl_cb)
{
	ThermalStatus status = { .code = ThermalStatusCode::SUCCESS };
	std::vector<TemperatureThreshold> thresholds;

	for (auto const& p : this->m_config.m_threshold) {

		if (filterType && (type != p.second.type))
			continue;

		thresholds.push_back(p.second);
	}

	if (thresholds.empty()) {
		status.code = ThermalStatusCode::FAILURE;
		status.debugMessage = "No threshold temperature matching the type \"" + toString(type) + "\"";
	}

	_hidl_cb(status, thresholds);

	return Void();
}

/**
 * Retrieves temperatures in Celsius.
 *
 * @param filterType whether to filter the result for a given type.
 * @param type the TemperatureType such as battery or skin.
 *
 * @return status Status of the operation. If status code is FAILURE,
 *    the status.debugMessage must be populated with a human-readable
 *    error message.
 *
 * @return temperatures If status code is SUCCESS, it's filled with the
 *    current temperatures. The order of temperatures of built-in
 *    devices (such as CPUs, GPUs and etc.) in the list must be kept
 *    the same regardless of the number of calls to this method even if
 *    they go offline, if these devices exist on boot. The method
 *    always returns and never removes such temperatures.
 *
 */
Return<void> Thermal::getCurrentTemperatures(bool filterType, TemperatureType type,
                                             getCurrentTemperatures_cb _hidl_cb)
{
	ThermalStatus status = { .code = ThermalStatusCode::SUCCESS };
	std::vector<Temperature_2_0> temperatures;

	for (auto &p : this->m_config.m_temperature_2_0) {

		std::string name = p.name;
		int temperature;

		if (filterType && (type != p.type))
			continue;

		temperature = this->getThermalZonetemp(name);
		if (temperature == INT_MAX) {
			LOG(ERROR) << "Failed to read \"" << name << "\" temperature";
			continue;
		}

		p.value = temperature / 1000;
		temperatures.push_back(p);
	}

	if (temperatures.empty()) {
		status.code = ThermalStatusCode::FAILURE;
		status.debugMessage = "No temperature matching the type \"" + toString(type) + "\"";
	}

	_hidl_cb(status, temperatures);

	return Void();
}

/**
 * Retrieves the cooling devices information.
 *
 * @param filterType whether to filter the result for a given type.
 * @param type the CoolingDevice such as CPU/GPU.
 *
 * @return status Status of the operation. If status code is FAILURE,
 *    the status.debugMessage must be populated with the human-readable
 *    error message.
 * @return devices If status code is SUCCESS, it's filled with the current
 *    cooling device information. The order of built-in cooling
 *    devices in the list must be kept the same regardless of the number
 *    of calls to this method even if they go offline, if these devices
 *    exist on boot. The method always returns and never removes from
 *    the list such cooling devices.
 *
 */
Return<void> Thermal::getCurrentCoolingDevices(bool filterType, CoolingType type,
                                               getCurrentCoolingDevices_cb _hidl_cb)
{
	ThermalStatus status = { .code = ThermalStatusCode::SUCCESS };

	std::vector<CoolingDevice_2_0> cooling_devices;

	for (auto const& p : this->m_config.m_cooling_device_2_0) {

		if (filterType && (type != p.type))
			continue;

		cooling_devices.push_back(p);
	}

	if (cooling_devices.empty()) {
		status.code = ThermalStatusCode::FAILURE;
		status.debugMessage = "No cooling device matching the type \"" + toString(type) + "\"";
	}

	_hidl_cb(status, cooling_devices);

	return Void();
}

/**
 * Register an IThermalChangedCallback, used by the Thermal HAL
 * to receive thermal events when thermal mitigation status changed.
 * Multiple registrations with different IThermalChangedCallback must be allowed.
 * Multiple registrations with same IThermalChangedCallback is not allowed, client
 * should unregister the given IThermalChangedCallback first.
 *
 * @param callback the IThermalChangedCallback to use for receiving
 *    thermal events (nullptr callback will lead to failure with status code FAILURE).
 * @param filterType if filter for given sensor type.
 * @param type the type to be filtered.
 *
 * @return status Status of the operation. If status code is FAILURE,
 *    the status.debugMessage must be populated with a human-readable error message.
 *
 */
Return<void> Thermal::registerThermalChangedCallback(const sp<IThermalChangedCallback> &callback,
                                                     bool filterType, TemperatureType type,
                                                     registerThermalChangedCallback_cb _hidl_cb)
{
	ThermalStatus status = { .code = ThermalStatusCode::SUCCESS };
	std::lock_guard<std::mutex> cbLock(m_callback_mutex);

	if (callback == nullptr) {
		status.code = ThermalStatusCode::FAILURE;
		status.debugMessage = "Invalid thermal changed callback (null)";
		goto out;
	}

	if (std::any_of(m_callbacks.begin(),
			m_callbacks.end(),
			[&](const CallbackSetting& c) {
				return interfacesEqual(c.callback, callback);
			})) {
		status.code = ThermalStatusCode::FAILURE;
		status.debugMessage = "Same callback interface already registered";
		goto out;
	}

	m_callbacks.emplace_back(callback, filterType, type);

	LOG(DEBUG) << "A callback has been registered to ThermalHAL, isFilter: "
		   << filterType << " " << toString(type);
out:
	_hidl_cb(status);
	return Void();
}

/**
 * Unregister an IThermalChangedCallback, used by the Thermal HAL
 * to receive thermal events when thermal mitigation status changed.
 *
 * @param callback the IThermalChangedCallback used for receiving
 *    thermal events (nullptr callback will lead to failure with status code FAILURE).
 *
 * @return status Status of the operation. If status code is FAILURE,
 *    the status.debugMessage must be populated with a human-readable error message.
 *
 */
Return<void> Thermal::unregisterThermalChangedCallback(const sp<IThermalChangedCallback> &callback,
						       unregisterThermalChangedCallback_cb _hidl_cb)
{
	ThermalStatus status = { .code = ThermalStatusCode::SUCCESS };
	std::lock_guard<std::mutex> cbLock(m_callback_mutex);
	std::vector<CallbackSetting>::iterator it;

	if (callback == nullptr) {
		status.code = ThermalStatusCode::FAILURE;
		status.debugMessage = "Invalid thermal changed callback (null)";
		goto out;
	}

	it = std::remove_if(m_callbacks.begin(), m_callbacks.end(),
			    [&](const CallbackSetting& c) {
				    return interfacesEqual(c.callback, callback);
			    });

	if (it == m_callbacks.end()) {
		status.code = ThermalStatusCode::FAILURE;
		status.debugMessage = "The callback was not registered before";
		goto out;
	}

	LOG(DEBUG) << "A callback has been unregistered from ThermalHAL, isFilter: "
		   << (*it).is_filter_type << " Type: " << toString((*it).type);

	m_callbacks.erase(it);
out:
	_hidl_cb(status);

	return Void();
}

/**
 * Notify all subscribers about a trip point crossed the way up or
 * down from the kernel.
 *
 * Please note, the kernel has a bogus behavior regarding the trip
 * point crossed. It can send multiple message for the same event. The
 * consumer of these events should not assume a trip point crossed the
 * way up event will be followed by a trip point crossed the way down
 * event or the next trip point crossed the way up.
 *
 * This bogus behavior is present in kernel < v6.7
 *
 * @param temperature the temperature sensor where the event happen
 *
 */
void Thermal::thermalChangedCallback(Temperature_2_0 &temperature)
{
	for (auto &c : m_callbacks) {
		if (c.is_filter_type && c.type != temperature.type)
			continue;

		c.callback->notifyThrottling(temperature);
	}
}

/**
 * Compute the severity of the throttling given the current
 * temperature for a specific sensor name
 *
 * The configuration file may describe for each temperature a severity
 * level. The function will figure out in which throttling interval
 * falls the temperature given a specified temperature name.
 *
 * @param name the temperature name to lookup in the configuration
 *
 * @param temperature a float giving the current temperature in Celsius
 *
 * @return ThrottlingSeverity the throttling severity which can be one
 * NONE, LIGHT, MODERATE, SEVERE, CRITICAL, EMERGENCY, SHUTDOWN. If no
 * throttling severity is configured for the specified temperature
 * @name, NONE is returned
 *
 */
ThrottlingSeverity Thermal::throttlingSeverity(const std::string &name, float temperature)
{
	TemperatureThreshold threshold;
	ThrottlingSeverity severity = ThrottlingSeverity::NONE;

	if (m_config.m_threshold.find(name) == m_config.m_threshold.end()) {
		LOG(DEBUG) << "No threshold for " << name;
		return severity;
	}

	threshold = m_config.m_threshold[name];

	for (const auto ts : hidl_enum_range<V2_0::ThrottlingSeverity>()) {

		if (std::isnan(threshold.hotThrottlingThresholds[(int)ts]))
			continue;

		if (temperature < threshold.hotThrottlingThresholds[(int)ts])
			break;

		severity = ts;
	}

	LOG(DEBUG) << "Throttle severity=" << toString(severity)
		   << " temp=" << threshold.hotThrottlingThresholds[(int)severity]
		   << " current temp=" << temperature;

	return severity;
}

/**
 * The thermalZoneCreate callback is called when a thermal zone is
 * created. That happens very early at system init. Usually when no
 * listener of the events are there. But it is possible to have
 * thermal zone to be created dynamically when loading a thermal
 * sensor module, in this case the event is emitted.
 *
 * The event is not handled by the thermal HAL.
 *
 * @param name a string containing the name of the thermal zone,
 * please note on some systems this name is not unique
 *
 * @param tzid an integer as unique identifier for the thermal zone
 *
 * @param arg a private pointer passed when registering all the
 * callback. It allows to pass private date from the library user to
 * its own routine
 *
 * @return 0
 *
 */
int Thermal::thermalZoneCreate(const char *name, int tz_id, __attribute__((unused))void *arg)
{
	LOG(DEBUG) << "Thermal zone " << name << "/" << tz_id << " created";

	return 0;
}

/**
 * The thermalZoneDelete callback is called when a thermal zone is
 * deleted. That happens when a sensor module is unloaded in this case
 * the event is emitted from the kernel.
 *
 * The event is not handled by the thermal HAL.
 *
 * @param tzid an integer as unique identifier for the thermal zone
 *
 * @param arg a private pointer passed when registering all the
 * callback. It allows to pass private date from the library user to
 * its own routine
 *
 * @return 0
 *
 */
int Thermal::thermalZoneDelete(int tz_id, __attribute__((unused))void *arg)
{
	LOG(DEBUG) << "Thermal zone " << tz_id << " delete";

	return 0;
}

/**
 * The thermalZoneEnable callback is called when a thermal zone is
 * enabled. This event happens when the thermal zone was disabled and
 * then enabled back. The action can be done from kernel space under
 * some circumstances like a suspend or for any other reason.
 * The userspace can be responsible of enabling / disabling a thermal
 * zone.
 *
 * The event is not handled by the thermal HAL.
 *
 * @param tzid an integer as unique identifier for the thermal zone
 *
 * @param arg a private pointer passed when registering all the
 * callback. It allows to pass private date from the library user to
 * its own routine
 *
 * @return 0
 *
 */
int Thermal::thermalZoneEnable(int tz_id, __attribute__((unused))void *arg)
{
	LOG(DEBUG) << "Thermal zone " << tz_id << " enabled";

	return 0;
}

/**
 * The thermalZoneEnable callback is called when a thermal zone is
 * disabled. This event happens when the thermal zone was disabled and
 * then enabled back. The action can be done from kernel space under
 * some circumstances like a suspend or for any other reason.
 * The userspace can be responsible of enabling / disabling a thermal
 * zone.
 *
 * The event is not handled by the thermal HAL.
 *
 * @param tz_id an integer as unique identifier for the thermal zone
 *
 * @param arg a private pointer passed when registering all the
 * callback. It allows to pass private date from the library user to
 * its own routine
 *
 * @return 0
 *
 */
int Thermal::thermalZoneDisable(int tz_id, __attribute__((unused))void *arg)
{
	LOG(DEBUG) << "Thermal zone " << tz_id << " disabled";

	return 0;
}

/**
 * The tripCrossed callback is called when a trip point is crossed the
 * way up or down. It will invoke the registered HAL
 * ThermalChangedCallback after figuring out the throttling severity
 * given the thermal zone id.
 *
 * @param tz_id an integer as unique identifier for the thermal zone
 *
 * @param trip_id an integer as unique identifier in the thermal zone
 * namespace for the trip point
 *
 * @param temp an integer representing the current temperature in
 * milliCelsius
 *
 * @param thermal a pointer to the current Thermal object
 *
 * @param up a boolean telling the way the trip point was crossed
 *
 * @return 0 on success, -1 in case of error
 *
 */
int Thermal::tripCrossed(int tz_id, int trip_id, int temp, Thermal *thermal, bool up)
{
	std::string name;
	float temperature = temp / 1000.0;

	name = thermal->getThermalZoneName(tz_id);
	if (name.empty()) {
		LOG(ERROR) << "No thermal zone name matching id " << tz_id;
		return -1;
	}

	for (auto &p : thermal->m_config.m_temperature_2_0) {

		if (p.name != name)
			continue;

		p.throttlingStatus = throttlingSeverity(name, temperature);
		p.value = temperature;

		thermal->thermalChangedCallback(p);
	}

	LOG(DEBUG) << "Thermal zone " << tz_id
		   << " (" << name << ")"
		   << " trip crossed the way "
		   << (up ? "up" : "down") << " with trip_id=" << trip_id
		   << ",temp=" << temp;

	return 0;
}

/**
 * The tripHigh callback is called from the thermal library when a
 * trip point is crossed the way up (temperature is above the limit)
 *
 * @param tz_id an integer as unique identifier for the thermal zone
 *
 * @param trip_id an integer as unique identifier in the thermal zone
 * namespace for the trip point
 *
 * @param arg a pointer to a private data
 *
 * @return 0 on success, !0 otherwise
 *
 */
int Thermal::tripHigh(int tz_id, int trip_id, int temp, void *arg)
{
	Thermal *thermal = (typeof(thermal))arg;

	return thermal->tripCrossed(tz_id, trip_id, temp, thermal, true);
}

/**
 * The tripLow callback is called from the thermal library when a trip
 * point is crossed the way down (temperature is below the limit)
 *
 * @param tz_id an integer as unique identifier for the thermal zone
 *
 * @param trip_id an integer as unique identifier in the thermal zone
 * namespace for the trip point
 *
 * @param arg a pointer to a private data
 *
 * @return 0 on success, !0 otherwise
 *
 */
int Thermal::tripLow(int tz_id, int trip_id, int temp, void *arg)
{
	Thermal *thermal = (typeof(thermal))arg;

	return thermal->tripCrossed(tz_id, trip_id, temp, thermal, false);
}

/**
 * The tripAdd callback is called from the thermal library when a trip
 * point is added. This event only happens on very specific platforms,
 * usually ACPI based. It is not known Android platforms with such a
 * behavior yet.
 *
 * This event is not supported by the thermal HAL
 *
 * @param tz_id an integer as unique identifier for the thermal zone
 *
 * @param trip_id an integer as unique identifier in the thermal zone
 * namespace for the trip point
 *
 * @param type an integer giving the type of the trip point
 *
 * @param temp an integer representing in milliCelsius the temperature limit
 *
 * @param hyst an integer giving in milliCelsius the hysteresis value
 * to trigger events
 *
 * @param arg a pointer to a private data
 *
 * @return 0
 *
 */
int Thermal::tripAdd(int tz_id, int trip_id, int type,
		     int temp, int hyst, __attribute__((unused))void *arg)
{
	LOG(DEBUG) << "Trip  " << trip_id << " from thermal zone "
		   << tz_id << " added: type="
		   << type << ", temp=" << temp << " ,hyst=" << hyst;

	return 0;
}

/**
 * The tripChange callback is called from the thermal library when a
 * trip point temperature or a hysteresis is changed. That can happen
 * in the case of writable trip point where the userspace changes in
 * sysfs these values.
 *
 * This can happen often if a thermal manager is monitoring closely
 * the temperature of specific zones with low temperature transitions.
 *
 * At this point, this is not supported by the thermal HAL but the
 * trip crossed events will reflect the change of situation for the
 * thermalChangedCallback consumers.
 *
 * @param tz_id an integer as unique identifier for the thermal zone
 *
 * @param trip_id an integer as unique identifier in the thermal zone
 * namespace for the trip point
 *
 * @param type an integer giving the type of the trip point
 *
 * @param temp an integer representing in milliCelsius the temperature limit
 *
 * @param hyst an integer giving in milliCelsius the hysteresis value
 * to trigger events
 *
 * @param arg a pointer to a private data
 *
 * @return 0
 *
 */
int Thermal::tripChange(int tz_id, int trip_id, int type,
			int temp, int hyst, __attribute__((unused))void *arg)
{
	LOG(DEBUG) << "Trip  " << trip_id << " from thermal zone "
		  << tz_id << " changed: type="
		  << type << ", temp=" << temp << " ,hyst=" << hyst;

	return 0;
}

/**
 * The tripDelete callback is called from the thermal library when a
 * trip point is deleted. This event only happens on very specific
 * platforms, usually ACPI based. It is not known Android platforms
 * with such a behavior yet.
 *
 * This event is not supported by the thermal HAL
 *
 * @param tz_id an integer as unique identifier for the thermal zone
 *
 * @param trip_id an integer as unique identifier in the thermal zone
 * namespace for the trip point
 *
 * @param arg a pointer to a private data
 *
 * @return 0
 *
 */
int Thermal::tripDelete(int tz_id, int trip_id, __attribute__((unused))void *arg)
{
	LOG(DEBUG) << "Trip  " << trip_id << " from thermal zone "
		  << tz_id << " deleted";

	return 0;
}

/**
 * The cdevAdd callback is called from the thermal library when a
 * cooling device is registered in the thermal framework.
 *
 * This event can happen when a specific driver is loaded and
 * registers itself in the thermal framework as a cooling device.
 *
 * The event does not tell if the cooling device is bound to a thermal
 * zone.
 *
 * This event is not supported by the thermal HAL
 *
 * @param name the name of the cooling device
 *
 * @param cdev_id an integer as an unique identifier for the cooling
 * device
 *
 * @param max_state an integer telling the maximum cooling device
 * state, the minimum being implicitly zero
 *
 * @param arg a pointer to a private data
 *
 * @return 0
 *
 */
int Thermal::cdevAdd(const char *name, int cdev_id, int max_state,
		     __attribute__((unused))void *arg)
{
	LOG(DEBUG) << "Cooling device '" << name << "'/" << cdev_id
		  << " max state" << max_state << " created";

	return 0;
}

/**
 * The cdevDelete callback is called from the thermal library when a
 * cooling device is unregistered from the thermal framework.
 *
 * This event can happen when a specific driver is unloaded and
 * unregisters itself from the thermal framework.
 *
 * This event is not supported by the thermal HAL
 *
 * @param cdev_id an integer as an unique identifier for the cooling
 * device
 *
 * @param arg a pointer to a private data
 *
 * @return 0
 *
 */
int Thermal::cdevDelete(int cdev_id, __attribute__((unused))void *arg)
{
	LOG(DEBUG) << "Cooling device " << cdev_id << " deleted";

	return 0;
}

/**
 * The cdevUpdate callback is called from the thermal library during
 * mitigation episode. It gives an update of the current level of
 * cooling effect. The number of events in this case can be high.
 *
 * This event is not supported by the thermal HAL
 *
 * @param cdev_id an integer as an unique identifier for the cooling
 * device
 *
 * @param state the current cooling device state
 *
 * @param arg a pointer to a private data
 *
 * @return 0
 *
 */
int Thermal::cdevUpdate(int cdev_id, int state, __attribute__((unused))void *arg)
{
	LOG(DEBUG) << "Coolling device  " << cdev_id << " state=" << state;

	return 0;
}

/**
 * The govChange callback is called from the thermal library if the
 * mitigation policy is changed. This event results from an userspace
 * action.
 *
 * This event is not supported by the thermal HAL
 *
 * @param tz_id an integer as an unique identifier for the thermal
 * zone
 *
 * @param name a string containing the thermal zone policy name
 *
 * @param arg a pointer to a private data
 *
 * @return 0
 *
 */
int Thermal::govChange(int tz_id, const char *name, __attribute__((unused))void *arg)
{
	LOG(DEBUG) << "Governor change '" << name << "' " << "for thermal zone " << tz_id;

	return 0;
}

/**
 * Process events when they are available on the thermal library. This
 * function will be responsible of having the thermal library invoking
 * the different callback registered in the m_ops.
 *
 * @return 1
 */
int Thermal::handleThermalEvents(void)
{
	int ret;

	ret = thermal_events_handle(m_th, this);
	if (ret)
		LOG(DEBUG) << "Failed to handle thermal event ret=" << ret;

	/*
	 * We don't want to have the file descriptor removed in case
	 * of an error, so let's pretend everything is all right even
	 * if we had an error before
	 */
	return 1;
}

/**
 * This function is wrapper to do the connection between the Looper
 * API and the Thermal object. It keeps the separation for the
 * internals of the objects.
 *
 * @param fd the file description where new data arrived (unused)
 *
 * @param events an incremental identifier (unused)
 *
 * @param data a private data pointer to be passed around (unused)
 *
 * @return 1
 */
int ThermalLooperCallback::handleEvent(__attribute__((unused))int fd,
				       __attribute__((unused))int events, void *data)
{
	Thermal *thermal = (typeof(thermal))data;

	return thermal->handleThermalEvents();
}

Thermal::Thermal(Looper *looper)
{
	LOG(DEBUG) << "Initializing the configuration";

	if (!this->m_config.init())
		throw("ThermalHAL failed to initialize the configuration");

	/*
	 * The Thermal class is inherited from the LibThermal. The
	 * constructor of the Libthermal will initialize itself. We
	 * can safely set the callback after the initialization of the
	 * LibThermal as those are needed when calling the
	 * 'thermal_events_handle()' function, so after setting and
	 * polling the file descriptor.
	 */
        m_ops.events.tz_create   = thermalZoneCreate;
	m_ops.events.tz_delete   = thermalZoneDelete;
	m_ops.events.tz_disable	 = thermalZoneDisable;
	m_ops.events.tz_enable	 = thermalZoneEnable;
	m_ops.events.trip_high	 = tripHigh;
	m_ops.events.trip_low	 = tripLow;
	m_ops.events.trip_add	 = tripAdd;
	m_ops.events.trip_delete = tripDelete;
	m_ops.events.trip_change = tripChange;
	m_ops.events.cdev_add    = cdevAdd;
	m_ops.events.cdev_delete = cdevDelete;
	m_ops.events.cdev_update = cdevUpdate;
	m_ops.events.gov_change  = govChange;

	if (!looper->addFd(thermal_events_fd(this->m_th), 0,
			   Looper::EVENT_INPUT, m_thermalLooperCallback, this))
		throw("Failed to add thermal file descriptor to the mainloop");
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace thermal
}  // namespace hardware
}  // namespace android
