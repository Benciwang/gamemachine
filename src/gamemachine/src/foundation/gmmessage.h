﻿#ifndef __GMMESSAGE_H__
#define __GMMESSAGE_H__
#include <gmcommon.h>
BEGIN_NS

enum class GameMachineMessageType
{
	None,
	QuitGameMachine,
	CrashDown,
	FrameUpdate,
	WindowSizeChanged,
	Dx11Ready,
	SystemMessage,
};

struct GMMessage
{
	GMMessage(GameMachineMessageType t = GameMachineMessageType::None, GMint tp = 0, GMint v = 0, void* objPtr = nullptr)
		: msgType(t)
		, type(tp)
		, value(v)
	{}

	GameMachineMessageType msgType = GameMachineMessageType::None;
	GMint type = 0;
	GMint value = 0;
	void* objPtr = 0;
};

enum class GMSystemEventType
{
	Unknown,
	Destroy,
	WindowSizeChanged,
	KeyDown,
	KeyUp,
	MouseMove,
	MouseDown,
	MouseUp,
	MouseDblClick,
	MouseWheel,
};

enum GMKey
{
	GMKey_Unknown,
	GMKey_Lbutton,
	GMKey_Rbutton,
	GMKey_Cancel,
	GMKey_Mbutton,
	GMKey_Xbutton1,
	GMKey_Xbutton2,
	GMKey_Back,
	GMKey_Tab,
	GMKey_Clear,
	GMKey_Return,
	GMKey_Shift,
	GMKey_Control,
	GMKey_Menu,
	GMKey_Pause,
	GMKey_Capital,
	GMKey_Kana,
	GMKey_Hangul,
	GMKey_Junja,
	GMKey_Final,
	GMKey_Hanja,
	GMKey_Kanji,
	GMKey_Escape,
	GMKey_Convert,
	GMKey_Nonconvert,
	GMKey_Accept,
	GMKey_Modechange,
	GMKey_Space,
	GMKey_Prior,
	GMKey_Next,
	GMKey_End,
	GMKey_Home,
	GMKey_Left,
	GMKey_Up,
	GMKey_Right,
	GMKey_Down,
	GMKey_Select,
	GMKey_Print,
	GMKey_Execute,
	GMKey_Snapshot,
	GMKey_Insert,
	GMKey_Delete,
	GMKey_Help,
	GMKey_Lwin,
	GMKey_Rwin,
	GMKey_Apps,
	GMKey_Sleep,
	GMKey_Numpad0,
	GMKey_Numpad1,
	GMKey_Numpad2,
	GMKey_Numpad3,
	GMKey_Numpad4,
	GMKey_Numpad5,
	GMKey_Numpad6,
	GMKey_Numpad7,
	GMKey_Numpad8,
	GMKey_Numpad9,
	GMKey_Multiply,
	GMKey_Add,
	GMKey_Separator,
	GMKey_Subtract,
	GMKey_Decimal,
	GMKey_Divide,
	GMKey_F1,
	GMKey_F2,
	GMKey_F3,
	GMKey_F4,
	GMKey_F5,
	GMKey_F6,
	GMKey_F7,
	GMKey_F8,
	GMKey_F9,
	GMKey_F10,
	GMKey_F11,
	GMKey_F12,
	GMKey_F13,
	GMKey_F14,
	GMKey_F15,
	GMKey_F16,
	GMKey_F17,
	GMKey_F18,
	GMKey_F19,
	GMKey_F20,
	GMKey_F21,
	GMKey_F22,
	GMKey_F23,
	GMKey_F24,
	GMKey_Numlock,
	GMKey_Scroll,
	GMKey_Lshift,
	GMKey_Rshift,
	GMKey_Lcontrol,
	GMKey_Rcontrol,
	GMKey_Lmenu,
	GMKey_Rmenu,
	GMKey_Browser_back,
	GMKey_Browser_forward,
	GMKey_Browser_refresh,
	GMKey_Browser_stop,
	GMKey_Browser_search,
	GMKey_Browser_favorites,
	GMKey_Browser_home,
	GMKey_Volume_mute,
	GMKey_Volume_down,
	GMKey_Volume_up,
	GMKey_Media_next_track,
	GMKey_Media_prev_track,
	GMKey_Media_stop,
	GMKey_Media_play_pause,
	GMKey_Launch_mail,
	GMKey_Launch_media_select,
	GMKey_Launch_app1,
	GMKey_Launch_app2,
	GMKey_Oem_1,
	GMKey_Oem_plus,
	GMKey_Oem_comma,
	GMKey_Oem_minus,
	GMKey_Oem_period,
	GMKey_Oem_2,
	GMKey_Oem_3,
	GMKey_Oem_4,
	GMKey_Oem_5,
	GMKey_Oem_6,
	GMKey_Oem_7,
	GMKey_Oem_8,
	GMKey_Oem_102,
	GMKey_Processkey,
	GMKey_Packet,
	GMKey_Attn,
	GMKey_Crsel,
	GMKey_Exsel,
	GMKey_Ereof,
	GMKey_Play,
	GMKey_Zoom,
	GMKey_Noname,
	GMKey_Pa1,
	GMKey_Oem_clear,
};

