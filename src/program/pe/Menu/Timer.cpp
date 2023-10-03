#include "pe/Menu/Timer.h"
#include "al/Library/LiveActor/ActorClippingFunction.h"
#include "al/Library/LiveActor/LiveActor.h"
#include "al/Library/Nerve/NerveUtil.h"
#include "nn/os.h"
#include "patch/code_patcher.hpp"
#include "pe/Menu/UserConfig.h"
#include "pe/Util/Offsets.h"
#include <cstdio>

namespace pe {

Timer* Timer::sInstance = nullptr;

static void shineGrabHook(al::LiveActor* shine)
{
    al::invalidateClipping(shine);
    Timer::sInstance->event(TimerHookType::ShineGrab);
}

static bool shineTickHook(al::LiveActor* actor)
{
    bool isFirstStep = al::isFirstStep(actor);
    if (actor->getNerveKeeper()->getCurrentStep() == 1) {
        Timer::sInstance->event(TimerHookType::ShineTick);
    }
    return isFirstStep;
}

Timer::Timer()
{
    sInstance = this;

    using Patcher = exl::patch::CodePatcher;
    Patcher(offsets::ShineGrabHook).BranchLinkInst((void*)shineGrabHook);
    Patcher(offsets::ShineTickHook).BranchLinkInst((void*)shineTickHook);
}

void Timer::start()
{
    mStartTick = nn::os::GetSystemTick();
    mIsRunning = true;
}

void Timer::stop()
{
    if (mIsRunning) {
        mEndTick = nn::os::GetSystemTick();
        mIsRunning = false;
    }
}

void Timer::reset()
{
    auto now = nn::os::GetSystemTick();
    mStartTick = now;
    mEndTick = now;
    mIsRunning = false;
}

void Timer::event(TimerHookType type)
{
    if (!mIsRunning && getConfig()->mTimerStartType == type)
        start();
    else if (mIsRunning && getConfig()->mTimerEndType == type)
        stop();
}

void Timer::draw()
{
    if (!getConfig()->mTimerEnabled)
        return;

    if (getConfig()->mTimerIsRTA) {
        char buffer[32] { 0 };
        s64 time = (mIsRunning ? nn::os::GetSystemTick().m_tick : mEndTick) - mStartTick;
        s64 seconds = time / nn::os::GetSystemTickFrequency();
        s64 minutes = seconds / 60;
        s64 remainingSeconds = seconds % 60;
        s64 milliseconds = (time % nn::os::GetSystemTickFrequency()) * 1000 / nn::os::GetSystemTickFrequency();
        if (time / nn::os::GetSystemTickFrequency() >= 60)
            snprintf(buffer, 32, "%ld:%02ld.%03ld", minutes, remainingSeconds, milliseconds);
        else
            snprintf(buffer, 32, "%02ld.%03ld", remainingSeconds, milliseconds);

        ImGui::GetForegroundDrawList()->AddText(ImGui::GetIO().Fonts->Fonts[0], getConfig()->mTimerFontSize, getConfig()->mTimerPos, IM_COL32(255, 255, 255, 255), buffer);
    }
}

} // namespace pe
