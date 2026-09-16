// Headless behavior tests. Only rendering and frame input are substituted;
// the controls, focus routing, Win32 message handling and bindings are real.
#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <string>
#include <vector>
#include "../FHGUI/Input.hpp"
#include "../FHGUI/Elements/Controls.hpp"
#include "../FHGUI/Elements/Window.hpp"
#include "../Render/D3DFont.hpp"

namespace FHGUI
{
	struct InputTestDriver
	{
		static void Frame(std::initializer_list<int> keys = {}, Point mouse = { 0, 0 }, const wchar_t* characters = L"", std::initializer_list<int> editingKeys = {})
		{
			auto& input = Input::Get();
			std::copy(std::begin(input.PressedKeys_), std::end(input.PressedKeys_), std::begin(input.PrevPressedKeys_));
			std::fill(std::begin(input.PressedKeys_), std::end(input.PressedKeys_), false);
			for (int key : keys) input.PressedKeys_[key] = true;
			input.MousePos_ = mouse;
			input.Characters_ = characters;
			input.EditingKeys_ = editingKeys;
		}
		static void Messages()
		{
			auto& input = Input::Get();
			Frame();
			input.Characters_.swap(input.PendingCharacters_);
			input.PendingCharacters_.clear();
			input.EditingKeys_.swap(input.PendingEditingKeys_);
			input.PendingEditingKeys_.clear();
		}
	};
}

static std::vector<std::string> DrawnText;
CD3DFont::CD3DFont(const TCHAR*, DWORD, DWORD, DWORD) : iHeight(12) {}
CD3DFont::~CD3DFont() {}
namespace Render
{
	static CD3DFont Font(L"Test", 8, FW_NORMAL);
	CD3DFont* MenuFont = &Font;
	void Line(int, int, int, int, Color) {}
	void Rect(int, int, int w, int h, Color) { assert(w > 0 && h > 0); }
	void FilledRect(int, int, int w, int h, Color) { assert(w >= 0 && h >= 0); }
	void FilledRectGradient(int, int, int w, int h, Color, Color, bool) { assert(w > 0 && h > 0); }
	void String(int, int, Color, const char* text, CD3DFont*, DWORD) { DrawnText.emplace_back(text); }
	Size GetTextSize(const char* text, CD3DFont*) { return { static_cast<int>(std::string(text).size()) * 6, 12 }; }
}

using namespace FHGUI;
using Driver = InputTestDriver;

static void Press(Control& control, int key)
{
	Driver::Frame();
	Driver::Frame({ key });
	control.Update();
	Driver::Frame();
}

static void TestTextAndNumbers()
{
	auto& input = Input::Get();
	std::string text = "abc";
	TextBox box("Text", &text, 5);
	input.SetFocus(&box);
	Driver::Frame({}, {}, L"deXYZ"); box.Update();
	assert(text == "abcde");
	Driver::Frame({}, {}, L"", { VK_HOME, VK_RIGHT }); box.Update();
	Driver::Frame({ VK_SHIFT }, {}, L"", { VK_RIGHT, VK_RIGHT }); box.Update();
	Driver::Frame({}, {}, L"Q"); box.Update();
	assert(text == "aQde");
	Driver::Frame({}, {}, L"\b"); box.Update();
	assert(text == "ade");
	Driver::Frame({}, {}, L"", { VK_DELETE }); box.Update();
	assert(text == "ae");
	Press(box, VK_ESCAPE);
	assert(text == "abc" && !input.FocusedControl());

	input.SetFocus(&box);
	input.OnWindowProc(nullptr, WM_CHAR, L'!', 0);
	input.OnWindowProc(nullptr, WM_CHAR, 0x03A9, 0); // Unsupported glyph is ignored.
	Driver::Messages(); box.Update();
	assert(text == "abc!");
	input.OnWindowProc(nullptr, WM_KEYDOWN, VK_HOME, 0);
	Driver::Messages(); box.Update();
	Driver::Frame({}, {}, L"Z"); box.Update();
	assert(text == "Zabc!");
	input.SetFocus(nullptr);
	assert(!input.OnWindowProc(nullptr, WM_CHAR, L'X', 0));
	assert(!input.KeyDown(-1) && !input.KeyDown(256) && !input.KeyPressed(999) && !input.KeyHeld(-5));
	text.clear(); input.SetFocus(&box);
	Driver::Frame({ VK_RETURN }, {}, L"done\r"); box.Update();
	assert(text == "done" && !input.FocusedControl());
	text.clear(); input.SetFocus(&box);
	Driver::Frame({}, {}, L"fast\r"); box.Update(); // Enter released before the next frame.
	assert(text == "fast" && !input.FocusedControl());

	float number = 4;
	NumberInput field("Number", &number, 0, 10, 0.5f);
	auto type = [&](const wchar_t* value) {
		input.SetFocus(nullptr); input.SetFocus(&field);
		Driver::Frame({ VK_CONTROL, 'A' }); field.Update();
		Driver::Frame({}, {}, value); field.Update();
	};
	type(L"99"); Press(field, VK_RETURN); assert(number == 10);
	type(L"-12"); input.SetFocus(nullptr); assert(number == 0);
	type(L"nan"); Press(field, VK_RETURN); assert(number == 0);
	type(L"5junk"); Press(field, VK_RETURN); assert(number == 0);
	type(L"3.5"); Press(field, VK_RETURN); assert(number == 3.5f);
	type(L"9"); Press(field, VK_ESCAPE); assert(number == 3.5f);
	input.SetFocus(&field);
	Driver::Frame({ VK_LBUTTON }, { 175, 20 }); field.OnClick(); assert(number == 4);
	field.Render();
	input.SetFocus(nullptr);
}

