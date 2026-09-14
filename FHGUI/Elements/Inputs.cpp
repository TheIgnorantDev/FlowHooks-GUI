#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <utility>

#include "Controls.hpp"
#include "../Input.hpp"
#include "../Theme.hpp"
#include "../../Render/D3DFont.hpp"

namespace FHGUI
{
	namespace
	{
		constexpr int LabelHeight = 14, RowHeight = 18;
		constexpr Render::Color Text{ 230, 230, 230, 255 };
		template<class T> T Clamp(T value, T minimum, T maximum) { return (std::max)(minimum, (std::min)(maximum, value)); }
		int TextWidth(const std::string& text) { return Render::GetTextSize(text.c_str(), Render::MenuFont).w; }
		void DrawText(int x, int y, std::string text, int width, Render::Color color = Text)
		{
			while (!text.empty() && TextWidth(text) > width) text.pop_back();
			Render::String(x, y, color, text.c_str(), Render::MenuFont);
		}
		void Frame(const Rect& area, bool focused)
		{
			Render::FilledRect(area.x, area.y, area.w, area.h, { 24, 24, 24, 255 });
			Render::Rect(area.x, area.y, area.w, area.h, focused ? Theme::Accent : Render::Color{ 0, 0, 0, 255 });
		}
		std::string Number(double value)
		{
			std::ostringstream stream;
			stream << std::setprecision(6) << value;
			return stream.str();
		}
		std::vector<std::string> CopyItems(const std::vector<const char*>& items)
		{
			std::vector<std::string> result;
			for (auto item : items) result.emplace_back(item ? item : "");
			return result;
		}
		bool Activate() { return Input::Get().KeyPressed(VK_SPACE) || Input::Get().KeyPressed(VK_RETURN); }
	}

	TextBox::TextBox(const char* title, std::string* value, size_t maxLength, int width, const char* tooltip)
		: Control(title, 0, 0, (std::max)(60, width), 34, ControlTypes::TEXTBOX, tooltip), Value_(value), MaxLength_(maxLength) {}

	Rect TextBox::FieldArea()
	{
		Rect area = AbsoluteArea();
		area.y += LabelHeight;
		area.h -= LabelHeight;
		return area;
	}

	void TextBox::OnFocus()
	{
		Original_ = Value_ ? *Value_ : "";
		Caret_ = Anchor_ = Original_.size();
		ViewStart_ = 0;
	}

	void TextBox::Cancel() { if (Value_) *Value_ = Original_; }

	void TextBox::DeleteSelection()
	{
		if (!Value_) return;
		Caret_ = (std::min)(Caret_, Value_->size());
		Anchor_ = (std::min)(Anchor_, Value_->size());
		size_t start = (std::min)(Caret_, Anchor_);
		Value_->erase(start, (std::max)(Caret_, Anchor_) - start);
		Caret_ = Anchor_ = start;
	}

	void TextBox::Insert(const std::string& text)
	{
		if (!Value_) return;
		DeleteSelection();
		const size_t available = MaxLength_ > Value_->size() ? MaxLength_ - Value_->size() : 0;
		const std::string addition = text.substr(0, available);
		Value_->insert(Caret_, addition);
		Caret_ += addition.size();
		Anchor_ = Caret_;
	}

	void TextBox::OnClick()
	{
		if (!Value_) return;
		const Rect field = FieldArea();
		if (!Input::Get().MouseInArea(field)) return;
		ViewStart_ = (std::min)(ViewStart_, Value_->size());
		size_t position = ViewStart_;
		int x = field.x + 4;
		while (position < Value_->size()) {
			int width = TextWidth(Value_->substr(position, 1));
			if (Input::Get().CursorPos().x < x + width / 2) break;
			x += width;
			++position;
		}
		Caret_ = position;
		if (!Input::Get().KeyDown(VK_SHIFT)) Anchor_ = Caret_;
	}

