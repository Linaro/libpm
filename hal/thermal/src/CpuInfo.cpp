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
#include <unistd.h>
#include <stdlib.h>

#include <iostream>
#include <iomanip>
#include <fstream>
#include <regex>

#include <hidl/HidlTransportSupport.h>
#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/strings.h>

#include "CpuInfo.h"

namespace android {
namespace hardware {
namespace thermal {
namespace V2_0 {
namespace implementation {

#define CPU_USAGE "/proc/stat"

bool CpuInfo::parseCpuUsagesFileAndAssignUsages(hidl_vec<CpuUsage> &cpuUsages)
{
	std::string line;
	std::ifstream file;
	std::regex regEx("^cpu[0-9](.+)");

	file.open(CPU_USAGE);
	if (!file) {
		LOG(ERROR) << "Failed to open: " << CPU_USAGE;
		return false;
	}

	while (std::getline(file, line)) {

		uint64_t cpuNum, cpuUser, cpuNice, cpuSys, cpuIdle;
		std::vector<std::string> fields;

		if (!regex_match(line, regEx))
			continue;

		fields = android::base::Split(line, " ");

		/*
		 * The /proc/stat file is an ABI and not supposed to
		 * change. We can trust its content and the
		 * format. There is no need to check if the CPU is
		 * online as it will be removed from the file if it is
		 * offline. Consequently, if we can read the usage for
		 * a particular CPU, that means it is in the list,
		 * thus online.
		 *
		 * The values are all positive in the 'stat' file. We
		 * can reasonably assume the kernel is safe and not
		 * giving garbage values, so there is no need to check
		 * if the call to strtol was successful.
		 */
                cpuNum	= strtol(fields[0].substr(3).c_str(), NULL, 10);
                cpuUser	= strtol(fields[1].c_str(), NULL, 10);
                cpuNice	= strtol(fields[2].c_str(), NULL, 10);
                cpuSys	= strtol(fields[3].c_str(), NULL, 10);
                cpuIdle	= strtol(fields[4].c_str(), NULL, 10);

		cpuUsages[cpuNum].name = fields[0];
                cpuUsages[cpuNum].active = cpuUser + cpuNice + cpuSys;
                cpuUsages[cpuNum].total = cpuUser + cpuNice + cpuSys + cpuIdle;
                cpuUsages[cpuNum].isOnline = true;
	}

	file.close();

	return true;
}

bool CpuInfo::CpuUsages(hidl_vec<CpuUsage> &cpuUsages)
{
	if (!m_NrCpus)
		return false;

	/*
	 * Set the vector size to the number of possible CPUs
	 */
	cpuUsages.resize(m_NrCpus);

	/*
	 * Parse the stat file
	 */
	parseCpuUsagesFileAndAssignUsages(cpuUsages);

	return true;
}

CpuInfo::CpuInfo()
{
	m_NrCpus = sysconf(_SC_NPROCESSORS_CONF);
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace thermal
}  // namespace hardware
}  // namespace android
