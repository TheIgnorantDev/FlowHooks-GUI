# FlowHooks-GUI

A Direct3D9 graphical interface inspired by FlowHooks, written in C++ for Windows.
The showcase opens on the **Inputs** tab. Existing example tabs and multiple-window support remain available.

## Build

Open `FlowHooks-GUI.sln` in Visual Studio with the Desktop development with C++ workload and a Windows SDK. The project targets the v142 toolset; install it or retarget to your installed toolset. Build and run `FHGUI Showcase` (Win32 or x64). All configurations use C++20.

## Controls

| Control | Backing value | Interaction |
| --- | --- | --- |
| `CheckBox` | `bool*` | Click the box/label; Space toggles |
| `Dropdown` | `int*`, zero-based index | Click to expand/select; Up/Down changes the selection; Escape closes |
| `TextBox` | `std::string*` | Single-line text with a length limit, caret, selection and horizontal scrolling |
| `NumberInput` | `float*` | Typed number with minimum/maximum and minus/plus step buttons |
| `Slider` | `float*` or `int*` | Drag; arrow keys adjust; Home/End selects a limit; optional float step |
| `Button` | `std::function<void()>` | Click, Space or Enter invokes the callback |
| `RadioGroup` | `int*`, zero-based index | One selected option; click or use arrow keys |
| `ListBox` | `int*`, zero-based index | Single selection; arrows, Home/End, Page Up/Down and Prev/Next paging |
| `MultiSelect` | `std::vector<bool>*` | Independent choices; click or Space toggles, arrows move the cursor |
| `ColorPicker` | `Render::Color*` | RGBA channel sliders, alpha preview and hexadecimal readout |
| `KeyBind` | `FHGUI::KeyBinding*` | Capture a keyboard key or mouse button; Hold, Toggle and Always modes |

Press **Tab / Shift+Tab** to move between controls in the active window and tab. Click a tab heading to change tabs. Drag a window by its title bar. **Insert** shows/hides the interface unless a text field or bind capture owns the keyboard.

Text fields support **Left/Right, Home/End, Shift+navigation, Backspace, Delete, Ctrl+A/C/X/V**. Enter accepts the edit; Escape restores the value from when the field gained focus. Text updates the bound string as you type. Number fields commit on Enter or focus loss, clamp valid finite values to their range, and discard invalid input. Escape discards the pending numeric edit. The existing font atlas contains only printable ASCII, so typing and paste accept ASCII characters 32-126; Unicode and multiline text are not supported.

Open a color picker to drag R/G/B/A channels. Up/Down selects a channel; Left/Right adjusts it by one (Shift: ten). Escape or an outside click closes the popup. Colors use four 8-bit channels in red, green, blue, alpha order; the displayed hex value is `#RRGGBBAA`.

The showcase's **Accent color** picker edits `FHGUI::Theme::Accent` directly. Tab indicators, checkboxes, sliders, radio buttons, selections, focus outlines and active bind highlights update in the same frame, including accent transparency. Selected rows use a darker shade of the accent. **Reset inputs** restores the default orange. To theme your own application, include `FHGUI/Theme.hpp` and change `FHGUI::Theme::Accent` or bind a color picker to it.

Click a bind's key field, release the activating click, then press a key or Mouse 1-5. Escape cancels capture; Backspace/Delete clears the binding. Click its mode field, or focus it and use Left/Right, to cycle Hold, Toggle and Always. Space/Enter starts keyboard capture. Binds contain one Win32 virtual-key code, not a key combination; Escape, Backspace and Delete are reserved capture commands. `Key == 0` means unbound in Hold/Toggle mode. Always mode does not require a key.

`KeyBinding::Active` updates each `Instance::Update()`, including hidden tabs and a hidden menu. Bind activation is suppressed while editing text or capturing another binding. Losing application focus deactivates all bindings and clears toggle state. The showcase highlights the bind title while it is active. Bind state is available to your application; the GUI does not execute an action automatically.

## Registering controls

Create controls after `Render::Init()` because label and tooltip measurements use the menu font. Register each window with the GUI instance, each tab with its window, and each group with its tab **before** registering the group's children. Groups stack children vertically; their height must accommodate the controls you add. Width arguments include the entire control, including numeric step buttons or bind mode selectors.

```cpp
#include "FHGUI/FHGUI.hpp"
#include "FHGUI/Elements/Controls.hpp"
#include "FHGUI/Elements/Window.hpp"

static std::string name = "Default";
static float scale = 1.0f;
static int quality = 1;
static Render::Color accent{255, 146, 0, 255};
static FHGUI::KeyBinding shortcut{'K', FHGUI::BindMode::Toggle, false};

auto window = new FHGUI::Window("Settings", 25, 25, 300, 440);
FHGUI::Instance::Get().RegisterWindow(window);
auto tab = new FHGUI::Tab("General");
window->RegisterTab(tab);
auto group = new FHGUI::GroupBox("Preferences", 0, 0, 270, 340);
tab->RegisterControl(group);

group->RegisterControl(new FHGUI::TextBox("Name", &name, 64, 220));
group->RegisterControl(new FHGUI::NumberInput("Scale", &scale, 0.25f, 4.0f, 0.25f, 220));
group->RegisterControl(new FHGUI::RadioGroup("Quality", {"Low", "Medium", "High"}, &quality, 220));
group->RegisterControl(new FHGUI::KeyBind("Shortcut", &shortcut, 220));
group->RegisterControl(new FHGUI::ColorPicker("Accent", &accent, 220));
group->RegisterControl(new FHGUI::Button("Reset name", [] { name = "Default"; }, 220));
```

The instance owns its windows, windows own tabs, and tabs own registered controls. Do not register an object twice or delete it yourself. Bound values remain caller-owned and must outlive their controls (for example, static settings or an application configuration object). Choice labels are copied into controls. `MultiSelect` resizes its backing vector to the option count, preserving existing entries and initializing new ones to false. Invalid dropdown indices render an empty selection until changed.

## Validation

From a Visual Studio Developer PowerShell, run:

```powershell
./Tests/Run.ps1
```

Alternatively, supply a Zig compiler path:

```powershell
./Tests/Run.ps1 -Compiler C:/tools/zig/zig.exe
```

The headless tests exercise the real controls, focus routing, Win32 text-message handling, bounds, popup click interception, key capture and binding modes, with deterministic frame input and a drawing stub. Test executables are written under `Build/Tests`.

Run `./Tests/Run.ps1 -RenderSmoke` (or add the same `-Compiler` option) for the separate Direct3D9 smoke test. It creates a hidden window, renders the showcase, and writes `Build/Tests/showcase.bmp`. This requires a working Direct3D9 device.