	void TextBox::Update()
	{
		if (!Focused() || !Value_) return;
		auto& input = Input::Get();
		Caret_ = (std::min)(Caret_, Value_->size());
		Anchor_ = (std::min)(Anchor_, Value_->size());
		if (input.KeyPressed(VK_ESCAPE)) { Cancel(); input.SetFocus(nullptr); return; }
		if (input.KeyDown(VK_CONTROL)) {
			if (input.KeyPressed('A')) { Anchor_ = 0; Caret_ = Value_->size(); }
			if (input.KeyPressed('C') || input.KeyPressed('X')) {
				if (Caret_ != Anchor_) input.CopyText(Value_->substr((std::min)(Caret_, Anchor_), (std::max)(Caret_, Anchor_) - (std::min)(Caret_, Anchor_)));
				if (input.KeyPressed('X')) DeleteSelection();
			}
			if (input.KeyPressed('V')) Insert(input.ClipboardText());
		}
		for (int key : input.EditingKeys()) {
			if (key == VK_DELETE) {
				if (Caret_ != Anchor_) DeleteSelection();
				else if (Caret_ < Value_->size()) Value_->erase(Caret_, 1);
				continue;
			}
			if (key == VK_HOME) Caret_ = 0;
			if (key == VK_END) Caret_ = Value_->size();
			if (key == VK_LEFT) Caret_ = !input.KeyDown(VK_SHIFT) && Caret_ != Anchor_ ? (std::min)(Caret_, Anchor_) : (Caret_ ? Caret_ - 1 : 0);
			if (key == VK_RIGHT) Caret_ = !input.KeyDown(VK_SHIFT) && Caret_ != Anchor_ ? (std::max)(Caret_, Anchor_) : (std::min)(Caret_ + 1, Value_->size());
			if (!input.KeyDown(VK_SHIFT)) Anchor_ = Caret_;
		}
		for (wchar_t character : input.Characters()) {
			if (character == L'\r') { Commit(); input.SetFocus(nullptr); return; }
			if (character == L'\b') {
				if (Caret_ != Anchor_) DeleteSelection();
				else if (Caret_) { Value_->erase(--Caret_, 1); Anchor_ = Caret_; }
			} else if (character >= 32 && character <= 126) Insert(std::string(1, static_cast<char>(character)));
		}
		if (input.KeyPressed(VK_RETURN)) { Commit(); input.SetFocus(nullptr); }
	}

	void TextBox::Render()
	{
		Rect area = AbsoluteArea(), field = FieldArea();
		DrawText(area.x, area.y, Title_, area.w);
		Frame(field, Focused());
		if (!Value_) return;
		Caret_ = (std::min)(Caret_, Value_->size());
		Anchor_ = (std::min)(Anchor_, Value_->size());
		ViewStart_ = (std::min)(ViewStart_, Caret_);
		while (ViewStart_ < Caret_ && TextWidth(Value_->substr(ViewStart_, Caret_ - ViewStart_)) > field.w - 10) ++ViewStart_;
		std::string visible = Value_->substr(ViewStart_);
		while (!visible.empty() && TextWidth(visible) > field.w - 8) visible.pop_back();
		if (Focused()) {
			size_t start = Clamp((std::min)(Caret_, Anchor_), ViewStart_, ViewStart_ + visible.size());
			size_t end = Clamp((std::max)(Caret_, Anchor_), ViewStart_, ViewStart_ + visible.size());
			int left = TextWidth(Value_->substr(ViewStart_, start - ViewStart_));
			int width = TextWidth(Value_->substr(start, end - start));
			if (width) Render::FilledRect(field.x + 4 + left, field.y + 2, width, field.h - 4, Theme::Selection());
			if ((GetTickCount64() / 500) % 2 == 0) {
				int x = field.x + 4 + TextWidth(Value_->substr(ViewStart_, Caret_ - ViewStart_));
				Render::Line(x, field.y + 3, x, field.y + field.h - 3, Theme::Accent);
			}
		}
		DrawText(field.x + 4, field.y + 3, visible, field.w - 8);
	}

