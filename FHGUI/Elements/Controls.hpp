#pragma once
#include <vector>
#include <string>
#include <functional>
#include "../../Render/Render.hpp"

#include "../Datatypes.hpp"

namespace FHGUI
{
	class GroupBox;
	class Tab;
	class Window;

	enum class ControlTypes : int
	{
		INVALID = 0,
		GROUPBOX,
		CHECKBOX,
		DROPDOWN,
		TEXTBOX, NUMBERINPUT, SLIDER, BUTTON, RADIOGROUP, LISTBOX, MULTISELECT, COLORPICKER, KEYBIND
	};

	class Control
	{
		friend GroupBox;
		friend Tab;
	public:
		Control(const char* strTitle, int PosX, int PosY, int Width, int Height, ControlTypes Type, const char* strTooltip = "")
			: Title_{ strTitle }, PosX_{ PosX }, PosY_{ PosY }, Width_{ Width }, Height_{ Height }, Type_{ Type }
		{
			SetTooltip(strTooltip);
		}

		void SetTooltip(const char* strTooltip);
		virtual ~Control();
		virtual bool Focusable() const { return Type_ != ControlTypes::GROUPBOX; }
		virtual bool CapturesKeyboard() const { return false; }
		virtual bool IsPopupOpen() const { return false; }
		virtual void OnFocus() {}
		virtual void OnBlur() {}
		virtual void UpdateBinding(bool suppressed) {}
		bool Focused() const;
		bool BelongsTo(const Tab* tab) const { return Tab_ == tab; }
		bool BelongsTo(const Window* window) const { return Window_ == window; }

		virtual void Render() = 0;
		virtual void Update() {};
		virtual void OnClick() {};
		virtual Rect AbsoluteArea();
		virtual Rect InputArea() { return AbsoluteArea(); };
		virtual Rect TooltipArea() { return InputArea(); };
		virtual void RenderTooltip();
	protected:
		std::string Title_{};
		std::string Tooltip_{};
		int PosX_{ 0 };
		int PosY_{ 0 };
		int Width_{ 0 };
		int Height_{ 0 };
		int TooltipHeight_{ 0 };
		ControlTypes Type_{ ControlTypes::INVALID };
		Tab* Tab_{ nullptr };
		Window* Window_{ nullptr };
	};

	class GroupBox : public Control
	{
	public:
		GroupBox(const char* strTitle, int PosX, int PosY, int Width, int Height);

		virtual void Render() override;

		void RegisterControl(Control* pControl);
	private:
		int OffsetX_{ 5 }, OffsetY_{ 10 };
	};

	class CheckBox : public Control
	{
	public:
		CheckBox(const char* strTitle, bool* State, const char* strTooltip = "");

		virtual void Render() override;
		virtual void OnClick() override;
		virtual Rect InputArea() override;
		virtual void Update() override;
	private:
		bool* Checked_{ nullptr };
		int TitleWidth_{ 0 };
	};

	class Dropdown : public Control
	{
	public:
		Dropdown(const char* strTitle, const std::vector<const char*>& Items, int* SelectedItem, const char* strTooltip = "");

		virtual void Render() override;
		virtual void OnClick() override;
		virtual Rect InputArea() override;
		virtual Rect TooltipArea() override { return AbsoluteArea(); };
		void Update() override;
		void OnBlur() override { IsOpen_ = false; }
		bool IsPopupOpen() const override { return IsOpen_; }
	private:
		std::vector<std::string> Items_{};
		int* SelectedItem_{ nullptr };
		bool IsOpen_{ false };
	};

	// Values are caller-owned and must outlive their controls. Tabs own controls.
	class TextBox : public Control
	{
	public:
		TextBox(const char* title, std::string* value, size_t maxLength = 128, int width = 180, const char* tooltip = "");
		void Render() override;
		void Update() override;
		void OnClick() override;
		void OnFocus() override;
		bool CapturesKeyboard() const override { return true; }
	protected:
		virtual void Commit() {}
		virtual void Cancel();
		void Insert(const std::string& text);
		void DeleteSelection();
		virtual Rect FieldArea();
		std::string* Value_;
		std::string Original_;
		size_t MaxLength_, Caret_{ 0 }, Anchor_{ 0 }, ViewStart_{ 0 };
	};

