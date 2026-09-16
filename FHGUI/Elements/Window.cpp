#include "Controls.hpp"
#include "Window.hpp"

#include "../Input.hpp"
#include "../Theme.hpp"

#include "../../Render/D3DFont.hpp"
#include "../../Render/Render.hpp"

namespace FHGUI
{
	constexpr int TAB_HEIGHT = 20;

	void Window::Render()
	{
		Render::Rect(PosX_, PosY_, Width_, Height_, { 0, 0, 0, 255 });
		Render::FilledRectGradient(PosX_ + 1, PosY_ + 1, Width_ - 2, 16, { 40, 40, 40, 255 }, { 12, 12, 12, 255 });
		Render::FilledRect(PosX_ + 1, PosY_ + 17, Width_ - 2, Height_ - 18, { 12, 12, 12, 255 });
		Render::Rect(PosX_ + 8, PosY_ + 18, Width_ - 16, Height_ - 26, { 0, 0, 0, 255 });
		Render::FilledRect(PosX_ + 9, PosY_ + 19, Width_ - 18, Height_ - 28, { 46, 46, 46, 255 });

		if (!Title_.empty()) {
			Render::String(PosX_ + (Width_ / 2), PosY_ + 10, { 255, 255, 255, 255 }, Title_.c_str(), Render::MenuFont, CD3DFONT_CENTERED_X | CD3DFONT_CENTERED_Y);
		}

		const Rect& Area = ClientArea();

		if (!Tabs_.empty()) {
			Render::FilledRect(Area.x, Area.y, Area.w, TAB_HEIGHT, { 57, 57, 57, 255 });
			Render::Line(Area.x, Area.y + TAB_HEIGHT, Area.x + Area.w, Area.y + TAB_HEIGHT, { 0, 0, 0, 255 });

			int OffsetX = 0;

			for (std::size_t i = 0; i < Tabs_.size(); ++i) {
				Tab* pTab = Tabs_[i];
				if (!pTab)
					continue;

				int TabWidth = pTab->GetTitleWidth() + 15;

				Rect TabArea = { Area.x + OffsetX, Area.y, TabWidth, TAB_HEIGHT };
				pTab->Render(TabArea, pTab == SelectedTab_, i == 0);

				OffsetX += TabArea.w;
			}
		}
	}

	void Window::Update()
	{
		if (Tabs_.empty())
			return;
		Control* focused = Input::Get().FocusedControl();
		if (focused && focused->BelongsTo(this) && dynamic_cast<KeyBind*>(focused) && focused->CapturesKeyboard()) {
			if (SelectedTab_) SelectedTab_->Update();
			return;
		}

		if (Input::Get().KeyPressed(VK_LBUTTON)) {
			const Rect& Area = ClientArea();
			int OffsetX = 0;

			for (std::size_t i = 0; i < Tabs_.size(); ++i) {
				Tab* pTab = Tabs_[i];
				if (!pTab)
					continue;

				int TabWidth = pTab->GetTitleWidth() + 15;

				Rect TabArea = { Area.x + OffsetX, Area.y, TabWidth, TAB_HEIGHT };
				if (Input::Get().MouseInArea(TabArea)) {
					Input::Get().SetFocus(nullptr);
					SelectedTab_ = pTab;
				}

				OffsetX += TabArea.w;
			}
		}

		if (SelectedTab_) {
			SelectedTab_->Update();
		}
	}

	bool Window::HitTest()
	{
		Control* focused = Input::Get().FocusedControl();
		return Input::Get().MouseInArea(Area()) || (focused && focused->BelongsTo(this) && focused->IsPopupOpen() && Input::Get().MouseInArea(focused->InputArea()));
	}

	void Window::UpdateBindings(bool suppressed)
	{
		for (auto tab : Tabs_) if (tab) tab->UpdateBindings(suppressed);
	}

	void Tab::UpdateBindings(bool suppressed)
	{
		for (auto control : Controls_) if (control) control->UpdateBinding(suppressed);
	}

	Tab::Tab(const std::string& strTitle) : Title_{ strTitle }
	{
		TitleWidth_ = Render::GetTextSize(Title_.c_str(), Render::MenuFont).w;
	}