	NumberInput::NumberInput(const char* title, float* value, float minimum, float maximum, float step, int width, const char* tooltip)
		: TextBox(title, nullptr, 32, (std::max)(100, width) - 40, tooltip), Number_(value),
		Minimum_(std::isfinite(minimum) ? minimum : 0.0f), Maximum_(std::isfinite(maximum) ? maximum : Minimum_),
		Step_(std::isfinite(step) && step > 0 ? step : 1.0f)
	{
		if (Minimum_ > Maximum_) std::swap(Minimum_, Maximum_);
		Width_ += 40;
		Type_ = ControlTypes::NUMBERINPUT;
		Value_ = &Buffer_;
		Sync();
	}

	void NumberInput::Sync() { Buffer_ = Number_ ? Number(*Number_) : ""; }
	void NumberInput::OnFocus() { Sync(); TextBox::OnFocus(); }
	void NumberInput::Commit()
	{
		if (!Number_) return;
		char* end = nullptr;
		float value = std::strtof(Buffer_.c_str(), &end);
		if (end != Buffer_.c_str() && *end == '\0' && std::isfinite(value)) *Number_ = Clamp(value, Minimum_, Maximum_);
		Sync();
	}
	void NumberInput::Cancel() { Sync(); }
	void NumberInput::OnBlur() { Commit(); }
	Rect NumberInput::FieldArea()
	{
		Rect field = TextBox::FieldArea();
		field.w -= 40;
		return field;
	}
	void NumberInput::Render()
	{
		if (!Focused()) Sync();
		TextBox::Render();
		Rect field = TextBox::FieldArea();
		for (int i = 0; i < 2; ++i) {
			Rect button{ field.x + field.w - 40 + i * 20, field.y, 20, field.h };
			Frame(button, false);
			DrawText(button.x + 6, button.y + 3, i ? "+" : "-", 14);
		}
	}
	void NumberInput::OnClick()
	{
		Rect field = TextBox::FieldArea();
		const auto mouse = Input::Get().CursorPos();
		if (Number_ && mouse.y >= field.y && mouse.x >= field.x + field.w - 40) {
			Commit();
			double current = std::isfinite(*Number_) ? *Number_ : Minimum_;
			*Number_ = static_cast<float>(Clamp(current + (mouse.x >= field.x + field.w - 20 ? Step_ : -Step_), static_cast<double>(Minimum_), static_cast<double>(Maximum_)));
			Sync();
			Caret_ = Anchor_ = Buffer_.size();
		} else {
			TextBox::OnClick();
		}
	}