	class NumberInput : public TextBox
	{
	public:
		NumberInput(const char* title, float* value, float minimum, float maximum, float step = 1.0f, int width = 180, const char* tooltip = "");
		void Render() override;
		void OnClick() override;
		void OnFocus() override;
		void OnBlur() override;
	protected:
		void Commit() override;
		void Cancel() override;
		Rect FieldArea() override;
	private:
		void Sync();
		std::string Buffer_;
		float* Number_;
		float Minimum_, Maximum_, Step_;
	};

	class Slider : public Control
	{
	public:
		Slider(const char* title, float* value, float minimum, float maximum, float step = 0.0f, int width = 180, const char* tooltip = "");
		Slider(const char* title, int* value, int minimum, int maximum, int width = 180, const char* tooltip = "");
		void Render() override;
		void Update() override;
		void OnClick() override;
		void OnBlur() override { Dragging_ = false; }
	private:
		double Value() const;
		void SetValue(double value);
		void SetFromMouse();
		float* Float_{ nullptr };
		int* Integer_{ nullptr };
		double Minimum_, Maximum_, Step_;
		bool Dragging_{ false };
	};

	class Button : public Control
	{
	public:
		Button(const char* title, std::function<void()> action, int width = 180, const char* tooltip = "");
		void Render() override;
		void Update() override;
		void OnClick() override;
	private:
		std::function<void()> Action_;
	};

	class RadioGroup : public Control
	{
	public:
		RadioGroup(const char* title, const std::vector<const char*>& items, int* selected, int width = 180, const char* tooltip = "");
		void Render() override;
		void Update() override;
		void OnClick() override;
	private:
		std::vector<std::string> Items_;
		int* Selected_;
	};

	class ListBox : public Control
	{
	public:
		ListBox(const char* title, const std::vector<const char*>& items, int* selected, int visibleRows = 4, int width = 180, const char* tooltip = "");
		void Render() override;
		void Update() override;
		void OnClick() override;
	protected:
		virtual bool Selected(int index) const;
		virtual void Select(int index);
		std::vector<std::string> Items_;
		int* Selected_;
		int Rows_, First_{ 0 }, Cursor_{ 0 };
	};

	class MultiSelect : public ListBox
	{
	public:
		MultiSelect(const char* title, const std::vector<const char*>& items, std::vector<bool>* selected, int visibleRows = 4, int width = 180, const char* tooltip = "");
	protected:
		bool Selected(int index) const override;
		void Select(int index) override;
	private:
		std::vector<bool>* Values_;
	};

	class ColorPicker : public Control
	{
	public:
		ColorPicker(const char* title, Render::Color* value, int width = 180, const char* tooltip = "");
		void Render() override;
		void Update() override;
		void OnClick() override;
		void OnBlur() override { Open_ = false; Channel_ = -1; }
		bool IsPopupOpen() const override { return Open_; }
		Rect InputArea() override;
		Rect TooltipArea() override { return AbsoluteArea(); }
	private:
		void SetFromMouse();
		Render::Color* Value_;
		bool Open_{ false };
		int Channel_{ -1 }, SelectedChannel_{ 0 };
	};

	enum class BindMode { Hold, Toggle, Always };
	struct KeyBinding
	{
		int Key{ 0 }; // Win32 virtual-key code; zero means unbound.
		BindMode Mode{ BindMode::Hold };
		bool Active{ false };
	};

	class KeyBind : public Control
	{
	public:
		KeyBind(const char* title, KeyBinding* binding, int width = 180, const char* tooltip = "");
		void Render() override;
		void Update() override;
		void OnClick() override;
		void OnBlur() override { Listening_ = false; }
		bool CapturesKeyboard() const override { return Listening_; }
		void UpdateBinding(bool suppressed) override;
	private:
		KeyBinding* Binding_;
		bool Listening_{ false }, Armed_{ false }, ToggleState_{ false }, WasSuppressed_{ false };
		int PreviousKey_{ 0 };
		BindMode PreviousMode_{ BindMode::Hold };
	};
}
