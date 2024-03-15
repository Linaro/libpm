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

#include <android-base/logging.h>

#include <fstream>

#include "Power.h"
#include "PowerHintSession.h"

#include "power.h"
#include "performance.h"

namespace aidl::android::hardware::power::impl::linaro {

using namespace std::chrono_literals;

using ndk::ScopedAStatus;

static const std::vector<Boost> BOOST_RANGE{ndk::enum_range<Boost>().begin(),
                                            ndk::enum_range<Boost>().end()};

static const std::vector<Mode> MODE_RANGE{ndk::enum_range<Mode>().begin(),
                                          ndk::enum_range<Mode>().end()};

#define DEFAULT_MODE static_cast<Mode>(std::numeric_limits<int32_t>::max())
#define DEFAULT_BOOST static_cast<Boost>(std::numeric_limits<int32_t>::max())

/*
  Modes:
  ======

  DOUBLE_TAP_TO_WAKE : This mode indicates that the device is allowed
  to wake up when the screen is tapped twice.

  LOW_POWER : This mode indicates whether low power mode is
  activated. This mode saves power consumption at the expense of
  performance

  SUSTAINED_PERFORMANCE : Sustained performance mode is designed to
  provide consistent performance levels over an extended period of
  time.

  FIXED_PERFORMANCE : Sets the device to a fixed performance level
  that lasts for at least 10 minutes under normal indoor
  conditions. The difference between this mode and
  SUSTAINED_PERFORMANCE mode is that: SUSTAINED_PERFORMANCE sets an
  upper limit on performance for long-term stability;
  FIXED_PERFORMANCE mode sets both an upper and lower limit on
  performance so that any workload running in FIXED_PERFORMANCE mode
  should perform at a repeatable time Completed within.

  VR : VR mode is designed to provide minimal guarantees of
  performance for as long as the device can sustain it.

  LAUNCH : This mode indicates that the application has been launched

  EXPENSIVE_RENDERING : This mode indicates that the device is about
  to enter an expensive rendering cycle.

  INTERACTIVE : This mode indicates that the device is about to
  enter/leave the interactive state or the non-interactive state. The
  non-interactive state may be entered after a period of inactivity in
  order to conserve battery power during such periods of
  inactivity. Typical actions are to turn the device on or off and
  adjust the cpufreq parameters. This function can also call the
  appropriate interface, allowing the kernel to suspend the system
  into a low-power sleep state when entering a non-interactive state,
  and disable low-power suspend when the system is in an interactive
  state. When low-power suspend states are enabled, the kernel can
  suspend the system without a wakelock being held.

  DEVICE_IDLE : This mode indicates that the device is in an idle
  state. For detailed information, please refer to: Optimizing for Low
  Power Mode and Application Standby Mode

  DISPLAY_INACTIVE : This mode means the display is off or still on,
  but optimized for low power consumption.  The following prompt
  options are currently not part of the Android framework, but OEMs
  may choose to implement power/performance optimizations.

  AUDIO_STREAMING_LOW_LATENCY : This mode means low latency audio is
  active

  CAMERA_STREAMING_SECURE : This mode indicates that camera security
  streaming is being initiated

  CAMERA_STREAMING_LOW : This mode indicates that camera
  low-resolution streaming is being started.

  CAMERA_STREAMING_MID : This mode indicates that camera
  mid-resolution streaming is being started.

  CAMERA_STREAMING_HIGH : This mode indicates that camera
  high-resolution streaming is being started


  Boost:
  ======

  INTERACTION: This boost is set when the user interacts with the
  device, for example, a touch screen event is passed in. CPU and GPU
  load may appear quickly, possibly increasing the speed of the CPU,
  memory bus, etc. appropriately. Note that this is different from
  INTERACTIVE mode, which only means that this interaction *may*
  happen, not that it is actively happening.

  DISPLAY_UPDATE_IMMINENT: This boost indicates that the framework may
  provide a new display frame soon. This means that the device should
  ensure that the display processing path is powered on and ready to
  receive that update.

  ML_ACC: This boost indicates that the device is interacting with the
  ML accelerator

  AUDIO_LAUNCH: This boost indicates that the device is setting up an
  audio stream.

  CAMERA_LAUNCH: This boost indicates that the Camera is being
  launched

  CAMERA_SHOT: This boost indicates that the Camera is shooting

*/
bool Power::setBoost_Interaction(bool enabled)
{
	LOG(DEBUG) << "Power::" << __func__ << "("
		   << enabled << ")";

	return true;
}

bool Power::setBoost_DisplayUpdateImminent(bool enabled)
{
	LOG(DEBUG) << "Power::" << __func__ << "("
		   << enabled << ")";

	return true;
}

bool Power::setBoost_MlAcc(bool enabled)
{
	LOG(DEBUG) << "Power::" << __func__ << "("
		   << enabled << ")";

	return true;
}

bool Power::setBoost_AudioLaunch(bool enabled)
{
	LOG(DEBUG) << "Power::" << __func__ << "("
		   << enabled << ")";

	return true;
}

bool Power::setBoost_CameraLaunch(bool enabled)
{
	LOG(DEBUG) << "Power::" << __func__ << "("
		   << enabled << ")";

	return true;
}

bool Power::setBoost_CameraShot(bool enabled)
{
	LOG(DEBUG) << "Power::" << __func__ << "("
		   << enabled << ")";

	return true;
}

std::string Power::getUuid(void)
{
	std::ifstream ifs("/proc/sys/kernel/random/uuid");
	std::ostringstream oss;

	if (!ifs.is_open()) {
		LOG(ERROR) << "Failed to open 'uuid' proc file";
		return "";
	}

	oss << ifs.rdbuf();

	return oss.str();
}

ScopedAStatus Power::setBoost(Boost type, int32_t durationMs)
{
	int ret;
	int duration;
	BoostAction *boostAction;

	if (mBoostActions.find(type) == mBoostActions.end())
		return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);

