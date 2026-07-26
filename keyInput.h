#pragma once
#include <dinput.h>
#include <array>
#include <cstdint>

class keyInput
{
private:
    static inline std::array<uint8_t, 256> key{};
    static inline std::array<uint8_t, 256> preKey{};

public:
    // 毎フレームの最初に1回だけ呼ぶ更新関数
    static void Update(IDirectInputDevice8* keyboard)
    {
        if (!keyboard) return;

        // 前フレームの状態を保存
        preKey = key;

        // 最新の状態を取得
        keyboard->Acquire();
        keyboard->GetDeviceState(static_cast<DWORD>(key.size()), key.data());
    }

    // 押されているか（押しっぱなし）
    static bool IsPress(uint8_t keyCode)
    {
        return (key[keyCode] & 0x80) != 0;
    }

    // 押した瞬間か（トリガー）
    static bool IsTrigger(uint8_t keyCode)
    {
        return (key[keyCode] & 0x80) && !(preKey[keyCode] & 0x80);
    }

    // 離した瞬間か（リリース）
    static bool IsRelease(uint8_t keyCode)
    {
        return !(key[keyCode] & 0x80) && (preKey[keyCode] & 0x80);
    }
};