#include "Menu.hpp"

#include "../FHGUI/FHGUI.hpp"
#include "../FHGUI/Theme.hpp"
#include "../FHGUI/Elements/Controls.hpp"
#include "../FHGUI/Elements/Window.hpp"

void Menu::Init()
{
	FHGUI::Window* Window = new FHGUI::Window("FlowHooks", 25, 25, 516, 608);

	// Register the input gallery first so the new controls are visible at launch.
	FHGUI::Tab* InputsTab = new FHGUI::Tab("Inputs");
	Window->RegisterTab(InputsTab);
	{
		static std::string profile = "Default";
		static bool enabled = true;
		static float amount = 25.0f, opacity = 0.75f;
		static int count = 5, mode = 0, preset = 0, theme = 0;
		static FHGUI::KeyBinding binding{ 'K', FHGUI::BindMode::Hold, false };
		static std::vector<bool> options{ true, false, true, false, false, false };

		auto fields = new FHGUI::GroupBox("Text and values", 0, 0, 240, 310);
		InputsTab->RegisterControl(fields);
		fields->RegisterControl(new FHGUI::CheckBox("Enabled", &enabled));
		fields->RegisterControl(new FHGUI::TextBox("Profile name", &profile, 64, 225, "Type to edit. Ctrl+A/C/X/V, Shift+arrows, Home/End. Enter accepts; Escape restores."));
		fields->RegisterControl(new FHGUI::NumberInput("Amount", &amount, 0.0f, 100.0f, 0.5f, 225, "Type a number or use minus/plus. Enter or leaving the field commits a value from 0 to 100."));
		fields->RegisterControl(new FHGUI::Slider("Opacity", &opacity, 0.0f, 1.0f, 0.01f, 225));
		fields->RegisterControl(new FHGUI::Slider("Count", &count, 0, 20, 225));
		fields->RegisterControl(new FHGUI::Dropdown("Theme", { "Dark", "Light", "System" }, &theme));
		fields->RegisterControl(new FHGUI::Button("Reset inputs", [] {
			profile = "Default"; enabled = true; amount = 25.0f; opacity = 0.75f;
			count = 5; mode = preset = theme = 0; FHGUI::Theme::Accent = FHGUI::Theme::DefaultAccent;
			binding = { 'K', FHGUI::BindMode::Hold, false };
			options = { true, false, true, false, false, false };
		}, 225));

		auto choices = new FHGUI::GroupBox("Choices", 250, 0, 240, 440);
		InputsTab->RegisterControl(choices);
		choices->RegisterControl(new FHGUI::RadioGroup("Quality", { "Low", "Medium", "High" }, &mode, 225));
		choices->RegisterControl(new FHGUI::ListBox("Preset", { "Default", "Balanced", "Performance", "Quality", "Custom", "Minimal" }, &preset, 4, 225, "Arrow keys select. Page Up/Down or Prev/Next scroll the list."));
		choices->RegisterControl(new FHGUI::MultiSelect("Visible elements", { "Names", "Icons", "Labels", "Details", "Status", "Hints" }, &options, 4, 225, "Click to toggle options. Arrow keys move; Space toggles the highlighted row."));

		auto appearance = new FHGUI::GroupBox("Color and binding", 0, 325, 240, 180);
		InputsTab->RegisterControl(appearance);
		appearance->RegisterControl(new FHGUI::KeyBind("Activation", &binding, 225, "Click the key, release, then press a keyboard or mouse button. Escape cancels; Backspace/Delete clears. Click the mode to cycle Hold, Toggle, Always."));
		appearance->RegisterControl(new FHGUI::ColorPicker("Accent color", &FHGUI::Theme::Accent, 225, "Changes UI highlights immediately. Drag RGBA channels, or use Up/Down to select and Left/Right to adjust. Shift changes by 10. Reset inputs restores orange."));
	}

	FHGUI::Tab* AimbotTab = new FHGUI::Tab("Aimbot");
	Window->RegisterTab(AimbotTab);
	{
		FHGUI::GroupBox* AimbotGroup = new FHGUI::GroupBox("Aimbot", 0, 0, 246, 228);
		AimbotTab->RegisterControl(AimbotGroup);
		{
			static bool bEnabled = false;
			static bool bAutoShoot = false;
			static int iSelection = 0;

			FHGUI::CheckBox* EnabledBox = new FHGUI::CheckBox("Enabled", &bEnabled, "Enable/Disable Aimbot.");
			AimbotGroup->RegisterControl(EnabledBox);

			FHGUI::CheckBox* AutoShootBox = new FHGUI::CheckBox("Auto Shoot", &bAutoShoot, "Enable/Disable Weapon Auto Fire.");
			AimbotGroup->RegisterControl(AutoShootBox);

			FHGUI::Dropdown* SelectionDropdown = new FHGUI::Dropdown("Selection", { "Field Of View", "Health", "Damage", "K/D Ratio" }, &iSelection, "Configure Target Selection.");
			AimbotGroup->RegisterControl(SelectionDropdown);
		}

		FHGUI::GroupBox* AccuracyGroup = new FHGUI::GroupBox("Accuracy", 251, 0, 239, 164);
		AimbotTab->RegisterControl(AccuracyGroup);
		{
			static bool bNoRecoil = false;

			FHGUI::CheckBox* NoRecoilBox = new FHGUI::CheckBox("No Recoil", &bNoRecoil, "Enable/Disable No Recoil.");
			AccuracyGroup->RegisterControl(NoRecoilBox);
		}
	}

	FHGUI::Tab* VisualTab = new FHGUI::Tab("Visual");
	Window->RegisterTab(VisualTab);

	FHGUI::Tab* MiscTab = new FHGUI::Tab("Misc");
	Window->RegisterTab(MiscTab);

	FHGUI::Tab* SteamAPITab = new FHGUI::Tab("SteamAPI");
	Window->RegisterTab(SteamAPITab);

	FHGUI::Tab* ConfigTab = new FHGUI::Tab("Config");
	Window->RegisterTab(ConfigTab);

	FHGUI::Instance::Get().RegisterWindow(Window);

	FHGUI::Window* PlayerListWindow = new FHGUI::Window("Player List", 566, 50, 616, 548);
	FHGUI::Instance::Get().RegisterWindow(PlayerListWindow);
}
