#include "GameFramework.h"
#include <chrono>
#include <vector>
#include <algorithm>

#pragma comment(lib, "winmm.lib")

namespace {
    using Clock = std::chrono::steady_clock;

    // 창을 끌고 있는 동안처럼 루프가 오래 멈췄다 돌아왔을 때 틱이 한꺼번에 몰리는 것을 막는다.
    const double MAX_FRAME_TIME = 0.25;
    const int MAX_TICKS_PER_FRAME = 5;

    struct Timer {
        int id;
        double interval;    // 초
        double elapsed;     // 아직 틱으로 바뀌지 않은 경과 시간(초)
        bool scaled;
        unsigned int serial;    // 재시작 여부를 감지하기 위한 번호
    };

    std::vector<GameMode*> modeStack;
    std::vector<Timer> timers;
    unsigned int timerSerial = 0;

    bool running = false;
    int exitCode = 0;
    double speed = 1.0;
    double gameTime = 0.0;
    double frameTime = 0.0;

    Timer* FindTimer(int id) {
        for (Timer& t : timers) {
            if (t.id == id) return &t;
        }
        return nullptr;
    }

    
    bool RunTimers(double dt) {
        struct Due { 
            int id;
            unsigned int serial;
            int count; 
        };
        std::vector<Due> due;

        for (Timer& t : timers) {
            t.elapsed += t.scaled ? dt * speed : dt;
            if (t.elapsed < t.interval) continue;

            int count = (int)(t.elapsed / t.interval);
            t.elapsed -= count * t.interval;
            if (count > MAX_TICKS_PER_FRAME) {
                count = MAX_TICKS_PER_FRAME;
                t.elapsed = 0.0;
            }
            due.push_back({ t.id, t.serial, count });
        }

        bool ticked = false;
        for (const Due& d : due) {
            for (int k = 0; k < d.count; k++) {
                Timer* t = FindTimer(d.id);
                if (!t || t->serial != d.serial) break;
                if (modeStack.empty()) break;

                modeStack.back()->OnTimer(d.id);
                ticked = true;
            }
        }
        return ticked;
    }
}

namespace GameFramework {

    void ChangeMode(GameMode* mode) {
        if (!modeStack.empty()) {
            modeStack.back()->Finish();
            modeStack.pop_back();
        }
        modeStack.push_back(mode);
        mode->Init();
    }

    void PushMode(GameMode* mode) {
        if (!modeStack.empty())
            modeStack.back()->Pause();
        modeStack.push_back(mode);
        mode->Init();
    }

    void PopMode() {
        if (!modeStack.empty()) {
            modeStack.back()->Finish();
            modeStack.pop_back();
        }
        if (!modeStack.empty())
            modeStack.back()->Resume();
    }

    void Quit() {
        running = false;
    }

    int Run(HWND hWnd, GameMode* startMode) {
        running = true;
        exitCode = 0;
        gameTime = 0.0;
        frameTime = 0.0;

        modeStack.clear();
        modeStack.push_back(startMode);
        startMode->Init();

        timeBeginPeriod(1);
        Clock::time_point current = Clock::now();

        while (running) {
            // 이벤트 처리
            MSG msg;
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                if (msg.message == WM_QUIT) {
                    exitCode = (int)msg.wParam;
                    running = false;
                    break;
                }
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            if (!running) break;

            // 시간 계산
            Clock::time_point now = Clock::now();
            frameTime = std::chrono::duration<double>(now - current).count();
            current = now;
            if (frameTime > MAX_FRAME_TIME) frameTime = MAX_FRAME_TIME;
            gameTime += frameTime * speed;

            // 게임 타이머 틱과 프레임 갱신
            bool ticked = RunTimers(frameTime);
            if (!modeStack.empty())
                modeStack.back()->Update(frameTime);

            // 화면 갱신 요청
            if (ticked)
                InvalidateRect(hWnd, NULL, FALSE);

            // 과잉 확인 방지 수면 -> 메시지가 오면 바로 깨어남
            MsgWaitForMultipleObjects(0, NULL, FALSE, 1, QS_ALLINPUT);
        }

        // 스택에 남은 모드를 위에서부터 정리한다
        while (!modeStack.empty()) {
            modeStack.back()->Finish();
            modeStack.pop_back();
        }
        timers.clear();
        timeEndPeriod(1);
        return exitCode;
    }

    void StartTimer(int timerId, int intervalMs, bool scaledBySpeed) {
        if (intervalMs < 1) intervalMs = 1;

        Timer* t = FindTimer(timerId);
        if (t) {
            t->interval = intervalMs / 1000.0;
            t->elapsed = 0.0;
            t->scaled = scaledBySpeed;
            t->serial = ++timerSerial;
        }
        else {
            timers.push_back({ timerId, intervalMs / 1000.0, 0.0, scaledBySpeed, ++timerSerial });
        }
    }

    void KillTimer(int timerId) {
        timers.erase(std::remove_if(timers.begin(), timers.end(),
            [timerId](const Timer& t) { return t.id == timerId; }), timers.end());
    }

    void SetSpeed(double newSpeed) {
        speed = newSpeed < 0.0 ? 0.0 : newSpeed;
    }

    double GetSpeed() { return speed; }
    double GameTime() { return gameTime; }
    double FrameTime() { return frameTime; }
}
