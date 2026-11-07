#ifndef GAME_FRAMEWORK_H
#define GAME_FRAMEWORK_H

#include <windows.h>

// 윈도우 타이머 대신 프레임워크가 직접 시간을 재서 틱을 발생

// 게임 모드
class GameMode {
public:
    virtual ~GameMode() {}

    virtual void Init() {}
    virtual void Finish() {}
    virtual void Pause() {}
    virtual void Resume() {}

    // 게임 타이머가 만료될 때마다 호출된다.
    virtual void OnTimer(int timerId) {}

    virtual void Update(double frameTime) {}
};

namespace GameFramework {
    // 모드 전환
    void ChangeMode(GameMode* mode);
    void PushMode(GameMode* mode);
    void PopMode();
    void Quit();

    // 메인 루프
    int Run(HWND hWnd, GameMode* startMode);

    // 게임 타이머.
    void StartTimer(int timerId, int intervalMs, bool scaledBySpeed = true);
    void KillTimer(int timerId);

    // 배속
    void SetSpeed(double speed);
    double GetSpeed();

    double GameTime();      // 시작 후 게임 내 경과 시간
    double FrameTime();     // 이전 프레임에서 흐른 시간
}

#endif
