#pragma once
#include <Windows.h>
#include <string>
#include <vector>

#include "Datatypes.hpp"
#include "../Singleton.hpp"

namespace FHGUI
{
	class Control;
	constexpr int MAX_KEYS = 256;

	class Input : public Singleton<Input>
	{
		friend struct InputTestDriver;
	public:
		void Init(HWND hWnd);
		void Update();
		bool OnWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		void SetFocus(Control* control);
		Control* FocusedControl() const { return FocusedControl_; }
		bool HasFocus() const { return GetForegroundWindow() == hWnd_; }
		bool KeyDown(int key) const { return key > 0 && key < MAX_KEYS && PressedKeys_[key]; }
		const std::wstring& Characters() const { return Characters_; }
		const std::vector<int>& EditingKeys() const { return EditingKeys_; }
		std::string ClipboardText() const;
		void CopyText(const std::string& text) const;

		bool KeyPressed(int Key)
		{
			return Key > 0 && Key < MAX_KEYS && PressedKeys_[Key] && !PrevPressedKeys_[Key];
		}

		bool KeyHeld(int Key)
		{
			return Key > 0 && Key < MAX_KEYS && PrevPressedKeys_[Key] && PressedKeys_[Key];
		}

		Point CursorPos()
		{
			return MousePos_;
		}

		bool MouseInArea(int left, int top, int right, int bottom)
		{
			return MousePos_.x >= left && MousePos_.y >= top && MousePos_.x < left + right && MousePos_.y < top + bottom;
		}

		bool MouseInArea(const Rect& area)
		{
			return MouseInArea(area.x, area.y, area.w, area.h);
		}

	private:
		HWND hWnd_{ nullptr };
		Control* FocusedControl_{ nullptr };
		std::wstring PendingCharacters_, Characters_;
		std::vector<int> PendingEditingKeys_, EditingKeys_;
		Point MousePos_{ 0, 0 };
		bool PressedKeys_[MAX_KEYS]{ false }, PrevPressedKeys_[MAX_KEYS]{ false };
	};
}
