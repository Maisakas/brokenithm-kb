#include "KeyboardSimulator.hpp"

#include "spdlog/spdlog.h"

#include <Windows.h>

// https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes

HINSTANCE hinst  = LoadLibraryW(L"DD.dll");

typedef int(__stdcall*lpfun_DD_todc)(int);
typedef int(__stdcall*lpfun_DD_btn)(int btn);
typedef int(__stdcall*lpfun_DD_key)(int code, int flag);


lpfun_DD_todc dd_todc = (lpfun_DD_todc)GetProcAddress(hinst, "DD_todc");//VK code to ddcode
lpfun_DD_btn dd_btn = (lpfun_DD_btn)GetProcAddress(hinst, "DD_btn");//Mouse move rel.
lpfun_DD_key dd_key = (lpfun_DD_key)GetProcAddress(hinst, "DD_key");//Keyboard

static constexpr int N_BUTTONS = 32 + 6;

WORD TASOLLER_BTN_MAP[N_BUTTONS] = {};

// top row sensors:             531ywusqomkigeca
// bottom row sensors:          642zxvtrpnljhfdb
// air sensors (down to up):    -=[]\;

WORD YUANCON_BTN_MAP[N_BUTTONS] = {
    '6', '5', '4', '3', '2', '1', 'Z', 'Y',
    'X', 'W', 'V', 'U', 'T', 'S', 'R', 'Q',
    'P', 'O', 'N', 'M', 'L', 'K', 'J', 'I',
    'H', 'G', 'F', 'E', 'D', 'C', 'B', 'A',
    VK_OEM_MINUS, VK_OEM_PLUS, VK_OEM_4,
    VK_OEM_6, VK_OEM_5, VK_OEM_1};

struct KeyboardSimulator::Impl
{
    WORD *m_layout;
    INPUT m_input_buffer[N_BUTTONS];

    uint64_t m_last_keys;
    int m_buffered_keys;

    Impl(KeyboardSimulator::KeyboardSimulatorLayout layout);

    void start();
    void key_down(int i);
    void key_up(int i);
    void end();

    void send(uint64_t keys);
};

KeyboardSimulator::KeyboardSimulator(KeyboardSimulatorLayout layout)
    : m_impl(std::make_unique<Impl>(layout)) {}

KeyboardSimulator::~KeyboardSimulator() = default;

void KeyboardSimulator::send(uint64_t keys)
{
    m_impl->send(keys);
}

void KeyboardSimulator::delay(int millis)
{
    Sleep(millis);
}

KeyboardSimulator::Impl::Impl(KeyboardSimulator::KeyboardSimulatorLayout layout)
    : m_layout(nullptr),
      m_last_keys(0),
      m_buffered_keys(0)
{
    switch (layout)
    {
    case LAYOUT_YUANCON:
        m_layout = YUANCON_BTN_MAP;
        break;
    case LAYOUT_TASOLLER:
        // m_layout = TASOLLER_BTN_MAP;
        // break;
    default:
        m_layout = YUANCON_BTN_MAP;
        break;
    }
          
    if (dd_todc && dd_btn && dd_key)
	{
		int st = dd_btn(0); //DD Initialize
		if(st == 1){
			spdlog::info("DD init OK");
        }
	}
	else {
		spdlog::info("DD init ERROR");
	}

    // Zero out buffer
    for (int i = 0; i < N_BUTTONS; i++)
    {
        m_input_buffer[i].type = INPUT_KEYBOARD;
        m_input_buffer[i].ki.wVk = 0;
        m_input_buffer[i].ki.wScan = 0;
        m_input_buffer[i].ki.dwFlags = 0;
        m_input_buffer[i].ki.time = 0;
        m_input_buffer[i].ki.dwExtraInfo = 0;
    }

    // Send one blank keydown
    start();
    end();
};

void KeyboardSimulator::Impl::send(uint64_t keys)
{
    uint64_t keys_changed = keys ^ m_last_keys;
    uint64_t keys_to_down = keys_changed & keys;
    uint64_t keys_to_up = keys_changed & m_last_keys;

    m_last_keys = keys;

    start();
    for (int i = 0; i < N_BUTTONS; i++)
    {
        if (keys_to_down & 1)
        {
            key_down(i);
        }
        else if (keys_to_up & 1)
        {
            key_up(i);
        }

        keys_to_down >>= 1;
        keys_to_up >>= 1;
    }
    end();
}

void KeyboardSimulator::Impl::key_down(int i)
{
    spdlog::debug("{} Down", m_layout[i]);

    int ddcode = dd_todc(m_layout[i]);
    dd_key(ddcode,1);


}

void KeyboardSimulator::Impl::key_up(int i)
{
    spdlog::debug("{} Up", m_layout[i]);

    int ddcode = dd_todc(m_layout[i]);
    dd_key(ddcode,2);

}