static void TestChoicesAndSliders()
{
	auto& input = Input::Get();
	int value = 0;
	Slider integer("Integer", &value, 0, 10);
	input.SetFocus(&integer);
	Driver::Frame({ VK_LBUTTON }, { 500, 22 }); integer.OnClick(); assert(value == 10);
	Driver::Frame({ VK_LBUTTON }, { -500, 22 }); integer.Update(); assert(value == 0);
	Driver::Frame(); integer.Update();
	Press(integer, VK_RIGHT); assert(value == 1);
	Press(integer, VK_END); assert(value == 10);
	float fraction = 0;
	Slider stepped("Step", &fraction, 1, 0, 0.25f);
	input.SetFocus(&stepped);
	Press(stepped, VK_RIGHT); assert(fraction == 0.25f);
	Slider fixed("Fixed", &fraction, 2, 2);
	input.SetFocus(&fixed); Press(fixed, VK_END); assert(fraction == 2); fixed.Render();

	int selected = -1;
	RadioGroup radio("Radio", { "A", "B", "C" }, &selected);
	input.SetFocus(&radio);
	Press(radio, VK_DOWN); assert(selected == 0);
	Press(radio, VK_UP); assert(selected == 2);
	Driver::Frame({ VK_LBUTTON }, { 25, 34 }); radio.OnClick(); assert(selected == 1);

	ListBox list("List", { "A", "B", "C", "D", "E" }, &selected, 2);
	input.SetFocus(&list);
	Press(list, VK_END); assert(selected == 4);
	Press(list, VK_PRIOR); assert(selected == 2);
	Driver::Frame({ VK_LBUTTON }, { 160, 55 }); list.OnClick(); // Next page.
	Driver::Frame({ VK_LBUTTON }, { 20, 16 }); list.OnClick(); assert(selected == 3);
	std::vector<bool> values;
	MultiSelect multiple("Multiple", { "A", "B", "C" }, &values, 2);
	assert(values.size() == 3);
	input.SetFocus(&multiple);
	Press(multiple, VK_SPACE); assert(values[0]);
	Press(multiple, VK_DOWN); assert(values[0] && !values[1]);
	Press(multiple, VK_SPACE); assert(values[0] && values[1]);
	values.clear(); Press(multiple, VK_SPACE); assert(values.size() == 3 && values[1]);
	input.SetFocus(nullptr);
}

static void TestPopupRoutingAndFocus()
{
	auto& input = Input::Get();
	int selected = 99;
	int clicks = 0;
	Window window("Window", 0, 0, 300, 300);
	auto tab = new Tab("Tab"); window.RegisterTab(tab);
	auto group = new GroupBox("Group", 0, 0, 240, 240); tab->RegisterControl(group);
	auto dropdown = new Dropdown("Dropdown", { "A", "B", "C" }, &selected); group->RegisterControl(dropdown);
	auto button = new Button("Below", [&] { ++clicks; }); group->RegisterControl(button);
	assert(button->AbsoluteArea().y >= dropdown->AbsoluteArea().y + dropdown->AbsoluteArea().h);
	assert(dropdown->InputArea().h == dropdown->AbsoluteArea().h);
	dropdown->Render(); // Invalid external selection must never index out of bounds.
	Rect area = dropdown->AbsoluteArea();
	Driver::Frame(); Driver::Frame({ VK_LBUTTON }, { area.x + 4, area.y + 5 }); tab->Update();
	assert(dropdown->IsPopupOpen());
	DrawnText.clear(); tab->Render({ 0, 0, 40, 20 }, true, true);
	assert(!DrawnText.empty() && DrawnText.back() == "C");
	Driver::Frame(); Driver::Frame({ VK_LBUTTON }, { area.x + 4, area.y + 20 }); tab->Update();
	assert(selected == 0 && clicks == 0 && !dropdown->IsPopupOpen());
	Driver::Frame(); Driver::Frame({ VK_TAB }); tab->Update(); assert(input.FocusedControl() == button);
	Press(*button, VK_SPACE); assert(clicks == 1);
	Driver::Frame({ VK_SHIFT, VK_TAB }); tab->Update(); assert(input.FocusedControl() == dropdown);
	Press(*dropdown, VK_SPACE); assert(dropdown->IsPopupOpen());
	Driver::Frame(); Driver::Frame({ VK_LBUTTON }, { 290, 290 }); tab->Update();
	assert(!dropdown->IsPopupOpen() && !input.FocusedControl());
	Dropdown empty("Empty", {}, &selected); empty.OnClick(); empty.Render(); assert(!empty.IsPopupOpen());

	Render::Color color{ 50, 60, 70, 80 };
	ColorPicker picker("Color", &color);
	input.SetFocus(&picker); Driver::Frame({ VK_LBUTTON }, { 5, 20 }); picker.OnClick();
	assert(picker.IsPopupOpen() && picker.InputArea().h > picker.AbsoluteArea().h);
	Driver::Frame({ VK_LBUTTON }, { 500, 40 }); picker.OnClick(); assert(color.red == 255);
	Driver::Frame({ VK_LBUTTON }, { -500, 40 }); picker.Update(); assert(color.red == 0);
	Driver::Frame(); picker.Update();
	Press(picker, VK_DOWN); Press(picker, VK_RIGHT); assert(color.green == 61);
	picker.Render();
	input.OnWindowProc(nullptr, WM_KILLFOCUS, 0, 0);
	assert(!picker.IsPopupOpen() && !input.FocusedControl());
}

