#include <cstring>
#include "Input.hpp"
#include "Elements/Controls.hpp"

#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))

namespace FHGUI
{
	void Input::Init(HWND hWnd)
	{
		hWnd_ = hWnd;
	}

	void Input::Update()
	{
		Characters_.swap(PendingCharacters_);
		PendingCharacters_.clear();
		EditingKeys_.swap(PendingEditingKeys_);
		PendingEditingKeys_.clear();
		std::memcpy(PrevPressedKeys_, PressedKeys_, sizeof(PrevPressedKeys_));

		bool OutOfFocus = GetForegroundWindow() != hWnd_;

		for (int i = 0; i < MAX_KEYS; ++i) {
			PressedKeys_[i] = !OutOfFocus && (GetAsyncKeyState(i) & 0x8000) != 0;
		}
		if (OutOfFocus) {
			SetFocus(nullptr);
			Characters_.clear();
			EditingKeys_.clear();
		}
	}

	void Input::SetFocus(Control* control)
	{
		if (FocusedControl_ == control) return;
		Control* previous = FocusedControl_;
		FocusedControl_ = control;
		if (previous) previous->OnBlur();
		if (control) control->OnFocus();
	}

	std::string Input::ClipboardText() const
	{
		std::string result;
		if (!OpenClipboard(hWnd_)) return result;
		HANDLE data = GetClipboardData(CF_UNICODETEXT);
		if (data) {
			const wchar_t* text = static_cast<const wchar_t*>(GlobalLock(data));
			if (text) {
				const size_t capacity = GlobalSize(data) / sizeof(wchar_t);
				for (size_t i = 0; i < capacity && text[i]; ++i)
					if (text[i] >= 32 && text[i] <= 126) result += static_cast<char>(text[i]);
				GlobalUnlock(data);
			}
		}
		CloseClipboard();
		return result;
	}

	void Input::CopyText(const std::string& text) const
	{
		HGLOBAL data = GlobalAlloc(GMEM_MOVEABLE, (text.size() + 1) * sizeof(wchar_t));
		if (!data) return;
		wchar_t* target = static_cast<wchar_t*>(GlobalLock(data));
		if (!target) { GlobalFree(data); return; }
		for (size_t i = 0; i < text.size(); ++i) target[i] = static_cast<unsigned char>(text[i]);
		target[text.size()] = 0;
		GlobalUnlock(data);
		if (OpenClipboard(hWnd_)) {
			if (EmptyClipboard() && SetClipboardData(CF_UNICODETEXT, data)) data = nullptr;
			CloseClipboard();
		}
		if (data) GlobalFree(data);
	}

	bool Input::OnWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg) {
		case WM_KILLFOCUS:
			SetFocus(nullptr);
			PendingCharacters_.clear();
			PendingEditingKeys_.clear();
			break;
		case WM_CHAR:
			if (FocusedControl_ && FocusedControl_->CapturesKeyboard()) {
				PendingCharacters_ += static_cast<wchar_t>(wParam);
				return true;
			}
			break;
		case WM_KEYDOWN:
			if (FocusedControl_ && FocusedControl_->CapturesKeyboard()) {
				if (wParam == VK_LEFT || wParam == VK_RIGHT || wParam == VK_HOME ||
					wParam == VK_END || wParam == VK_DELETE)
					PendingEditingKeys_.push_back(static_cast<int>(wParam));
			}
			break;
		case WM_MOUSEMOVE:
			MousePos_ = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
			return true;
		}

		return false;
	}
}