	Slider::Slider(const char* title, float* value, float minimum, float maximum, float step, int width, const char* tooltip)
		: Control(title, 0, 0, (std::max)(60, width), 34, ControlTypes::SLIDER, tooltip), Float_(value),
		Minimum_(std::isfinite(minimum) ? minimum : 0), Maximum_(std::isfinite(maximum) ? maximum : Minimum_), Step_(std::isfinite(step) && step > 0 ? step : 0)
	{
		if (Minimum_ > Maximum_) std::swap(Minimum_, Maximum_);
	}
	Slider::Slider(const char* title, int* value, int minimum, int maximum, int width, const char* tooltip)
		: Control(title, 0, 0, (std::max)(60, width), 34, ControlTypes::SLIDER, tooltip), Integer_(value),
		Minimum_((std::min)(minimum, maximum)), Maximum_((std::max)(minimum, maximum)), Step_(1) {}
	double Slider::Value() const
	{
		double value = Float_ ? *Float_ : Integer_ ? *Integer_ : Minimum_;
		return std::isfinite(value) ? Clamp(value, Minimum_, Maximum_) : Minimum_;
	}
	void Slider::SetValue(double value)
	{
		value = Clamp(value, Minimum_, Maximum_);
		if (Step_ > 0 && value != Maximum_) value = Minimum_ + std::round((value - Minimum_) / Step_) * Step_;
		value = Clamp(value, Minimum_, Maximum_);
		if (Float_) *Float_ = static_cast<float>(value);
		if (Integer_) *Integer_ = static_cast<int>(std::round(value));
	}
	void Slider::SetFromMouse()
	{
		Rect area = AbsoluteArea();
		double ratio = Clamp(static_cast<double>(Input::Get().CursorPos().x - area.x - 4) / (area.w - 8), 0.0, 1.0);
		SetValue(Minimum_ + ratio * (Maximum_ - Minimum_));
	}
	void Slider::OnClick()
	{
		if (Input::Get().CursorPos().y < AbsoluteArea().y + LabelHeight) return;
		Dragging_ = true;
		SetFromMouse();
	}
	void Slider::Update()
	{
		if (!Focused()) return;
		auto& input = Input::Get();
		if (Dragging_) {
			if (input.KeyDown(VK_LBUTTON)) SetFromMouse();
			else Dragging_ = false;
		}
		double step = Step_ > 0 ? Step_ : (Maximum_ - Minimum_) / 100.0;
		if (input.KeyPressed(VK_LEFT) || input.KeyPressed(VK_DOWN)) SetValue(Value() - step);
		if (input.KeyPressed(VK_RIGHT) || input.KeyPressed(VK_UP)) SetValue(Value() + step);
		if (input.KeyPressed(VK_HOME)) SetValue(Minimum_);
		if (input.KeyPressed(VK_END)) SetValue(Maximum_);
	}
	void Slider::Render()
	{
		Rect area = AbsoluteArea();
		std::string value = Number(Value());
		int valueWidth = (std::min)(TextWidth(value), area.w / 2);
		DrawText(area.x, area.y, Title_, area.w - valueWidth - 5);
		DrawText(area.x + area.w - valueWidth, area.y, value, valueWidth, Theme::Accent);
		Rect track{ area.x, area.y + LabelHeight, area.w, 18 };
		Frame(track, Focused());
		double ratio = Maximum_ > Minimum_ ? (Value() - Minimum_) / (Maximum_ - Minimum_) : 0;
		int filled = static_cast<int>(ratio * (area.w - 8));
		Render::FilledRect(track.x + 4, track.y + 7, area.w - 8, 4, { 57, 57, 57, 255 });
		if (filled) Render::FilledRect(track.x + 4, track.y + 7, filled, 4, Theme::Accent);
		Render::FilledRect(track.x + 2 + filled, track.y + 3, 4, 12, Theme::Accent);
	}

	Button::Button(const char* title, std::function<void()> action, int width, const char* tooltip)
		: Control(title, 0, 0, (std::max)(40, width), 22, ControlTypes::BUTTON, tooltip), Action_(std::move(action)) {}
	void Button::OnClick() { if (Action_) Action_(); }
	void Button::Update() { if (Focused() && Activate()) OnClick(); }
	void Button::Render()
	{
		Rect area = AbsoluteArea();
		Frame(area, Focused() || Input::Get().MouseInArea(area));
		DrawText(area.x + 5, area.y + 4, Title_, area.w - 10);
	}

	RadioGroup::RadioGroup(const char* title, const std::vector<const char*>& items, int* selected, int width, const char* tooltip)
		: Control(title, 0, 0, (std::max)(60, width), LabelHeight + static_cast<int>(items.size()) * RowHeight, ControlTypes::RADIOGROUP, tooltip), Items_(CopyItems(items)), Selected_(selected) {}
	void RadioGroup::OnClick()
	{
		int y = Input::Get().CursorPos().y - AbsoluteArea().y - LabelHeight;
		int row = y / RowHeight;
		if (Selected_ && y >= 0 && row < static_cast<int>(Items_.size())) *Selected_ = row;
	}
	void RadioGroup::Update()
	{
		if (!Focused() || !Selected_ || Items_.empty()) return;
		int count = static_cast<int>(Items_.size());
		auto& input = Input::Get();
		if (input.KeyPressed(VK_DOWN) || input.KeyPressed(VK_RIGHT)) *Selected_ = *Selected_ < 0 || *Selected_ >= count - 1 ? 0 : *Selected_ + 1;
		if (input.KeyPressed(VK_UP) || input.KeyPressed(VK_LEFT)) *Selected_ = *Selected_ <= 0 || *Selected_ >= count ? count - 1 : *Selected_ - 1;
		if (input.KeyPressed(VK_HOME)) *Selected_ = 0;
		if (input.KeyPressed(VK_END)) *Selected_ = count - 1;
		if (Activate() && (*Selected_ < 0 || *Selected_ >= count)) *Selected_ = 0;
	}
	void RadioGroup::Render()
	{
		Rect area = AbsoluteArea();
		DrawText(area.x, area.y, Title_, area.w, Focused() ? Theme::Accent : Text);
		for (int i = 0; i < static_cast<int>(Items_.size()); ++i) {
			int y = area.y + LabelHeight + i * RowHeight;
			// Scanline discs match the pixel-based renderer without a new primitive.
			for (int dy = -6; dy <= 6; ++dy) {
				int radius = static_cast<int>(std::sqrt(36 - dy * dy));
				Render::Line(area.x + 7 - radius, y + 8 + dy, area.x + 7 + radius, y + 8 + dy, { 15, 15, 15, 255 });
			}
			if (Selected_ && *Selected_ == i) {
				for (int dy = -3; dy <= 3; ++dy) {
					int radius = static_cast<int>(std::sqrt(9 - dy * dy));
					Render::Line(area.x + 7 - radius, y + 8 + dy, area.x + 7 + radius, y + 8 + dy, Theme::Accent);
				}
			}
			DrawText(area.x + 18, y + 2, Items_[i], area.w - 18);
		}
	}

