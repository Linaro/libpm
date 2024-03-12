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

#ifndef THERMAL_CONFIG_PARSER_H__
#define THERMAL_CONFIG_PARSER_H__

#include <android/hardware/thermal/2.0/IThermal.h>

#include <map>
#include <string>

#include <json/reader.h>
#include <json/value.h>

namespace android {
namespace hardware {
namespace thermal {
namespace V2_0 {
namespace implementation {

using Temperature_1_0 = ::android::hardware::thermal::V1_0::Temperature;
using Temperature_2_0 = ::android::hardware::thermal::V2_0::Temperature;
using CoolingDevice_1_0 = ::android::hardware::thermal::V1_0::CoolingDevice;
using CoolingDevice_2_0 = ::android::hardware::thermal::V2_0::CoolingDevice;

using ::android::hardware::thermal::V2_0::TemperatureThreshold;
using ::android::hardware::thermal::V2_0::TemperatureType;

class Config {
private:
	template<class T> bool typeToEnum(std::string &type, T &t);

	std::string toUpper(const std::string &str);

	bool read(std::string file);

	void initThreshold(TemperatureThreshold &tempThreshold);

	bool readHotColdThrottling(Json::Value &throttling, float *tempThreshold);

	bool readHotThrottling(Json::Value &throttling,
			       TemperatureThreshold &tempThreshold);

	bool readColdThrottling(Json::Value &throttling,
				TemperatureThreshold &tempThreshold);

	bool readVrThrottling(Json::Value &throttling,
			      TemperatureThreshold &tempThreshold);

	bool readThrottling(const std::string &name,
			    Json::Value &throttling);

	bool readCoolingDevice(Json::Value &coolingDevice);

	bool readSensor(Json::Value &sensor);

	bool parseFile(std::string path, Json::Value &root);

public:
	std::vector<Temperature_1_0> m_temperature_1_0;
	std::vector<Temperature_2_0> m_temperature_2_0;

	std::vector<CoolingDevice_1_0> m_cooling_device_1_0;
	std::vector<CoolingDevice_2_0> m_cooling_device_2_0;

	/*
	 * Each sensor can have associated a configuration for the
	 * threshold, the key is the name of the sensor.
	 */
	std::map<const std::string, TemperatureThreshold> m_threshold;

	/*
	 * Contains the list of the skin temperature sensors.
	 */
	std::vector<const std::string> m_skin_sensors;

	bool init(void);
};

}  // namespace implementation
}  // namespace V2_0
}  // namespace thermal
}  // namespace hardware
}  // namespace android

#endif
