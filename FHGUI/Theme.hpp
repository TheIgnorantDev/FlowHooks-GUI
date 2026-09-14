#pragma once
#include "../Render/Render.hpp"

namespace FHGUI::Theme
{
	inline constexpr Render::Color DefaultAccent{ 255, 146, 0, 255 };
	inline Render::Color Accent = DefaultAccent;

	// Keep selected rows subdued so their labels stay readable.
	inline Render::Color Selection()
	{
		return {
			static_cast<std::uint8_t>(Accent.red * 0.55f),
			static_cast<std::uint8_t>(Accent.green * 0.55f),
			static_cast<std::uint8_t>(Accent.blue * 0.55f),
			Accent.alpha
		};
	}
}