	ListBox::ListBox(const char* title, const std::vector<const char*>& items, int* selected, int visibleRows, int width, const char* tooltip)
		: Control(title, 0, 0, (std::max)(80, width), LabelHeight + Clamp(visibleRows, 1, 20) * RowHeight + RowHeight, ControlTypes::LISTBOX, tooltip),
		Items_(CopyItems(items)), Selected_(selected), Rows_(Clamp(visibleRows, 1, 20))
	{
		if (selected && !items.empty()) Cursor_ = Clamp(*selected, 0, static_cast<int>(items.size()) - 1);
		First_ = (std::max)(0, Cursor_ - Rows_ + 1);
	}
	bool ListBox::Selected(int index) const { return Selected_ && *Selected_ == index; }
	void ListBox::Select(int index) { if (Selected_) *Selected_ = index; }
	void ListBox::OnClick()
	{
		Rect area = AbsoluteArea();
		int y = Input::Get().CursorPos().y - area.y - LabelHeight;
		if (y < 0) return;
		int row = y / RowHeight;
		if (row == Rows_) {
			int direction = Input::Get().CursorPos().x < area.x + area.w / 2 ? -1 : 1;
			First_ = Clamp(First_ + direction * Rows_, 0, (std::max)(0, static_cast<int>(Items_.size()) - Rows_));
		} else if (row < Rows_ && First_ + row < static_cast<int>(Items_.size())) {
			Cursor_ = First_ + row;
			Select(Cursor_);
		}
	}
	void ListBox::Update()
	{
		if (!Focused() || Items_.empty()) return;
		auto& input = Input::Get();
		int previous = Cursor_;
		if (input.KeyPressed(VK_UP)) --Cursor_;
		if (input.KeyPressed(VK_DOWN)) ++Cursor_;
		if (input.KeyPressed(VK_PRIOR)) Cursor_ -= Rows_;
		if (input.KeyPressed(VK_NEXT)) Cursor_ += Rows_;
		if (input.KeyPressed(VK_HOME)) Cursor_ = 0;
		if (input.KeyPressed(VK_END)) Cursor_ = static_cast<int>(Items_.size()) - 1;
		Cursor_ = Clamp(Cursor_, 0, static_cast<int>(Items_.size()) - 1);
		if (Cursor_ != previous) {
			if (Cursor_ < First_) First_ = Cursor_;
			if (Cursor_ >= First_ + Rows_) First_ = Cursor_ - Rows_ + 1;
			if (Type_ == ControlTypes::LISTBOX) Select(Cursor_);
		}
		if (Activate()) Select(Cursor_);
	}
	void ListBox::Render()
	{
		Rect area = AbsoluteArea();
		DrawText(area.x, area.y, Title_, area.w);
		Rect list{ area.x, area.y + LabelHeight, area.w, area.h - LabelHeight };
		Frame(list, Focused());
		for (int row = 0; row < Rows_ && First_ + row < static_cast<int>(Items_.size()); ++row) {
			int index = First_ + row, y = list.y + row * RowHeight;
			if (Selected(index)) Render::FilledRect(list.x + 1, y + 1, list.w - 2, RowHeight - 1, Theme::Selection());
			if (Focused() && Cursor_ == index) Render::Rect(list.x + 1, y + 1, list.w - 2, RowHeight - 1, Theme::Accent);
			std::string prefix = Type_ == ControlTypes::MULTISELECT ? (Selected(index) ? "[x] " : "[ ] ") : "";
			DrawText(list.x + 4, y + 3, prefix + Items_[index], list.w - 8);
		}
		int y = list.y + Rows_ * RowHeight;
		Render::Line(list.x, y, list.x + list.w, y, { 0, 0, 0, 255 });
		DrawText(list.x + 4, y + 2, "< Prev", list.w / 2 - 8, First_ > 0 ? Text : Render::Color{ 100, 100, 100, 255 });
		DrawText(list.x + list.w / 2 + 4, y + 2, "Next >", list.w / 2 - 8, First_ + Rows_ < static_cast<int>(Items_.size()) ? Text : Render::Color{ 100, 100, 100, 255 });
	}
	MultiSelect::MultiSelect(const char* title, const std::vector<const char*>& items, std::vector<bool>* selected, int visibleRows, int width, const char* tooltip)
		: ListBox(title, items, nullptr, visibleRows, width, tooltip), Values_(selected)
	{
		Type_ = ControlTypes::MULTISELECT;
		if (Values_) Values_->resize(Items_.size(), false);
	}
	bool MultiSelect::Selected(int index) const { return Values_ && index >= 0 && static_cast<size_t>(index) < Values_->size() && (*Values_)[index]; }
	void MultiSelect::Select(int index)
	{
		if (!Values_ || index < 0 || static_cast<size_t>(index) >= Items_.size()) return;
		Values_->resize(Items_.size(), false);
		(*Values_)[index] = !(*Values_)[index];
	}

