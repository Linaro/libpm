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
#include "Config.h"

#include <cmath>
#include <iostream>
#include <fstream>

#include <android-base/properties.h>
#include <android-base/logging.h>

namespace android {
namespace hardware {
namespace thermal {
namespace V2_0 {
namespace implementation {

template<class T> bool Config::typeToEnum(std::string &type, T &t)
{
	if (type.empty())
		return false;;

	for (const auto cdt : hidl_enum_range<T>()) {

		const std::string upperType = toString(cdt);

		/*
		 * Let's compare on uppercase only so we can be
		 * comfortable with the syntax used in the
		 * configuration file.
		 */
		if (toUpper(type) != toUpper(upperType))
			continue;

		t = cdt;

		return true;
	}

	return false;
}

std::string Config::toUpper(const std::string &str)
{
	std::string upper_str(str);

	for (std::string::size_type i = 0; i < upper_str.length(); i++)
		upper_str[i] = std::toupper(str[i], std::locale());

	return upper_str;
}

bool Config::readHotColdThrottling(Json::Value &throttling,
				   float *tempThreshold)
{
	if (throttling.empty())
		return false;

	for (const std::string mn : throttling.getMemberNames()) {

		for (const auto ts : hidl_enum_range<V2_0::ThrottlingSeverity>()) {

			const std::string severity = toString(ts);

			if (toUpper(severity) != toUpper(mn))
				continue;

			tempThreshold[(int)ts] = throttling[mn].asFloat();

			LOG(DEBUG) << "Throttling[\"" << severity << "\"] = " <<
				tempThreshold[(int)ts] << "°C";

		}
	}

	return true;
}

bool Config::readColdThrottling(Json::Value &throttling,
				TemperatureThreshold &tempThreshold)
{
	return readHotColdThrottling(throttling,
				     tempThreshold.coldThrottlingThresholds.data());
}

bool Config::readHotThrottling(Json::Value &throttling,
			       TemperatureThreshold &tempThreshold)
{
	return readHotColdThrottling(throttling,
				     tempThreshold.hotThrottlingThresholds.data());
}

bool Config::readVrThrottling(Json::Value &throttling,
			      TemperatureThreshold &tempThreshold)
{
	LOG(DEBUG) << "Reading Virtual Reality configuration";

	if (throttling["None"].empty()) {
		LOG(ERROR) << "Invalid temperature threshold";
		return false;
	}

	tempThreshold.vrThrottlingThreshold = throttling["None"].asFloat();

	LOG(DEBUG) << "Virtual Reality Throtlling: " <<
		tempThreshold.vrThrottlingThreshold << "°C";

	return true;
}

void Config::initThreshold(TemperatureThreshold &tempThreshold)
{
	for (int i = 0; i < tempThreshold.hotThrottlingThresholds.size(); i++)
		tempThreshold.hotThrottlingThresholds[i] = NAN;

	for (int i = 0; i < tempThreshold.coldThrottlingThresholds.size(); i++)
		tempThreshold.coldThrottlingThresholds[i] = NAN;

	tempThreshold.vrThrottlingThreshold = NAN;
}

bool Config::readThrottling(const std::string &name, Json::Value &throttling)
{
	TemperatureThreshold tempThreshold = {
		.name = name,
	};

	initThreshold(tempThreshold);

	if (throttling.empty()) {
		LOG(DEBUG) << "No threshold specified for '" << name << "'";
		return true;
	}

	for (Json::Value::ArrayIndex i = 0; i < throttling.size(); ++i) {

		std::string type = throttling[i]["Type"].asString();

		LOG(DEBUG) << "Getting '" << type << "' throttling configuration";

		if (type == "Hot") {
			if (!readHotThrottling(throttling[i], tempThreshold)) {
				LOG(ERROR) << "Failed to read hot throttling entry";
				return false;
			}
		} else if (type == "Cold") {
			if (!readColdThrottling(throttling[i], tempThreshold)) {
				LOG(ERROR) << "Failed to read cold throttling entry";
				return false;
			}
		} else if (type == "Vr") {
			if (!readVrThrottling(throttling[i], tempThreshold)) {
				LOG(ERROR) << "Failed to read Virtual Reality throttling entry";
				return false;
			}
		} else {
			LOG(ERROR) << "Invalid Throttling type: " << type;
			return false;
		}

		tempThreshold.name = name;
	}

	this->m_threshold.insert(std::pair<std::string,
				 TemperatureThreshold>(name, tempThreshold));

	return true;
}

bool Config::readSensor(Json::Value &sensor)
{
	std::string name = sensor["Name"].asString();
	std::string type = sensor["Type"].asString();

	Temperature_1_0 temperature_1_0;
	Temperature_2_0 temperature_2_0;

	if (name.empty()) {
		LOG(ERROR) << "Missing sensor name section";
		return false;
	}

	temperature_1_0.name = name;
	temperature_2_0.name = name;

	if (typeToEnum(type, temperature_1_0.type))
		m_temperature_1_0.push_back(temperature_1_0);

	if (typeToEnum(type, temperature_2_0.type))
		m_temperature_2_0.push_back(temperature_2_0);

	/*
	 * The skin temperature sensors are special ones and are
	 * stored in a second list for quick access for monitoring
	 */
	if (temperature_1_0.type == V1_0::TemperatureType::SKIN)
		m_skin_sensors.push_back(name);

	LOG(DEBUG) << "Sensor: '" << name << "' / type: " << type;

	if (!readThrottling(name, sensor["Throttling"]))
		return false;

	return true;
}

bool Config::readCoolingDevice(Json::Value &coolingDevice)
{
	std::string name = coolingDevice["Name"].asString();
	std::string type = coolingDevice["Type"].asString();

	CoolingDevice_1_0 coolingDevice_1_0;
	CoolingDevice_2_0 coolingDevice_2_0;

	if (name.empty() || type.empty()) {
		LOG(ERROR) << "Missing Cooling device name/type";
		return false;
	}

	coolingDevice_1_0.name = name;
	coolingDevice_2_0.name = name;

	if (typeToEnum(type, coolingDevice_1_0.type))
		m_cooling_device_1_0.push_back(coolingDevice_1_0);

	if (typeToEnum(type, coolingDevice_2_0.type))
		m_cooling_device_2_0.push_back(coolingDevice_2_0);

	return true;
}

bool Config::parseFile(std::string path, Json::Value &root)
{
	Json::CharReaderBuilder builder;
	std::string strerr;
	std::ifstream ifs;

	LOG(DEBUG) << "Reading configuration file: " << path;

	ifs.open(path);
	if (!ifs) {
		LOG(ERROR) << "Failed to open: " << path;
		return false;
	}

	if (!parseFromStream(builder, ifs, &root, &strerr))  {
		LOG(ERROR) << "Failed to parse JSON config: " << strerr;
		return false;
	}

	LOG(DEBUG) << "Configuration file parsed successfully";

	ifs.close();

	return true;
}

bool Config::read(std::string path)
{
	Json::Value root;
	Json::Value sensors;
	Json::Value coolingDevices;

	if (!parseFile(path, root))
		return false;

	sensors = root["Sensors"];

	for (Json::Value::ArrayIndex i = 0; i < sensors.size(); ++i) {

		if (!readSensor(sensors[i]))
			return false;
        }

	coolingDevices = root["CoolingDevices"];

	for (Json::Value::ArrayIndex i = 0; i < coolingDevices.size(); ++i) {

		if (!readCoolingDevice(coolingDevices[i]))
			return false;
        }

	return true;
}

bool Config::init(void)
{
	std::string property("vendor.thermal.config");
	std::string default_conf("thermal.json");
	std::string path = "/vendor/etc/" + android::base::GetProperty(property, default_conf);

	return read(path);
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace thermal
}  // namespace hardware
}  // namespace android
