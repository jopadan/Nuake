#pragma once
#include "src/Core/Core.h"
#include "src/Core/Maths.h"

#include "imgui/imgui.h"
#include "../../../Editor/src/Misc/InterfaceFonts.h"

#include "src/Resource/FontAwesome5.h"

namespace Nuake
{
	namespace UI
	{
		static uint32_t PrimaryCol = IM_COL32(97, 0, 255, 255);
		static uint32_t GrabCol = IM_COL32(97, 0, 255, 255);

		static ImVec2 ButtonPadding = ImVec2(16.0f, 8.0f);
		static ImVec2 IconButtonPadding = ImVec2(8.0f, 8.0f);

		void BeginWindow(const std::string& name);

		void EndWindow();

		bool PrimaryButton(const std::string& name, const Vector2& size = {0, 0});
		bool SecondaryButton(const std::string& name, const Vector2& size = { 0, 0 });

		bool IconButton(const std::string& icon);

		bool FloatSlider(const std::string& name, float& input, float min = 0.0f, float max = 1.0f, float speed = 0.01f);

		bool CheckBox(const std::string& name, bool& value);

		void ToggleButton(const char* str_id, bool* v);
	}
}