	namespace
	{
		std::uint8_t& Channel(Render::Color& color, int channel)
		{
			switch (channel) { case 0: return color.red; case 1: return color.green; case 2: return color.blue; default: return color.alpha; }
		}
	}
	ColorPicker::ColorPicker(const char* title, Render::Color* value, int width, const char* tooltip)
		: Control(title, 0, 0, (std::max)(140, width), 34, ControlTypes::COLORPICKER, tooltip), Value_(value) {}
	Rect ColorPicker::InputArea()
	{
		Rect area = AbsoluteArea();
		if (Open_) area.h += 4 * RowHeight + 8;
		return area;
	}
	void ColorPicker::SetFromMouse()
	{
		if (!Value_ || Channel_ < 0) return;
		Rect area = AbsoluteArea();
		double ratio = Clamp(static_cast<double>(Input::Get().CursorPos().x - area.x - 22) / (area.w - 62), 0.0, 1.0);
		Channel(*Value_, Channel_) = static_cast<std::uint8_t>(std::round(ratio * 255));
	}
	void ColorPicker::OnClick()
	{
		if (!Value_) return;
		Rect area = AbsoluteArea();
		if (Input::Get().CursorPos().y < area.y + area.h) { Open_ = !Open_; Channel_ = -1; return; }
		int y = Input::Get().CursorPos().y - area.y - area.h - 4;
		if (Open_ && y >= 0 && y < 4 * RowHeight) {
			SelectedChannel_ = Channel_ = y / RowHeight;
			SetFromMouse();
		}
	}
	void ColorPicker::Update()
	{
		if (!Focused() || !Value_) return;
		auto& input = Input::Get();
		if (Activate()) Open_ = !Open_;
		if (input.KeyPressed(VK_ESCAPE)) OnBlur();
		if (!Open_) return;
		if (Channel_ >= 0) {
			if (input.KeyDown(VK_LBUTTON)) SetFromMouse();
			else Channel_ = -1;
		}
		if (input.KeyPressed(VK_UP)) SelectedChannel_ = (SelectedChannel_ + 3) % 4;
		if (input.KeyPressed(VK_DOWN)) SelectedChannel_ = (SelectedChannel_ + 1) % 4;
		int delta = input.KeyDown(VK_SHIFT) ? 10 : 1;
		if (input.KeyPressed(VK_LEFT)) Channel(*Value_, SelectedChannel_) = static_cast<std::uint8_t>(Clamp(Channel(*Value_, SelectedChannel_) - delta, 0, 255));
		if (input.KeyPressed(VK_RIGHT)) Channel(*Value_, SelectedChannel_) = static_cast<std::uint8_t>(Clamp(Channel(*Value_, SelectedChannel_) + delta, 0, 255));
	}
	void ColorPicker::Render()
	{
		Rect area = AbsoluteArea();
		DrawText(area.x, area.y, Title_, area.w);
		Rect field{ area.x, area.y + LabelHeight, area.w, area.h - LabelHeight };
		Frame(field, Focused());
		if (!Value_) return;
		for (int y = 0; y < 12; y += 4) for (int x = 0; x < 24; x += 4)
			Render::FilledRect(field.x + 4 + x, field.y + 4 + y, 4, 4, (x / 4 + y / 4) % 2 ? Render::Color{ 160, 160, 160, 255 } : Render::Color{ 70, 70, 70, 255 });
		Render::FilledRect(field.x + 4, field.y + 4, 24, 12, *Value_);
		std::ostringstream hex;
		hex << '#' << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << static_cast<int>(Value_->red) << std::setw(2) << static_cast<int>(Value_->green) << std::setw(2) << static_cast<int>(Value_->blue) << std::setw(2) << static_cast<int>(Value_->alpha);
		DrawText(field.x + 34, field.y + 3, hex.str(), field.w - 38);
		if (!Open_) return;
		Frame({ area.x, area.y + area.h, area.w, 4 * RowHeight + 8 }, true);
		const char* names[] = { "R", "G", "B", "A" };
		for (int i = 0; i < 4; ++i) {
			int y = area.y + area.h + 4 + i * RowHeight;
			DrawText(area.x + 5, y + 2, names[i], 15, i == SelectedChannel_ ? Theme::Accent : Text);
			Render::Color low = *Value_, high = *Value_;
			low.alpha = high.alpha = 255;
			Channel(low, i) = 0; Channel(high, i) = 255;
			Render::FilledRect(area.x + 22, y + 3, area.w - 62, 12, { 75, 75, 75, 255 });
			Render::FilledRectGradient(area.x + 22, y + 3, area.w - 62, 12, low, high, true);
			int x = area.x + 22 + static_cast<int>(Channel(*Value_, i) / 255.0 * (area.w - 62));
			Render::Line(x, y + 1, x, y + 16, Text);
			DrawText(area.x + area.w - 34, y + 2, std::to_string(Channel(*Value_, i)), 30);
		}
	}

