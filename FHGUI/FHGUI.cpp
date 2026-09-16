#include <algorithm>

#include "FHGUI.hpp"
#include "Input.hpp"
#include "Elements/Window.hpp"
#include "Elements/Controls.hpp"

namespace FHGUI
{
	void Instance::Update()
	{
		Input::Get().Update();
		auto& input = Input::Get();
		Control* focused = input.FocusedControl();
		const bool keyboardCaptured = focused && focused->CapturesKeyboard();
		// Evaluate bindings on all tabs, even while the menu is hidden.
		for (auto window : Windows_) if (window) window->UpdateBindings(keyboardCaptured);

		if (!keyboardCaptured && Input::Get().KeyPressed(VK_INSERT)) {
			IsOpen_ = !IsOpen_;
			input.SetFocus(nullptr);
			DraggingWindow_ = nullptr;
		}

		if (!IsOpen_) return;
		if (Windows_.empty()) return;

		CurrentTime_ += 0.01f;

		if (DraggingWindow_ && !Input::Get().KeyDown(VK_LBUTTON)) {
			DraggingWindow_ = nullptr;
		}

		Point CursorPos = Input::Get().CursorPos();

		if (DraggingWindow_) {
			DraggingWindow_->PosX_ = CursorPos.x - DragOffsetX_;
			DraggingWindow_->PosY_ = CursorPos.y - DragOffsetY_;
		}

		bool LeftClick = Input::Get().KeyPressed(VK_LBUTTON);

		// Reverse order matches rendering, including windows with equal timestamps.
		Window* target = nullptr;
		for (auto it = Windows_.rbegin(); it != Windows_.rend(); ++it)
			if (*it && (*it)->HitTest()) { target = *it; break; }

		Window* focusedWindow = nullptr;
		for (auto window : Windows_)
			if (window && focused && focused->BelongsTo(window)) focusedWindow = window;

		// Mouse buttons are valid bind candidates; do not dispatch their capture click.
		if (focusedWindow && keyboardCaptured && dynamic_cast<KeyBind*>(focused)) {
			focusedWindow->Update();
			return;
		}
		if (LeftClick) {
			if (target != focusedWindow) input.SetFocus(nullptr);
			if (target) {
				ActiveWindow_ = target;
				target->LastInputTime_ = CurrentTime_;
				if (input.MouseInArea(target->DragArea())) {
					input.SetFocus(nullptr);
					DragOffsetX_ = CursorPos.x - target->PosX_;
					DragOffsetY_ = CursorPos.y - target->PosY_;
					DraggingWindow_ = target;
					return;
				}
				target->Update();
			}
		} else if (!DraggingWindow_) {
			if (focusedWindow) focusedWindow->Update();
			else if (ActiveWindow_) ActiveWindow_->Update();
		}
	}

	void Instance::Render()
	{
		if (!IsOpen_) return;
		if (Windows_.empty()) return;

		std::stable_sort(Windows_.begin(), Windows_.end(), [](const Window* a, const Window* b) {
			return a->LastInputTime_ < b->LastInputTime_;
			});

		for (std::size_t i = 0; i < Windows_.size(); ++i) {
			Window* pWindow = Windows_[i];
			if (!pWindow) continue;

			pWindow->Render();
		}
	}

	Instance::~Instance()
	{
		for (std::size_t i = 0; i < Windows_.size(); ++i) {
			Window* pWindow = Windows_[i];
			SAFE_DELETE(pWindow);
		}
	}
}