	LOG(DEBUG) << "Power::setBoost[" << toString(type) << "]"
		   << ", duration= " << durationMs;

	boostAction = mBoostActions[type];

	duration = durationMs ? durationMs : boostAction->mDefaultDurationMs;

	/*
	 * The caller has a knowledge of the duration of the requested
	 * boost. It gives us a value. With a zero value specified in
	 * argument, we check if there is a default value set for this
	 * boost action.
	 *
	 * Without a default duration, the boost will stay enabled
	 * until it is disabled manually, without a
	 * timeout. Obviously, that is not desirable because if the
	 * caller forgets to disable the boost, the power consumption
	 * will be higher until disabled.
	 *
	 * For instance, Boost::INTERACTION or
	 * Boost::DISPLAY_UPDATE_IMMINENT can give a zero duration
	 * without disabling the boost afterwards. It is up to the
	 * power HAL to define a default duration value to prevent a
	 * situation where the system is set with a high power
	 * consumption boost action and stays in this state.
	 */
	if (duration > 0) {

		LOG(DEBUG) << "Set cancelling boost[" << toString(type)
			   << "] action timer with duration="
			   << duration;

		if (!boostAction->mTimer->start(duration * NS_PER_MILLISECOND,
						mCancelBoostAction, boostAction)) {
			LOG(ERROR) << "Failed to set timer for boost " << toString(type);
			return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
		}
	}

	/*
	 * The caller disables the boost. We disable the associated
	 * timer without taking care if it was previously set. In all
	 * cases the result will be the same.  If the timer expires
	 * while we are stopping, the timer 'stop()' function will
	 * take care of flushing the expiration.
	 */
	if (duration < 0) {

		LOG(DEBUG) << "Cancelling boost[" << toString(type) << "]";

		if (!boostAction->mTimer->stop()) {
			LOG(ERROR) << "Failed to cancel timer for boost["
				   << toString(type) << "]";
			return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
		}
	}

	/*
	 * Call the associated boost action function. If the duration
	 * is negative, that means we disable the boost, otherwise we
	 * enable it.
	 */
	if (!std::invoke(boostAction->mAction, this, duration < 0 ? false : true)) {
		LOG(ERROR) << "Boost[" << toString(type) << "] action failed";
		return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
	}

	mCurrentBoost = duration < 0 ? DEFAULT_BOOST : type;

	return ScopedAStatus::ok();
}

bool Power::setMode_DoubleTapToWake(bool enabled)
{
	LOG(DEBUG) << "Power::" << __func__ << "("
		   << enabled << ")";

	return true;
}

bool Power::setMode_LowPower(bool enabled)
{
	LOG(DEBUG) << "Power::" << __func__ << "("
		   << enabled << ")";

	return true;
}

bool Power::setMode_SustainedPerformance(bool enabled)
{
	LOG(DEBUG) << "Power::" << __func__ << "("
		   << enabled << ")";

	return true;
}

bool Power::setMode_FixedPerformance(bool enabled)
{
	LOG(DEBUG) << "Power::" << __func__ << "("
		   << enabled << ")";

	return true;
}

bool Power::setMode_VR(bool enabled)
{
	LOG(DEBUG) << "Power::" << __func__ << "("
		   << enabled << ")";

	return true;
}