	namespace
	{
		std::string KeyName(int key)
		{
			switch (key) {
			case 0: return "Unbound";
			case VK_LBUTTON: return "Mouse 1";
			case VK_RBUTTON: return "Mouse 2";
			case VK_MBUTTON: return "Mouse 3";
			case VK_XBUTTON1: return "Mouse 4";
			case VK_XBUTTON2: return "Mouse 5";
			}
			UINT scan = MapVirtualKeyA(key, MAPVK_VK_TO_VSC);
			if (key == VK_LEFT || key == VK_RIGHT || key == VK_UP || key == VK_DOWN || key == VK_HOME || key == VK_END || key == VK_PRIOR || key == VK_NEXT || key == VK_INSERT || key == VK_DELETE || key == VK_DIVIDE || key == VK_NUMLOCK) scan |= 0x100;
			char name[64]{};
			if (GetKeyNameTextA(static_cast<LONG>(scan << 16), name, 64)) return name;
			return "Key " + std::to_string(key);
		}
	}
	KeyBind::KeyBind(const char* title, KeyBinding* binding, int width, const char* tooltip)
		: Control(title, 0, 0, (std::max)(160, width), 34, ControlTypes::KEYBIND, tooltip), Binding_(binding) {}
	void KeyBind::OnClick()
	{
		if (!Binding_ || Listening_) return;
		Rect area = AbsoluteArea();
		if (Input::Get().CursorPos().y < area.y + LabelHeight) return;
		if (Input::Get().CursorPos().x >= area.x + area.w - 54) {
			Binding_->Mode = Binding_->Mode == BindMode::Hold ? BindMode::Toggle : Binding_->Mode == BindMode::Toggle ? BindMode::Always : BindMode::Hold;
		} else { Listening_ = true; Armed_ = false; }
	}
	void KeyBind::Update()
	{
		if (!Focused() || !Binding_) return;
		auto& input = Input::Get();
		if (!Listening_) {
			if (Activate()) { Listening_ = true; Armed_ = false; }
			if (input.KeyPressed(VK_LEFT) || input.KeyPressed(VK_RIGHT)) Binding_->Mode = Binding_->Mode == BindMode::Hold ? BindMode::Toggle : Binding_->Mode == BindMode::Toggle ? BindMode::Always : BindMode::Hold;
			return;
		}
		if (!Armed_) {
			// Wait for the activating click/key to be released before capturing.
			Armed_ = !input.KeyDown(VK_LBUTTON) && !input.KeyDown(VK_SPACE) && !input.KeyDown(VK_RETURN);
			return;
		}
		if (input.KeyPressed(VK_ESCAPE)) { Listening_ = false; return; }
		if (input.KeyPressed(VK_BACK) || input.KeyPressed(VK_DELETE)) { Binding_->Key = 0; Listening_ = false; return; }
		for (int key = 1; key < MAX_KEYS; ++key) {
			if (key == VK_SHIFT || key == VK_CONTROL || key == VK_MENU) continue;
			if (input.KeyPressed(key)) { Binding_->Key = key; Listening_ = false; return; }
		}
	}
	void KeyBind::UpdateBinding(bool suppressed)
	{
		if (!Binding_) return;
		auto& input = Input::Get();
		if (Binding_->Key != PreviousKey_ || Binding_->Mode != PreviousMode_) {
			ToggleState_ = false;
			PreviousKey_ = Binding_->Key;
			PreviousMode_ = Binding_->Mode;
		}
		if (!input.HasFocus()) { ToggleState_ = false; Binding_->Active = false; WasSuppressed_ = true; return; }
		if (suppressed || Listening_) { Binding_->Active = false; WasSuppressed_ = true; return; }
		if (Binding_->Mode == BindMode::Always) Binding_->Active = true;
		else if (Binding_->Key <= 0 || Binding_->Key >= MAX_KEYS) Binding_->Active = false;
		else if (Binding_->Mode == BindMode::Hold) Binding_->Active = input.KeyDown(Binding_->Key);
		else {
			if (!WasSuppressed_ && input.KeyPressed(Binding_->Key)) ToggleState_ = !ToggleState_;
			Binding_->Active = ToggleState_;
		}
		WasSuppressed_ = false;
	}
	void KeyBind::Render()
	{
		Rect area = AbsoluteArea();
		DrawText(area.x, area.y, Title_, area.w, Binding_ && Binding_->Active ? Theme::Accent : Text);
		Rect field{ area.x, area.y + LabelHeight, area.w, area.h - LabelHeight };
		Frame(field, Focused());
		if (!Binding_) return;
		DrawText(field.x + 4, field.y + 3, Listening_ ? "Press key..." : KeyName(Binding_->Key), field.w - 62);
		Render::Line(field.x + field.w - 54, field.y, field.x + field.w - 54, field.y + field.h, { 0, 0, 0, 255 });
		const char* mode = Binding_->Mode == BindMode::Hold ? "Hold" : Binding_->Mode == BindMode::Toggle ? "Toggle" : "Always";
		DrawText(field.x + field.w - 50, field.y + 3, mode, 46);
	}
}
