// Draw the real showcase to a hidden Direct3D window and save the backbuffer.
#include <fstream>
#include <iostream>
#include <vector>
#include "../DirectX/DirectX.hpp"
#include "../Render/Render.hpp"
#include "../FHGUI/FHGUI.hpp"
#include "../FHGUI/Input.hpp"
#include "../Menu/Menu.hpp"

int main(int argc, char** argv)
{
	WNDCLASSW windowClass{};
	windowClass.lpfnWndProc = DefWindowProcW;
	windowClass.hInstance = GetModuleHandleW(nullptr);
	windowClass.lpszClassName = L"FlowHooksRenderSmoke";
	RegisterClassW(&windowClass);
	HWND window = CreateWindowW(windowClass.lpszClassName, L"Render test", WS_POPUP, 0, 0, 1280, 720,
		nullptr, nullptr, windowClass.hInstance, nullptr);
	if (!window || !DirectX::Get().Init(window)) return 1;
	Render::Init(DirectX::Get().Device());
	FHGUI::Input::Get().Init(window);
	Menu::Init();
	if (!DirectX::Get().BeginRender()) return 2;
	FHGUI::Instance::Get().Render();
	auto device = DirectX::Get().Device();
	device->EndScene();
	IDirect3DSurface9* target = nullptr;
	IDirect3DSurface9* copy = nullptr;
	if (FAILED(device->GetRenderTarget(0, &target))) return 3;
	D3DSURFACE_DESC description{};
	target->GetDesc(&description);
	if (FAILED(device->CreateOffscreenPlainSurface(description.Width, description.Height, description.Format,
		D3DPOOL_SYSTEMMEM, &copy, nullptr))) return 4;
	if (FAILED(device->GetRenderTargetData(target, copy))) return 5;
	D3DLOCKED_RECT pixels{};
	if (FAILED(copy->LockRect(&pixels, nullptr, D3DLOCK_READONLY))) return 6;
	BITMAPFILEHEADER file{};
	BITMAPINFOHEADER info{};
	info.biSize = sizeof(info);
	info.biWidth = static_cast<LONG>(description.Width);
	info.biHeight = -static_cast<LONG>(description.Height);
	info.biPlanes = 1;
	info.biBitCount = 32;
	info.biCompression = BI_RGB;
	info.biSizeImage = description.Width * description.Height * 4;
	file.bfType = 0x4d42;
	file.bfOffBits = sizeof(file) + sizeof(info);
	file.bfSize = file.bfOffBits + info.biSizeImage;
	std::ofstream output(argc > 1 ? argv[1] : "showcase.bmp", std::ios::binary);
	output.write(reinterpret_cast<const char*>(&file), sizeof(file));
	output.write(reinterpret_cast<const char*>(&info), sizeof(info));
	for (UINT y = 0; y < description.Height; ++y)
		output.write(static_cast<const char*>(pixels.pBits) + y * pixels.Pitch, description.Width * 4);
	copy->UnlockRect();
	copy->Release(); target->Release();
	Render::Shutdown();
	DirectX::Get().Shutdown();
	DestroyWindow(window);
	UnregisterClassW(windowClass.lpszClassName, windowClass.hInstance);
	std::cout << "PASS: rendered the showcase using Direct3D9\n";
	return output ? 0 : 7;
}
