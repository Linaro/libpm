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

#include <android-base/logging.h>
#include <utils/Looper.h>

#define NS_PER_SECOND		1000000000LL
#define NS_PER_MILLISECOND	1000000LL
#define NS_PER_MICROSECOND	1000LL

using ::android::Looper;
using ::android::LooperCallback;

namespace aidl::android::hardware::power::impl::linaro {

class TimerCallback;

class Timer {
private:
	bool mPeriodic;
	Looper *mLooper;

protected:
	int mTimerFd;
public:
	Timer(Looper *looper, bool periodic);
	bool start(int64_t durationNs, TimerCallback *timerCallback, void *data);
	bool stop();
	bool isRunning();
};

class TimerCallback : public LooperCallback {
public:
	int handleEvent(int fd, int events, void *data);
	virtual int handleTimer(void *data) = 0;
	Timer *mTimer;
};

} // namespace aidl::android::hardware::power::impl::linaro