bool Power::setMode_Launch(bool enabled)
{
	LOG(DEBUG) << "Power::" << __func__ << "("
		   << enabled << ")";

	if (performance_set_global_latency(enabled ? 0 : INT_MAX)) {
		LOG(ERROR) << "Failed to set global latency";
		return false;
	}

	return true;
}

bool Power::setMode_ExpensiveRendering(bool enabled)
{
	LOG(DEBUG) << "Power::" << __func__ << "("
		   << enabled << ")";

	return true;
}

bool Power::setMode_Interactive(bool enabled)
{
	LOG(DEBUG) << "Power::" << __func__ << "("
		   << enabled << ")";

	return true;
}

bool Power::setMode_DeviceIdle(bool enabled)
{
	LOG(DEBUG) << "Power::" << __func__ << "("
		   << enabled << ")";

	return true;
}

bool Power::setMode_DisplayInactive(bool enabled)
{
	LOG(DEBUG) << "Power::" << __func__ << "("
		   << enabled << ")";

	return true;
}

bool Power::setMode_AudioStreamingLowLatency(bool enabled)
{
	LOG(DEBUG) << "Power::" << __func__ << "("
		   << enabled << ")";

	return true;
}

bool Power::setMode_CameraStreamingSecure(bool enabled)
{
	LOG(DEBUG) << "Power::" << __func__ << "("
		   << enabled << ")";

	return true;
}

bool Power::setMode_CameraStreamingLow(bool enabled)
{
	LOG(DEBUG) << "Power::" << __func__ << "("
		   << enabled << ")";

	return true;
}

bool Power::setMode_CameraStreamingMid(bool enabled)
{
	LOG(DEBUG) << "Power::" << __func__ << "("
		   << enabled << ")";

	return true;
}

bool Power::setMode_CameraStreamingHigh(bool enabled)
{
	LOG(DEBUG) << "Power::" << __func__ << "("
		   << enabled << ")";

	return true;
}

bool Power::setMode_Game(bool enabled)
{
	LOG(DEBUG) << "Power::" << __func__ << "("
		   << enabled << ")";

	return true;
}

bool Power::setMode_GameLoading(bool enabled)
{
	LOG(DEBUG) << "Power::" << __func__ << "("
		   << enabled << ")";

	return true;
}

ScopedAStatus Power::setMode(Mode type, bool enabled)
{
	if (mModeActions.find(type) == mModeActions.end())
		return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);

	LOG(DEBUG) << "Power::setMode[" << toString(type) << "]"
		   << ", enabled=" << enabled;

	if (!std::invoke(mModeActions[type], this, enabled))
		return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);

	mCurrentMode = enabled ? type : DEFAULT_MODE;

	return ScopedAStatus::ok();
}

ScopedAStatus Power::isModeSupported(Mode type, bool *_aidl_return)
{
	*_aidl_return = type >= MODE_RANGE.front() && type <= MODE_RANGE.back();

	LOG(DEBUG) << "Power::isModeSupported(" << toString(type)
		   << ")=" << *_aidl_return << std::endl;

	return ScopedAStatus::ok();
}

ScopedAStatus Power::isBoostSupported(Boost type, bool *_aidl_return)
{
	*_aidl_return = type >= BOOST_RANGE.front() && type <= BOOST_RANGE.back();

	LOG(DEBUG) << "Power::isBoostSupported(" << toString(type)
		   << ")=" << *_aidl_return << std::endl;

	return ScopedAStatus::ok();
}

ScopedAStatus Power::createHintSession(int32_t tgid, int32_t uid, const std::vector<int32_t>& tids,
				       int64_t durationNanos, std::shared_ptr<IPowerHintSession> *_aidl_return)
{
	if (tids.size() == 0) {
		LOG(ERROR) << "Empty thread list (empty)";
		return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
	}

	std::shared_ptr<PowerHintSession> powerHintSession = ndk::SharedRefBase::make<PowerHintSession>();

	powerHintSession->mUuid = getUuid();

	if (powerHintSession->mUuid.empty()) {
		LOG(ERROR) << "Failed to generate uuid for Hint Session";
		return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
	}

	LOG(DEBUG) << "Created hint session tgid=" << tgid
		   << ", uid=" << uid << ", duration=" << durationNanos
		   << ", uuid=" << powerHintSession->mUuid;

	*_aidl_return = powerHintSession;

	return ScopedAStatus::ok();
}

ScopedAStatus Power::getHintSessionPreferredRate(int64_t* outNanoseconds)
{
	/*
	 * Depending on the current mode or boost, we may want a
	 * different update rate.
	 */
	*outNanoseconds = std::chrono::nanoseconds(100ms).count();

	return ScopedAStatus::ok();
}

