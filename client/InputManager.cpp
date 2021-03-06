//
// InputManager.cpp
//

#include "InputManager.hpp"
#include <DxLib.h>
#include <boost/timer.hpp>
#include <stdint.h>
#include <assert.h>
#include <cmath>
#include <iostream>

int InputManager::mouse_x_, InputManager::mouse_y_;
double InputManager::pad_x_, InputManager::pad_y_, InputManager::pad_z_;
double InputManager::pad_rx_, InputManager::pad_ry_, InputManager::pad_rz_;
double InputManager::pov_x_, InputManager::pov_y_;
double InputManager::slider1_, InputManager::slider2_;

int InputManager::gamepad_type_ = 0;

int InputManager::static_mouse_right_count_ = 0,
        InputManager::static_mouse_left_count_ = 0,
        InputManager::static_mouse_middle_count_ = 0;

bool InputManager::static_mouse_right_ = false,
        InputManager::static_mouse_left_ = false,
        InputManager::static_mouse_middle_ = false;

bool InputManager::prev_mouse_right_ = false, InputManager::prev_mouse_left_ =
        false, InputManager::prev_mouse_middle_ = false;

int InputManager::static_mouse_wheel_ = 0;

std::array<int, 256> InputManager::static_key_count_ = {{0}};
std::array<int, 256> InputManager::static_key_count_tmp_ = {{0}};

std::array<int, 32> InputManager::static_pad_count_ = {{0}};
std::array<int, 32> InputManager::static_pad_count_tmp_ = {{0}};

// キーバインドの初期設定
int
InputManager::KEYBIND_FORWARD =         KEY_INPUT_W,
InputManager::KEYBIND_BACK =            KEY_INPUT_S,
InputManager::KEYBIND_RIGHT_TRUN =      KEY_INPUT_D,
InputManager::KEYBIND_LEFT_TURN =       KEY_INPUT_A,
InputManager::KEYBIND_JUMP =            KEY_INPUT_SPACE,
InputManager::KEYBIND_CHANGE_SPEED =    KEY_INPUT_LSHIFT,
InputManager::KEYBIND_CHANGE_SPEED2 =   KEY_INPUT_RSHIFT,
InputManager::KEYBIND_TAB =             KEY_INPUT_TAB,
InputManager::KEYBIND_SHIFT =           KEY_INPUT_LSHIFT,
InputManager::KEYBIND_RETURN =          KEY_INPUT_RETURN,
InputManager::KEYBIND_EXIT =            KEY_INPUT_ESCAPE,
InputManager::KEYBIND_REFRESH =         KEY_INPUT_F12,
InputManager::KEYBIND_SCRIPT_MODE =     KEY_INPUT_F8,
InputManager::KEYBIND_SCREEN_SHOT =		KEY_INPUT_P,
InputManager::KEYBIND_ENTER =			KEY_INPUT_Z,

InputManager::KEYBIND_LCTRL = KEY_INPUT_LCONTROL,
InputManager::KEYBIND_RCTRL = KEY_INPUT_RCONTROL,
InputManager::KEYBIND_LALT = KEY_INPUT_LALT,
InputManager::KEYBIND_RALT = KEY_INPUT_RALT,

InputManager::KEYBIND_MOTION_01 = KEY_INPUT_NUMPAD1,
InputManager::KEYBIND_MOTION_02 = KEY_INPUT_NUMPAD2,
InputManager::KEYBIND_MOTION_03 = KEY_INPUT_NUMPAD3,
InputManager::KEYBIND_MOTION_04 = KEY_INPUT_NUMPAD4,
InputManager::KEYBIND_MOTION_05 = KEY_INPUT_NUMPAD5,
InputManager::KEYBIND_MOTION_06 = KEY_INPUT_NUMPAD6,
InputManager::KEYBIND_MOTION_07 = KEY_INPUT_NUMPAD7,
InputManager::KEYBIND_MOTION_08 = KEY_INPUT_NUMPAD8,
InputManager::KEYBIND_MOTION_09 = KEY_INPUT_NUMPAD9,
InputManager::KEYBIND_MOTION_00 = KEY_INPUT_NUMPAD0,