	Tab::~Tab() {
		for (size_t i = 0; i < Controls_.size(); ++i) {
			Control* pControl = Controls_[i];
			SAFE_DELETE(pControl);
		}
	}

	void Tab::Render(const Rect& Area, bool Selected, bool FirstTab)
	{
		if (Selected) {
			Render::Rect(Area.x, Area.y, Area.w, Area.h, { 0, 0, 0, 255 });

			int PosX = FirstTab ? Area.x : (Area.x + 1);
			int Width = FirstTab ? (Area.w - 1) : (Area.w - 2);

			Render::FilledRect(PosX, Area.y + 1, Width, Area.h, { 46, 46, 46, 255 });
			Render::FilledRect(PosX, Area.y + 1, Width, 2, Theme::Accent);
		}
		else {
			Render::FilledRect(Area.x, Area.y, Area.w, Area.h, { 24, 24, 24, 255 });
		}

		if (!Title_.empty()) {
			Render::String(Area.x + (Area.w / 2), Area.y + (Area.h / 2), { 255, 255, 255, 255 }, Title_.c_str(), Render::MenuFont, CD3DFONT_CENTERED_X | CD3DFONT_CENTERED_Y);
		}

		if (!Selected || Controls_.empty())
			return;

		Control* focused = Input::Get().FocusedControl();
		for (size_t i = 0; i < Controls_.size(); ++i) {
			Control* pControl = Controls_[i];
			if (pControl && !(pControl == focused && pControl->IsPopupOpen())) {
				pControl->Render();
			}
		}
		if (focused && focused->BelongsTo(this) && focused->IsPopupOpen()) focused->Render();
		if (focused && focused->IsPopupOpen()) return;

		for (size_t i = 0; i < Controls_.size(); ++i) {
			Control* pControl = Controls_[i];
			if (pControl && Input::Get().MouseInArea(pControl->TooltipArea())) {
				pControl->RenderTooltip();
			}
		}
	}

	void Tab::Update()
	{
		if (Controls_.empty())
			return;

		auto& input = Input::Get();
		Control* focused = input.FocusedControl();
		if (input.KeyPressed(VK_TAB) && !(focused && focused->CapturesKeyboard() && focused->Type_ == ControlTypes::KEYBIND)) {
			// Commit any characters received in the same frame before moving focus.
			if (focused && focused->BelongsTo(this)) focused->Update();
			std::vector<Control*> focusable;
			int current = -1;
			for (auto control : Controls_) {
				if (!control || !control->Focusable()) continue;
				if (control == focused) current = static_cast<int>(focusable.size());
				focusable.push_back(control);
			}
			if (!focusable.empty()) {
				int count = static_cast<int>(focusable.size());
				int next = current < 0 ? (input.KeyDown(VK_SHIFT) ? count - 1 : 0) : (current + (input.KeyDown(VK_SHIFT) ? count - 1 : 1)) % count;
				input.SetFocus(focusable[next]);
			}
			return;
		}

		// A listening key bind owns mouse buttons as well as keyboard keys.
		const bool capturingBind = focused && focused->BelongsTo(this) && focused->Type_ == ControlTypes::KEYBIND && focused->CapturesKeyboard();
		for (size_t i = 0; i < Controls_.size(); ++i) {
			Control* pControl = Controls_[i];
			if (pControl) {
				pControl->Update();
			}
		}
		if (capturingBind) return;

		if (Input::Get().KeyPressed(VK_LBUTTON)) {
			focused = input.FocusedControl();
			if (focused && focused->BelongsTo(this) && focused->IsPopupOpen()) {
				if (input.MouseInArea(focused->InputArea())) focused->OnClick();
				else input.SetFocus(nullptr);
				return;
			}
			for (auto it = Controls_.rbegin(); it != Controls_.rend(); ++it) {
				Control* pControl = *it;
				if (pControl && pControl->Focusable() && Input::Get().MouseInArea(pControl->InputArea())) {
					input.SetFocus(pControl);
					pControl->OnClick();
					return;
				}
			}
			input.SetFocus(nullptr);
		}
	}

	void Tab::RegisterControl(Control* pControl)
	{
		pControl->Tab_ = this;
		pControl->Window_ = Window_;
		Controls_.emplace_back(pControl);
	}
}