enum GMMouseButtonType
{
	GMMouseButton_None = 0x00,
	GMMouseButton_Left = 0x01,
	GMMouseButton_Right = 0x02,
	GMMouseButton_Middle = 0x04,
};

typedef GMuint GMMouseButton;

enum GMModifierType
{
	GMModifier_None = 0x00,
	GMModifier_Ctrl = 0x01,
	GMModifier_Shift = 0x02
};

typedef GMuint GMModifier;

GM_PRIVATE_OBJECT(GMSystemEvent)
{
	GMMessage message = { GameMachineMessageType::SystemMessage };
	GMSystemEventType type;
};

class GMSystemEvent : public GMObject
{
	DECLARE_PRIVATE(GMSystemEvent)
	GM_ALLOW_COPY_DATA(GMSystemEvent)
	GM_DECLARE_PROPERTY(Type, type, GMSystemEventType);

public:
	GMSystemEvent() = default;
	GMSystemEvent(GMSystemEventType type);
};

GM_PRIVATE_OBJECT(GMSystemKeyEvent)
{
	GMKey key = GMKey_Unknown;
	GMModifier modifier;
};

class GMSystemKeyEvent : public GMSystemEvent
{
	DECLARE_PRIVATE_AND_BASE(GMSystemKeyEvent, GMSystemEvent);
	GM_ALLOW_COPY_DATA(GMSystemKeyEvent)
	GM_DECLARE_PROPERTY(Key, key, GMKey)
	GM_DECLARE_PROPERTY(Modifier, modifier, GMModifier)

public:
	GMSystemKeyEvent(GMSystemEventType type, GMKey key, GMModifier modifier)
		: Base(type)
	{
		D(d);
		d->key = key;
		d->modifier = modifier;
	}
};

GM_PRIVATE_OBJECT(GMSystemMouseEvent)
{
	GMPoint point;
	GMMouseButton buttons; //!< 表示当前事件产生时，鼠标按键的状态。
	GMMouseButton button; //!< 表示引起当前事件的鼠标按键。
	GMModifier modifier;
};

class GMSystemMouseEvent : public GMSystemEvent
{
	DECLARE_PRIVATE_AND_BASE(GMSystemMouseEvent, GMSystemEvent);
	GM_ALLOW_COPY_DATA(GMSystemMouseEvent)
	GM_DECLARE_PROPERTY(Point, point, GMPoint)
	GM_DECLARE_PROPERTY(Button, button, GMMouseButton)
	GM_DECLARE_PROPERTY(Buttons, buttons, GMMouseButton)
	GM_DECLARE_PROPERTY(Modifier, modifier, GMModifier)
	
public:
	GMSystemMouseEvent() = default;
	GMSystemMouseEvent(GMSystemEventType type, const GMPoint& pt, GMMouseButton button, GMMouseButton buttons, GMModifier modifier)
		: Base(type)
	{
		D(d);
		d->point = pt;
		d->button = button;
		d->buttons = buttons;
		d->modifier = modifier;
	}
};

GM_PRIVATE_OBJECT(GMSystemMouseWheelEvent)
{
	GMshort delta;
};

class GMSystemMouseWheelEvent : public GMSystemMouseEvent
{
	DECLARE_PRIVATE_AND_BASE(GMSystemMouseWheelEvent, GMSystemMouseEvent)
	GM_ALLOW_COPY_DATA(GMSystemMouseWheelEvent)
	GM_DECLARE_PROPERTY(Delta, delta, GMshort)

public:
	GMSystemMouseWheelEvent(GMSystemEventType type, const GMPoint& pt, GMMouseButton button, GMMouseButton buttons, GMModifier modifier, GMshort delta)
		: Base(type, pt, button, buttons, modifier)
	{
		D(d);
		d->delta = delta;
	}
};

END_NS
#endif