InputManager::PADBIND_JUMP =     PAD_INPUT_1
;

InputManager::InputManager() :
        mouse_right_(static_mouse_right_),
        mouse_left_(static_mouse_left_),
        mouse_middle_(static_mouse_middle_),
        mouse_right_count_(static_mouse_right_count_),
        mouse_left_count_(static_mouse_left_count_),
        mouse_middle_count_(static_mouse_middle_count_),
        mouse_wheel_(static_mouse_wheel_),
        key_count_(static_key_count_tmp_),
        pad_count_(static_pad_count_tmp_)
{

}

void InputManager::Update()
{
    GetMousePoint(&mouse_x_, &mouse_y_);

    if (GetGamepadNum() > 0) {
        DINPUT_JOYSTATE state;
        GetJoypadDirectInputState(DX_INPUT_PAD1, &state);
        pad_x_ = state.X / 1000.0f;
        pad_y_ = state.Y / 1000.0f;
        pad_z_ = state.Z / 1000.0f;
        pad_rx_ = state.Rx / 1000.0f;
        pad_ry_ = state.Ry / 1000.0f;
        pad_rz_ = state.Rz / 1000.0f;
        slider1_ = (32767 - state.Slider[0]) / (32767.0f);
        slider2_ = (32767 - state.Slider[1]) / (32767.0f);

        int pov = GetJoypadPOVState(DX_INPUT_PAD1, 0);
        if (pov >= 0) {
            double rad = ((720 - (pov / 100 + 90)) % 360) / 180.0 * DX_PI;
            pov_x_ = cos(rad);
            pov_y_ = sin(rad);
        } else {
            pov_x_ = pov_y_ = 0;
        }

    } else {
        pad_x_ = pad_y_ = pad_z_ = 0;
        pad_rx_ = pad_ry_ = pad_rz_ = 0;
        pov_x_ = pov_y_ = 0;
    }

    prev_mouse_right_ = static_mouse_right_;
    prev_mouse_left_ = static_mouse_left_;
    prev_mouse_middle_ = static_mouse_middle_;

    int mouse_button = GetMouseInput();
    static_mouse_right_ = mouse_button & MOUSE_INPUT_RIGHT;
    static_mouse_left_ = mouse_button & MOUSE_INPUT_LEFT;
    static_mouse_middle_ = mouse_button & MOUSE_INPUT_MIDDLE;

    if (static_mouse_right_) {
        static_mouse_right_count_++;
    } else {
        static_mouse_right_count_ = 0;
    }

    if (static_mouse_left_) {
        static_mouse_left_count_++;
    } else {
        static_mouse_left_count_ = 0;
    }

    if (static_mouse_middle_) {
        static_mouse_middle_count_++;
    } else {
        static_mouse_middle_count_ = 0;
    }

    static_mouse_wheel_ = GetMouseWheelRotVol();

    char KeyBuf[256];
    GetHitKeyStateAll(KeyBuf);

    for (int i = 0; i < 256; i++) {
        if (KeyBuf[i] == 1) {
            static_key_count_[i]++;
        } else {
            static_key_count_[i] = 0;
        }
        static_key_count_tmp_[i] = static_key_count_[i];
    }

    int pad_state = GetJoypadInputState(DX_INPUT_PAD1);
    for (int i = 0; i < 32; i++) {
        if (((uint32_t)1 << i) & pad_state) {
            static_pad_count_[i]++;
        } else {
            static_pad_count_[i] = 0;
        }
        static_pad_count_tmp_[i] = static_pad_count_[i];
    }
}

void InputManager::operator&=(const InputManager& input)
{
    mouse_right_ &= input.mouse_right_;
    mouse_left_ &= input.mouse_left_;
    mouse_middle_ &= input.mouse_middle_;

    mouse_right_count_ = std::min(mouse_right_count_, input.mouse_right_count_);
    mouse_left_count_ = std::min(mouse_left_count_, input.mouse_left_count_);
    mouse_middle_count_ = std::min(mouse_middle_count_, input.mouse_middle_count_);

    mouse_wheel_ = std::min(mouse_wheel_, input.mouse_wheel_);

    auto it = key_count_.begin();
    auto it2 = input.key_count_.begin();
    for (;it != key_count_.end() && it2 != input.key_count_.end(); ++it, ++it2) {
        *it = std::min(*it, *it2);
    }
}