static void TestBindings()
{
	auto& input = Input::Get();
	input.Init(GetForegroundWindow());
	KeyBinding binding{ 'K', BindMode::Hold, false };
	KeyBind bind("Bind", &binding);
	Driver::Frame({ 'K' }); bind.UpdateBinding(false); assert(binding.Active);
	Driver::Frame(); bind.UpdateBinding(false); assert(!binding.Active);
	binding.Mode = BindMode::Toggle;
	Driver::Frame({ 'K' }); bind.UpdateBinding(false); assert(binding.Active);
	Driver::Frame({ 'K' }); bind.UpdateBinding(false); assert(binding.Active);
	Driver::Frame(); bind.UpdateBinding(false); assert(binding.Active);
	Driver::Frame({ 'K' }); bind.UpdateBinding(false); assert(!binding.Active);
	bind.UpdateBinding(true); assert(!binding.Active);
	bind.UpdateBinding(false); assert(!binding.Active);
	binding.Mode = BindMode::Always; bind.UpdateBinding(false); assert(binding.Active);
	bind.UpdateBinding(true); assert(!binding.Active);

	input.SetFocus(&bind);
	Driver::Frame({ VK_LBUTTON }, { 5, 20 }); bind.OnClick();
	bind.Update(); assert(bind.CapturesKeyboard() && binding.Key == 'K');
	Driver::Frame(); bind.Update(); // Release arms capture.
	Driver::Frame({ VK_RBUTTON }); bind.Update(); assert(binding.Key == VK_RBUTTON && !bind.CapturesKeyboard());
	Driver::Frame({ VK_LBUTTON }, { 5, 20 }); bind.OnClick();
	Driver::Frame(); bind.Update();
	Driver::Frame({ VK_ESCAPE }); bind.Update(); assert(binding.Key == VK_RBUTTON);
	Driver::Frame({ VK_LBUTTON }, { 5, 20 }); bind.OnClick();
	Driver::Frame(); bind.Update();
	Driver::Frame({ VK_DELETE }); bind.Update(); assert(binding.Key == 0);
	bind.Render();
	input.SetFocus(nullptr);

	// A left-click captured over a tab header must not switch tabs or refocus.
	Window window("Window", 0, 0, 300, 300);
	auto first = new Tab("First"), second = new Tab("Second");
	window.RegisterTab(first); window.RegisterTab(second);
	auto capture = new KeyBind("Capture", &binding); first->RegisterControl(capture);
	input.SetFocus(capture);
	Rect area = capture->AbsoluteArea();
	Driver::Frame({ VK_LBUTTON }, { area.x + 5, area.y + 20 }); capture->OnClick();
	Driver::Frame(); window.Update();
	Driver::Frame({ VK_LBUTTON }, { 90, 25 }); window.Update();
	assert(binding.Key == VK_LBUTTON && input.FocusedControl() == capture);
	// Bindings on an unselected tab still update.
	KeyBinding hidden{ 'J', BindMode::Hold, false };
	second->RegisterControl(new KeyBind("Hidden", &hidden));
	Driver::Frame({ 'J' }); window.UpdateBindings(false); assert(hidden.Active);
	input.SetFocus(nullptr);
}

int main()
{
	Input::Get().Init(GetForegroundWindow());
	TestTextAndNumbers();
	TestChoicesAndSliders();
	TestPopupRoutingAndFocus();
	TestBindings();
	std::cout << "PASS: text/number editing, bounds, selections, popup routing, focus, color, key capture and binding modes\n";
}
