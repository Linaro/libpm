/*
 * Copyright (C) 2023 The Android Open Source Project
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
#include "PowerHintSession.h"

#include <android-base/logging.h>

namespace aidl::android::hardware::power::impl::linaro {

using ndk::ScopedAStatus;

PowerHintSession::PowerHintSession():
	mPeriodNsAvg(0), mPeriodNsStddev(0),
	mPeriodNsLastUpdate(0) {}

ScopedAStatus PowerHintSession::updateTargetWorkDuration(int64_t targetDurationNs)
{
	LOG(DEBUG) << __func__ << ": PowerHintSesion uuid=" << mUuid
		   << "targetDurationNs=" << targetDurationNs;

	return ScopedAStatus::ok();
}

ScopedAStatus PowerHintSession::reportActualWorkDuration(const std::vector<WorkDuration>& workDuration)
{
	LOG(DEBUG) << __func__ << ": PowerHintSesion uuid=" << mUuid;

	if (workDuration.empty()) {
		LOG(DEBUG) << "Empty work durations reporting";
		return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
	}

	LOG(DEBUG) << "Reported working duration:";

	/*
	 * The workDuration structure gives a couple of information.
	 *
	 * - timeStampNanos : when the work duration measurement was done
	 * - durationNanos : the work duration itself
	 *
	 * In order to optimize the power and the performance, we need
	 * some information we can deduce from the work duration
	 * reporting:
	 *
	 * - Are the work durations periodic ?
	 *
	 *  ... <---WD---><--something else--><---WD--><-nothing-> ...
	 *
	 * - Are the periods between work durations periodic ?
	 *
	 *  ... <-WD-><----nonWD----><---WD---><----nonWD----><--WD--> ...
	 *
	 * There is no documentation telling if the timestamp was read
	 * before or after the measurement of the workload
	 * duration. That is coming from the client side and we don't
	 * have control on how it measures the duration and the
	 * timestamp order. However, dead code reading shows the
	 * report of the duration is called with systemTime().  We can
	 * reasonably assume the timestamp is after the workload
	 * duration measurement.
	 *
	 *  <---WD0--->|<-----nonWD0-----><--WD1-->|<--nonWD1--><-WD2->|
	 *             v                           v                   v
	 *            ts0                         ts1                 ts2
	 *
	 * Information extracted:
	 *
	 * 1. Duration of nonWD0 : (ts1 - WD1) - ts0
	 *                nonWD1 : (ts2 - WD2) - ts1
	 *
	 * 2. If the nonWD durations are periodic, then we can safely
	 *    compute when the next WD will happen and take some
	 *    performance wise decision to increase the workload
	 *    reactivity. For instance, wakeup 100ms before the
	 *    expected next WD and set a cpu/dma zero latency.
	 *
	 * Information infered:
	 *
	 * 1. Target Work duration vs Work duration average
	 *
	 *    Target WD > WDavg --> Increase the performance, decrease the latency
	 *    Target WD < WDavg --> Increase the latency, decrease the performance
	 *
	 */
	for (int i = 0; i < workDuration.size(); i++) {

		int64_t periodNs;

		LOG(DEBUG) << i << ":\t" << workDuration[i].toString();

		if (!mPeriodNsLastUpdate) {
			mPeriodNsLastUpdate = workDuration[i].timeStampNanos;
			continue;
		}

		periodNs = workDuration[i].timeStampNanos - mPeriodNsLastUpdate;
		mPeriodNsLastUpdate = workDuration[i].timeStampNanos;

		pushPeriod(periodNs);
	}

	return ScopedAStatus::ok();
}

ScopedAStatus PowerHintSession::pause()
{
	LOG(DEBUG) << __func__ << ": PowerHintSesion uuid=" << mUuid;

	return ScopedAStatus::ok();
}

ScopedAStatus PowerHintSession::resume()
{
	LOG(DEBUG) << __func__ << ": PowerHintSesion uuid=" << mUuid;

	return ScopedAStatus::ok();
}

ScopedAStatus PowerHintSession::close()
{
	LOG(DEBUG) << __func__ << ": PowerHintSesion uuid=" << mUuid;

	return ScopedAStatus::ok();
}

void PowerHintSession::pushPeriod(int64_t newValue)
{
	const int64_t ema_alpha_val = 64;
	const int64_t ema_alpha_shift = 7;
	int64_t diff;

	if (!mPeriodNsAvg) {
		mPeriodNsAvg = newValue;
		return;
	}

	diff = (newValue - mPeriodNsAvg) * ema_alpha_val;

	mPeriodNsAvg = mPeriodNsAvg + (diff >> ema_alpha_shift);

	LOG(DEBUG) << "Period=" << newValue << ", emaPeriod=" << mPeriodNsAvg;
}

}  // namespace aidl::android::hardware::power::impl::linaro
