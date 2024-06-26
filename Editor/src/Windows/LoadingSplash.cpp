#include "LoadingSplash.h"

#include "../Misc/InterfaceFonts.h"

#include <src/Core/Maths.h>
#include "src/Window.h"


#include <src/Vendors/imgui/imgui.h>

#include <dependencies/glfw/include/GLFW/glfw3.h>
#include <src/Rendering/Textures/TextureManager.h>


LoadingSplash::LoadingSplash()
{
	_NuakeLogo = Nuake::TextureManager::Get()->GetTexture(NUAKE_LOGO_PATH);

	Nuake::Window::Get()->SetDecorated(false);
	Nuake::Window::Get()->SetSize({ 640, 480 });
	Nuake::Window::Get()->Center();
}

void LoadingSplash::Draw()
{
	using namespace Nuake;

	// Make viewport fullscreen
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);
	ImGui::SetNextWindowViewport(viewport->ID);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(32.0f, 32.0f));
	ImGui::Begin("Welcome Screen", 0, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize);
	{
		// Draw Nuake logo
		{
			const Vector2 logoSize = _NuakeLogo->GetSize();
			const ImVec2 imguiSize = ImVec2(logoSize.x, logoSize.y);
			ImGui::Image((ImTextureID)_NuakeLogo->GetID(), imguiSize, ImVec2(0, 1), ImVec2(1, 0));
		}

		ImGui::Text("LOADING SCENE...");
	}

	ImGui::End();
	ImGui::PopStyleVar();
	ImGui::PopStyleVar();
}