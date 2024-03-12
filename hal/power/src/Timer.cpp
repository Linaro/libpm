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
#define LOG_TAG "android.hardware.power@1.3-service.linaro-generic"

#include <sys/poll.h>
#include <sys/timerfd.h>

#include <iostream>

#include <android-base/logging.h>

#include "Timer.h"

namespace aidl::android::hardware::power::impl::linaro {

int TimerCallback::handleEvent(int fd, __attribute__((unused))int events, void *data)
{
	struct itimerspec iti;
	uint64_t expirations;

	if (read(fd, &expirations, sizeof(expirations)) < 0) {
                LOG(DEBUG) << "Failed to read timer data";
                return 0;
        }

	LOG(DEBUG) << "Handling timer id=" << fd
		   << ", expirations=" << expirations;

	if (!handleTimer(data)) {
		mTimer->stop();
		return 0;
	}

	return 1;
}

Timer::Timer(Looper *looper, bool periodic): mPeriodic(periodic), mLooper(looper)
{
	mTimerFd = timerfd_create(CLOCK_MONOTONIC, 0);
	if (mTimerFd < 0)
                throw("Failed to create timer");

	LOG(DEBUG) << "Created timer id=" << mTimerFd;
}

bool Timer::start(int64_t durationNs, TimerCallback *timerCallback, void *data)
{
	struct timespec ts = {
		.tv_sec = durationNs / NS_PER_SECOND,
		.tv_nsec = durationNs % NS_PER_SECOND,
	};

        struct itimerspec iti = {};

	if (mPeriodic)
		iti.it_interval = ts;

	iti.it_value = ts;

        if (timerfd_settime(mTimerFd, 0, &iti, NULL) < 0) {
                LOG(DEBUG) << "Failed to set timer duration";
                return false;
        }

        if (!mLooper->addFd(mTimerFd, 0, Looper::EVENT_INPUT,
			     timerCallback, data)) {
                LOG(DEBUG) << "Failed to add skin temperature callback to mainloop";
                return false;
        }

	timerCallback->mTimer = this;

	LOG(DEBUG) << "Started timer id=" << mTimerFd
		   << " with duration=" << durationNs << " nsec";

	return true;
}

bool Timer::stop(void)
{
	uint64_t expirations;

	struct itimerspec iti = {
                .it_interval	= { 0, 0 },
                .it_value	= { 0, 0 },
        };

	struct pollfd pfd {
		.fd = mTimerFd,
		.events = 0,
	};

        if (timerfd_settime(mTimerFd, 0, &iti, NULL) < 0) {
                LOG(DEBUG) << "Failed to stop timer";
                return false;
        }

	if (!mLooper->removeFd(mTimerFd)) {
		LOG(DEBUG) << "Failed to remove file descriptor from looper";
		return false;
	}

	/*
	 * If the timer expired while stopping it, we want the event
	 * to be consumed from the file descriptor to prevent an
	 * immediate timeout the next time the timer is started.
	 */
	if (poll(&pfd, 1, 0) > 0) {

		LOG(DEBUG) << "Timer expired before stopping it, flushing data";

		if (read(mTimerFd, &expirations, sizeof(expirations)) < 0) {
			LOG(DEBUG) << "Failed to read timer data";
			return 0;
		}
	}

	LOG(DEBUG) << "Stopped timer id=" << mTimerFd;

	return true;
}

bool Timer::isRunning(void)
{
        struct itimerspec iti = {};

	if (timerfd_gettime(mTimerFd, &iti)) {
		LOG(DEBUG) << "Failed to get timer time";
		return false;
	}

	return (iti.it_value.tv_sec || iti.it_value.tv_sec);
}

}  // namespace aidl::android::hardware::power::impl::linaro