int InputManager::GetGamepadNum()
{
//    static boost::timer t;
//    if (t.elapsed() > 3.0f) {
//        ReSetupJoypad();
//        t.restart();
//    }
    return GetJoypadNum();
}

int InputManager::GetMouseX() const
{
    return mouse_x_;
}

int InputManager::GetMouseY() const
{
    return mouse_y_;
}

double InputManager::GetGamepadAnalogX() const
{
    return pad_x_;
}

double InputManager::GetGamepadAnalogY() const
{
    return pad_y_;
}

double InputManager::GetGamepadAnalogZ() const
{
    return pad_z_;
}

double InputManager::GetGamepadAnalogRx() const
{
    return pad_rx_;
}

double InputManager::GetGamepadAnalogRy() const
{
    return pad_ry_;
}

double InputManager::GetGamepadAnalogRz() const
{
    return pad_rz_;
}


void InputManager::SetGamepadType(int type)
{
	gamepad_type_ = type;
}

double InputManager::GetGamepadManagedAnalogRx() const
{
	switch (gamepad_type_) {
	case 0:
		return pad_z_;
	case 1:
		return pad_rz_;
	case 2:
		return pad_rz_;
	default:
		assert(0);
		return 0.0;
	}
}

double InputManager::GetGamepadManagedAnalogRy() const
{
	switch (gamepad_type_) {
	case 0:
		return -pad_rz_;
	case 1:
		return slider1_;
	case 2:
		return -pad_z_;
	default:
		assert(0);
		return 0.0;
	}
}

double InputManager::GetGamepadPOVX() const
{
    return pov_x_;
}

double InputManager::GetGamepadPOVY() const
{
    return pov_y_;
}

double InputManager::GetGamepadSlider1() const
{
    return slider1_;
}

double InputManager::GetGamepadSlider2() const
{
    return slider2_;
}

std::pair<int, int> InputManager::GetMousePos() const
{
    return std::pair<int, int>(mouse_x_, mouse_y_);
}

int InputManager::GetMouseWheel() const
{
    return mouse_wheel_;
}

bool InputManager::GetMouseRight() const
{
    return mouse_right_;
}

bool InputManager::GetMouseLeft() const
{
    return mouse_left_;
}

bool InputManager::GetMouseMiddle() const
{
    return mouse_middle_;
}

int InputManager::GetMouseRightCount() const
{
    return mouse_right_count_;
}

int InputManager::GetMouseLeftCount() const
{
    return mouse_left_count_;
}

int InputManager::GetMouseMiddleCount() const
{
    return mouse_middle_count_;
}


bool InputManager::GetPrevMouseRight() const
{
    return prev_mouse_right_;
}

bool InputManager::GetPrevMouseLeft() const
{
    return prev_mouse_left_;
}

bool InputManager::GetPrevMouseMiddle() const
{
    return prev_mouse_middle_;
}

int InputManager::GetKeyCount(int key) const
{
    assert(0 <= key && key < 256);
    return key_count_[key];
}

int InputManager::GetGamepadCount(int key) const
{
    int i = 0;
    for (;key >> (i + 1);i++);
    assert(0 <= i && i < 32);
    return pad_count_[i];
}

void InputManager::CancelMouseWheel()
{
    mouse_wheel_ = 0;
}

void InputManager::CancelMouseRight()
{
    mouse_right_ = false;
    mouse_right_count_ = 0;
}

void InputManager::CancelMouseLeft()
{
    mouse_left_ = false;
    mouse_left_count_ = 0;
}

void InputManager::CancelMouseMiddle()
{
    mouse_middle_ = false;
    mouse_middle_count_ = 0;
}

void InputManager::CancelKeyCount(int key)
{
    if (key >= 0 && key < 256) {
        key_count_[key] = 0;
    }
}

void InputManager::CancelKeyCountAll()
{
    for (int i = 0; i < 255; i++) {
        if (i != KEYBIND_TAB && i != KEYBIND_SHIFT) {
            key_count_[i] = 0;
        }
    }
}