static std::map<std::string, int> PerfCapableDevices;

static int ForEachPerfDevice(struct performance_handler *perf_handler,
			     const char *device, void *data)
{
	Power *power = (Power *)data;
	int id;

	id = performance_get_device_id(perf_handler, device);
	if (id < 0) {
		LOG(ERROR) << "Failed to get device id";
		return -1;
	}

	PerfCapableDevices[device] = id;

	LOG(DEBUG) << "Device '" << device << "' found with id=%" << id;

	return 0;
}

struct performance_handler *Power::getPerfHandler(Power *power)
{
	return power->perf_handler;
}

Power::Power(Looper *looper)
{
	this->perf_handler = performance_create();
	if (!this->perf_handler) {
		LOG(ERROR) << "Failed to initialize power/performance library";
		throw("Failed to initialize power/performance library");
	}

	if (performance_for_each_device(this->perf_handler,
					ForEachPerfDevice, this)) {
		LOG(ERROR) << "Failed to enumerate performance device";
		throw("Failed to enumerate performance device");
	}

	mCancelBoostAction = new CancelBoostAction();
	if (!mCancelBoostAction) {
		LOG(ERROR) << "Failed to allocate CancelBoostAction object";
		throw("Failed to allocate CancelBoostAction object");
	}

	/*
	 * As the enums are generated from AIDL, there is no value for
	 * out of Mode or Boost value. Let's trick the enum by setting
	 * it to int max
	 */
	mCurrentMode = static_cast<Mode>(std::numeric_limits<int32_t>::max());
	mCurrentBoost = static_cast<Boost>(std::numeric_limits<int32_t>::max());

	/*
	 * At this point we don't care about freeing the resources,
	 * the power class is supposed to be a singleton and with a
	 * life cycle equal to the power service. So if the class is
	 * destroyed, the process associated with as well as the
	 * resources are freed
	 */
	mBoostActions[Boost::INTERACTION]             = new BoostAction(&Power::setBoost_Interaction, 500, looper, this);
	mBoostActions[Boost::DISPLAY_UPDATE_IMMINENT] = new BoostAction(&Power::setBoost_DisplayUpdateImminent, 250, looper, this);
	mBoostActions[Boost::ML_ACC]                  = new BoostAction(&Power::setBoost_MlAcc, 0, looper, this);
	mBoostActions[Boost::AUDIO_LAUNCH]            = new BoostAction(&Power::setBoost_AudioLaunch, 0, looper, this);
	mBoostActions[Boost::CAMERA_LAUNCH]           = new BoostAction(&Power::setBoost_CameraLaunch, 0, looper, this);
	mBoostActions[Boost::CAMERA_SHOT]             = new BoostAction(&Power::setBoost_CameraShot, 0, looper, this);

	mModeActions[Mode::DOUBLE_TAP_TO_WAKE]		= &Power::setMode_DoubleTapToWake;
	mModeActions[Mode::LOW_POWER]			= &Power::setMode_LowPower;
	mModeActions[Mode::SUSTAINED_PERFORMANCE]	= &Power::setMode_SustainedPerformance;
	mModeActions[Mode::FIXED_PERFORMANCE]		= &Power::setMode_FixedPerformance;
	mModeActions[Mode::VR]				= &Power::setMode_VR;
	mModeActions[Mode::LAUNCH]			= &Power::setMode_Launch;
	mModeActions[Mode::EXPENSIVE_RENDERING]		= &Power::setMode_ExpensiveRendering;
	mModeActions[Mode::INTERACTIVE]			= &Power::setMode_Interactive;
	mModeActions[Mode::DEVICE_IDLE]			= &Power::setMode_DeviceIdle;
	mModeActions[Mode::DISPLAY_INACTIVE]		= &Power::setMode_DisplayInactive;
	mModeActions[Mode::AUDIO_STREAMING_LOW_LATENCY]	= &Power::setMode_AudioStreamingLowLatency;
	mModeActions[Mode::CAMERA_STREAMING_SECURE]	= &Power::setMode_CameraStreamingSecure;
	mModeActions[Mode::CAMERA_STREAMING_LOW]	= &Power::setMode_CameraStreamingLow;
	mModeActions[Mode::CAMERA_STREAMING_MID]	= &Power::setMode_CameraStreamingMid;
	mModeActions[Mode::CAMERA_STREAMING_HIGH]	= &Power::setMode_CameraStreamingHigh;
	mModeActions[Mode::GAME]			= &Power::setMode_Game;
	mModeActions[Mode::GAME_LOADING]		= &Power::setMode_GameLoading;
}

}  // namespace aidl::android::hardware::power::impl::linaro
