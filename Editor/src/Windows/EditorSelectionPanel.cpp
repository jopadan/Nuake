#define IMGUI_DEFINE_MATH_OPERATORS

#include "EditorSelectionPanel.h"
#include "../Misc/ImGuiTextHelper.h"
#include <src/Scene/Components.h>
#include "src/Scene/Components/FieldTypes.h"

#include "../ComponentsPanel/MaterialEditor.h"
#include "../ComponentsPanel/BoxColliderPanel.h"

#include <src/Rendering/Textures/Material.h>
#include <src/Resource/ResourceLoader.h>
#include <src/Resource/FontAwesome5.h>
#include <src/FileSystem/FileDialog.h>

#include <Engine.h>
#include <src/Resource/SkyResource.h>
#include <src/Resource/Prefab.h>

#include "src/Rendering/Textures/TextureManager.h"

#include <entt/entt.hpp>

#include "Tracy.hpp"

using namespace Nuake;

EditorSelectionPanel::EditorSelectionPanel()
{
	virtualScene = CreateRef<Scene>();
	virtualScene->SetName("Virtual Scene");
	virtualScene->CreateEntity("Camera").AddComponent<CameraComponent>();
	
	RegisterComponentDrawer<LightComponent, &LightPanel::Draw>();
	RegisterComponentDrawer<ModelComponent, &MeshPanel::Draw>(&meshPanel);
	RegisterComponentDrawer<CameraComponent, &CameraPanel::Draw>();
	RegisterComponentDrawer<MeshColliderComponent, &MeshColliderPanel::Draw>();
	RegisterComponentDrawer<CapsuleColliderComponent, &CapsuleColliderPanel::Draw>();
	RegisterComponentDrawer<NetScriptComponent, &NetScriptPanel::Draw>();
	RegisterComponentDrawer<CylinderColliderComponent, &CylinderColliderPanel::Draw>();
	RegisterComponentDrawer<CharacterControllerComponent, &CharacterControllerPanel::Draw>();
	RegisterComponentDrawer<BoneComponent, &BonePanel::Draw>();
	RegisterComponentDrawer<NavMeshVolumeComponent, &NavMeshVolumePanel::Draw>();

	RegisterTypeDrawer<bool, &EditorSelectionPanel::DrawFieldTypeBool>(this);
	RegisterTypeDrawer<float, &EditorSelectionPanel::DrawFieldTypeFloat>(this);
	RegisterTypeDrawer<Vector2, &EditorSelectionPanel::DrawFieldTypeVector2>(this);
	RegisterTypeDrawer<Vector3, &EditorSelectionPanel::DrawFieldTypeVector3>(this);
	RegisterTypeDrawer<std::string, &EditorSelectionPanel::DrawFieldTypeString>(this);
	RegisterTypeDrawer<ResourceFile, &EditorSelectionPanel::DrawFieldTypeResourceFile>(this);
	RegisterTypeDrawer<DynamicItemList, &EditorSelectionPanel::DrawFieldTypeDynamicItemList>(this);
}

void EditorSelectionPanel::ResolveFile(Ref<Nuake::File> file)
{
    using namespace Nuake;

    currentFile = file;

    if (currentFile->GetExtension() == ".project")
    {

    }

    if (currentFile->GetExtension() == ".material")
    {
        Ref<Material> material = ResourceLoader::LoadMaterial(currentFile->GetRelativePath());
        selectedResource = material;
    }

	if (currentFile->GetFileType() == FileType::Sky)
	{
		Ref<SkyResource> sky = ResourceLoader::LoadSky(currentFile->GetRelativePath());
		selectedResource = sky;
	}
}

void EditorSelectionPanel::Draw(EditorSelection selection, const std::string& id)
{
	const std::string& windowID = id.empty() ? "Properties" : std::string("Properties##" + id);
    if (ImGui::Begin(windowID.c_str()))
    {
        switch (selection.Type)
        {
            case EditorSelectionType::None:
            {
                DrawNone();
                break;
            }

            case EditorSelectionType::Entity:
            {
                DrawEntity(selection.Entity);
                break;
            }
            case EditorSelectionType::File:
            {
                if (currentFile != selection.File)
                {
                    ResolveFile(selection.File);
                }

				if (!selection.File->Exist())
				{
					std::string text = "File is invalid";
					auto windowWidth = ImGui::GetWindowSize().x;
					auto windowHeight = ImGui::GetWindowSize().y;

					auto textWidth = ImGui::CalcTextSize(text.c_str()).x;
					auto textHeight = ImGui::CalcTextSize(text.c_str()).y;
					ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
					ImGui::SetCursorPosY((windowHeight - textHeight) * 0.5f);

					ImGui::TextColored({1.f, 0.1f, 0.1f, 1.0f}, text.c_str());
				}

                DrawFile(selection.File);
                break;
            }
            case EditorSelectionType::Resource:
            {
                DrawResource(selection.Resource);
                break;
            }
        }
    }
    ImGui::End();

}

void EditorSelectionPanel::DrawNone()
{
    std::string text = "No selection";
    auto windowWidth = ImGui::GetWindowSize().x;
    auto windowHeight = ImGui::GetWindowSize().y;

    auto textWidth = ImGui::CalcTextSize(text.c_str()).x;
    auto textHeight = ImGui::CalcTextSize(text.c_str()).y;
    ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
    ImGui::SetCursorPosY((windowHeight - textHeight) * 0.5f);

    ImGui::Text(text.c_str());
}

void EditorSelectionPanel::DrawEntity(Nuake::Entity entity)
{
	ZoneScoped;

	if (!entity.IsValid())
	{
		return;
	}

    DrawAddComponentMenu(entity);

	mTransformPanel.Draw(entity);

	entt::registry& registry = entity.GetScene()->m_Registry;
	for (auto&& [componentTypeId, storage] : registry.storage())
	{
		entt::type_info componentType = storage.type();
		
		entt::entity entityId = static_cast<entt::entity>(entity.GetHandle());
		if (storage.contains(entityId))
		{
			entt::meta_type type = entt::resolve(componentType);
			entt::meta_any component = type.from_void(storage.value(entityId));
			
			ComponentTypeTrait typeTraits = type.traits<ComponentTypeTrait>();
			// Component not exposed as an inspector panel
			if ((typeTraits & ComponentTypeTrait::InspectorExposed) == ComponentTypeTrait::None)
			{
				continue;
			}
			
			DrawComponent(entity, component);
		}
	}

	using namespace Nuake;
	
	float availWidth = ImGui::GetContentRegionAvail().x;
	const float buttonWidth = 200.f;
	float posX = (availWidth / 2.f) - (buttonWidth / 2.f);
	ImGui::SetCursorPosX(posX);
	
	if (UI::PrimaryButton("Add Component", { buttonWidth, 32 }))
	{
		ImGui::OpenPopup("ComponentPopup");
	}

	if (ImGui::BeginPopup("ComponentPopup"))
	{
		for(auto [fst, component] : entt::resolve())
		{
			std::string componentName = Component::GetName(component);
			if (ImGui::MenuItem(componentName.c_str()))
			{
				entity.AddComponent(component);
			}
		}
		
		ImGui::EndPopup();
	}

}

void EditorSelectionPanel::DrawAddComponentMenu(Nuake::Entity entity)
{
	using namespace Nuake;
    if (entity.HasComponent<NameComponent>())
    {
		UIFont* boldIconFont = new UIFont(Fonts::Icons);
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8.0f);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4.0f);
		ImGui::Text(ICON_FA_BOX);
		delete boldIconFont;

		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 2.0f);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 4.0f);
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
		UIFont* boldFont = new UIFont(Fonts::Bold);
        auto& entityName = entity.GetComponent<NameComponent>().Name;


        ImGuiTextSTD("##Name", entityName);
		delete boldFont;

		ImGui::PopStyleColor();
    }
    
}

void EditorSelectionPanel::DrawFile(Ref<Nuake::File> file)
{
    using namespace Nuake;
	switch (file->GetFileType())
	{
		case FileType::Material:
		{
			MaterialEditor matEditor;
			matEditor.Draw(std::static_pointer_cast<Material>(selectedResource));
			break;
		}
		case FileType::Project:
		{
			DrawProjectPanel(Nuake::Engine::GetProject());
			break;
		}
		case FileType::Script:
		{
			break;
		}
		case FileType::NetScript:
		{
			DrawNetScriptPanel(file);
			break;
		}
		case FileType::Prefab:
		{
			//Ref<Prefab> prefab = CreateRef<Prefab>(file->GetRelativePath());
			//DrawPrefabPanel(prefab);
			break;
		}
		case FileType::Sky:
		{
			auto sky = std::static_pointer_cast<SkyResource>(selectedResource);
			std::string skyName = sky->Path;
			{
				UIFont boldfont = UIFont(Fonts::SubTitle);
				ImGui::Text(sky->Path.c_str());

			}
			ImGui::SameLine();
			{
				UIFont boldfont = UIFont(Fonts::Icons);
				if (ImGui::Button(ICON_FA_SAVE))
				{
					if (ResourceManager::IsResourceLoaded(sky->ID))
					{
						ResourceManager::RegisterResource(sky);
					}

					std::string fileData = sky->Serialize().dump(4);

					FileSystem::BeginWriteFile(sky->Path);
					FileSystem::WriteLine(fileData);
					FileSystem::EndWriteFile();
				}
			}

			int textureId = 0;

			// Top
			ImGui::Text("Top");
			if (auto topTexture = sky->GetFaceTexture(SkyFaces::Top); 
				!topTexture.empty())
			{
				textureId = TextureManager::Get()->GetTexture(FileSystem::RelativeToAbsolute(topTexture))->GetID();
			}

			if (ImGui::ImageButtonEx(ImGui::GetCurrentWindow()->GetID("#skytexture1"), (void*)textureId, ImVec2(80, 80), ImVec2(0, 1), ImVec2(1, 0), ImVec4(0, 0, 0, 0), ImVec4(1, 1, 1, 1)))
			{
				std::string texture = FileDialog::OpenFile("*.png | *.jpg");
				if (!texture.empty())
				{
					sky->SetTextureFace(SkyFaces::Top, FileSystem::AbsoluteToRelative(texture));
				}
			}

			if (ImGui::BeginPopupContextWindow())
			{
				if (ImGui::MenuItem("Clear Texture"))
				{
					sky->SetTextureFace(SkyFaces::Top, "");
				}
				ImGui::EndPopup();
			}

			textureId = 0;

			ImGui::Text("Bottom");
			if (auto bottomTexture = sky->GetFaceTexture(SkyFaces::Bottom);
				!bottomTexture.empty())
			{
				textureId = TextureManager::Get()->GetTexture(FileSystem::RelativeToAbsolute(bottomTexture))->GetID();
			}

			if (ImGui::ImageButtonEx(ImGui::GetCurrentWindow()->GetID("#skytexture2"), (void*)textureId, ImVec2(80, 80), ImVec2(0, 1), ImVec2(1, 0), ImVec4(0, 0, 0, 0), ImVec4(1, 1, 1, 1)))
			{
				std::string texture = FileDialog::OpenFile("*.png | *.jpg");
				if (!texture.empty())
				{
					sky->SetTextureFace(SkyFaces::Bottom, FileSystem::AbsoluteToRelative(texture));
				}
			}

			if (ImGui::BeginPopupContextWindow())
			{
				if (ImGui::MenuItem("Clear Texture"))
				{
					sky->SetTextureFace(SkyFaces::Bottom, "");
				}
				ImGui::EndPopup();
			}

			textureId = 0;

			ImGui::Text("Left");
			if (auto bottomTexture = sky->GetFaceTexture(SkyFaces::Left);
				!bottomTexture.empty())
			{
				textureId = TextureManager::Get()->GetTexture(FileSystem::RelativeToAbsolute(bottomTexture))->GetID();
			}

			if (ImGui::ImageButtonEx(ImGui::GetCurrentWindow()->GetID("#skytexture3"), (void*)textureId, ImVec2(80, 80), ImVec2(0, 1), ImVec2(1, 0), ImVec4(0, 0, 0, 0), ImVec4(1, 1, 1, 1)))
			{
				std::string texture = FileDialog::OpenFile("*.png | *.jpg");
				if (!texture.empty())
				{
					sky->SetTextureFace(SkyFaces::Left, FileSystem::AbsoluteToRelative(texture));
				}
			}

			if (ImGui::BeginPopupContextWindow())
			{
				if (ImGui::MenuItem("Clear Texture"))
				{
					sky->SetTextureFace(SkyFaces::Left, "");
				}
				ImGui::EndPopup();
			}

			textureId = 0;

			ImGui::Text("Right");
			if (auto bottomTexture = sky->GetFaceTexture(SkyFaces::Right);
				!bottomTexture.empty())
			{
				textureId = TextureManager::Get()->GetTexture(FileSystem::RelativeToAbsolute(bottomTexture))->GetID();
			}

			if (ImGui::ImageButtonEx(ImGui::GetCurrentWindow()->GetID("#skytexture4"), (void*)textureId, ImVec2(80, 80), ImVec2(0, 1), ImVec2(1, 0), ImVec4(0, 0, 0, 0), ImVec4(1, 1, 1, 1)))
			{
				std::string texture = FileDialog::OpenFile("*.png | *.jpg");
				if (!texture.empty())
				{
					sky->SetTextureFace(SkyFaces::Right, FileSystem::AbsoluteToRelative(texture));
				}
			}

			if (ImGui::BeginPopupContextWindow())
			{
				if (ImGui::MenuItem("Clear Texture"))
				{
					sky->SetTextureFace(SkyFaces::Right, "");
				}
				ImGui::EndPopup();
			}

			textureId = 0;

			ImGui::Text("Front");
			if (auto bottomTexture = sky->GetFaceTexture(SkyFaces::Front);
				!bottomTexture.empty())
			{
				textureId = TextureManager::Get()->GetTexture(FileSystem::RelativeToAbsolute(bottomTexture))->GetID();
			}

			if (ImGui::ImageButtonEx(ImGui::GetCurrentWindow()->GetID("#skytexture5"), (void*)textureId, ImVec2(80, 80), ImVec2(0, 1), ImVec2(1, 0), ImVec4(0, 0, 0, 0), ImVec4(1, 1, 1, 1)))
			{
				std::string texture = FileDialog::OpenFile("*.png | *.jpg");
				if (!texture.empty())
				{
					sky->SetTextureFace(SkyFaces::Front, FileSystem::AbsoluteToRelative(texture));
				}
			}

			if (ImGui::BeginPopupContextWindow())
			{
				if (ImGui::MenuItem("Clear Texture"))
				{
					sky->SetTextureFace(SkyFaces::Front, "");
				}
				ImGui::EndPopup();
			}

			textureId = 0;

			ImGui::Text("Back");
			if (auto bottomTexture = sky->GetFaceTexture(SkyFaces::Back);
				!bottomTexture.empty())
			{
				textureId = TextureManager::Get()->GetTexture(FileSystem::RelativeToAbsolute(bottomTexture))->GetID();
			}

			if (ImGui::ImageButtonEx(ImGui::GetCurrentWindow()->GetID("#skytexture6"), (void*)textureId, ImVec2(80, 80), ImVec2(0, 1), ImVec2(1, 0), ImVec4(0, 0, 0, 0), ImVec4(1, 1, 1, 1)))
			{
				std::string texture = FileDialog::OpenFile("*.png | *.jpg");
				if (!texture.empty())
				{
					sky->SetTextureFace(SkyFaces::Back, FileSystem::AbsoluteToRelative(texture));
				}
			}

			if (ImGui::BeginPopupContextWindow())
			{
				if (ImGui::MenuItem("Clear Texture"))
				{
					sky->SetTextureFace(SkyFaces::Back, "");
				}
				ImGui::EndPopup();
			}

			
			break;
		}
	}
}

void EditorSelectionPanel::DrawResource(Nuake::Resource resource)
{

}

void EditorSelectionPanel::DrawPrefabPanel(Ref<Nuake::Prefab> prefab)
{
	  
}

void EditorSelectionPanel::DrawMaterialPanel(Ref<Nuake::Material> material)
{
    using namespace Nuake;

    std::string materialTitle = material->Path;
    {
        UIFont boldfont = UIFont(Fonts::SubTitle);
        ImGui::Text(material->Path.c_str());

    }
    ImGui::SameLine();
    {
        UIFont boldfont = UIFont(Fonts::Icons);
        if (ImGui::Button(ICON_FA_SAVE))
        {
            std::string fileData = material->Serialize().dump(4);

            FileSystem::BeginWriteFile(material->Path);
            FileSystem::WriteLine(fileData);
            FileSystem::EndWriteFile();
        }
    }

    bool flagsHeaderOpened;
    {
		UIFont boldfont = UIFont(Fonts::Bold); 
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, 0.f));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.f, 8.f));
		flagsHeaderOpened = ImGui::CollapsingHeader(" FLAGS", ImGuiTreeNodeFlags_DefaultOpen);
		ImGui::PopStyleVar(2);
    }
	
    if (flagsHeaderOpened)
    {
		ImGui::BeginTable("##Flags", 3, ImGuiTableFlags_BordersInner);
		{
			ImGui::TableSetupColumn("name", 0, 0.3f);
			ImGui::TableSetupColumn("set", 0, 0.6f);
			ImGui::TableSetupColumn("reset", 0, 0.1f);
			ImGui::TableNextColumn();

			ImGui::Text("Unlit");
			ImGui::TableNextColumn();

			bool unlit = material->data.u_Unlit == 1;
			ImGui::Checkbox("Unlit", &unlit);
			material->data.u_Unlit = (int)unlit;
		}
		ImGui::EndTable();
    }

	const auto TexturePanelHeight = 100;
    const ImVec2 TexturePanelSize = ImVec2(0, TexturePanelHeight);
    bool AlbedoOpened;
	{
		UIFont boldfont = UIFont(Fonts::Bold);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, 0.f));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.f, 8.f));
        AlbedoOpened = ImGui::CollapsingHeader("Albedo", ImGuiTreeNodeFlags_DefaultOpen);
		ImGui::PopStyleVar(2);
	}

	if (AlbedoOpened)
	{
        ImGui::BeginChild("##albedo", TexturePanelSize, true);
        {
			uint32_t textureID = 0;
			if (material->HasAlbedo())
			{
				textureID = material->m_Albedo->GetID();
			}

			if (ImGui::ImageButtonEx(ImGui::GetCurrentWindow()->GetID("#image1"), (void*)textureID, ImVec2(80, 80), ImVec2(0, 1), ImVec2(1, 0), ImVec4(0, 0, 0, 0), ImVec4(1, 1, 1, 1)))
			{
				std::string texture = FileDialog::OpenFile("*.png | *.jpg");
				if (texture != "")
				{
					material->SetAlbedo(TextureManager::Get()->GetTexture(texture));
				}
			}

			if (ImGui::BeginPopupContextWindow())
			{
				if (ImGui::MenuItem("Clear Texture"))
				{
					material->m_Albedo = nullptr;
				}
				ImGui::EndPopup();
			}

			ImGui::SameLine();
			ImGui::ColorEdit3("Color", &material->data.m_AlbedoColor.r);
        }
        ImGui::EndChild();
	}

	if (ImGui::CollapsingHeader("Normal", ImGuiTreeNodeFlags_DefaultOpen))
	{
        ImGui::BeginChild("##normal", TexturePanelSize, true);
        {
			uint32_t textureID = 0;
			if (material->HasNormal())
			{
				textureID = material->m_Normal->GetID();
			}

			if (ImGui::ImageButtonEx(ImGui::GetCurrentWindow()->GetID("#image3"), (void*)textureID, ImVec2(80, 80), ImVec2(0, 1), ImVec2(1, 0), ImVec4(0, 0, 0, 1), ImVec4(1, 1, 1, 1)))
			{
				std::string texture = FileDialog::OpenFile("*.png | *.jpg");
				if (texture != "")
				{
					material->SetNormal(TextureManager::Get()->GetTexture(texture));
				}
			}

			if (ImGui::BeginPopupContextWindow())
			{
				if (ImGui::MenuItem("Clear Texture"))
				{
					material->m_Normal = nullptr;
				}
				ImGui::EndPopup();
			}
        }
        ImGui::EndChild();
	}

	if (ImGui::CollapsingHeader("AO", ImGuiTreeNodeFlags_DefaultOpen))
	{
        ImGui::BeginChild("##ao", TexturePanelSize, true);
        {
			uint32_t textureID = 0;
			if (material->HasAO())
			{
				textureID = material->m_AO->GetID();
			}

			if (ImGui::ImageButtonEx(ImGui::GetCurrentWindow()->GetID("#image2"), (void*)textureID, ImVec2(80, 80), ImVec2(0, 1), ImVec2(1, 0), ImVec4(1, 1, 1, 1), ImVec4(1, 1, 1, 1)))
			{
				std::string texture = FileDialog::OpenFile("Image files (*.png) | *.png | Image files (*.jpg) | *.jpg");
				if (texture != "")
				{
					material->SetAO(TextureManager::Get()->GetTexture(texture));
				}
			}

			if (ImGui::BeginPopupContextWindow())
			{
				if (ImGui::MenuItem("Clear Texture"))
				{
					material->m_AO = nullptr;
				}
				ImGui::EndPopup();
			}
        }
        ImGui::EndChild();
	}
	
	if (ImGui::CollapsingHeader("Metalness", ImGuiTreeNodeFlags_DefaultOpen))
	{
        ImGui::BeginChild("##metalness", TexturePanelSize, true);
        {
			uint32_t textureID = 0;
			if (material->HasMetalness())
			{
				textureID = material->m_Metalness->GetID();
			}

			if (ImGui::ImageButtonEx(ImGui::GetCurrentWindow()->GetID("#image4"), (void*)textureID, ImVec2(80, 80), ImVec2(0, 1), ImVec2(1, 0), ImVec4(0, 0, 0, 1), ImVec4(1, 1, 1, 1)))
			{
				std::string texture = FileDialog::OpenFile("*.png | *.jpg");
				if (texture != "")
				{
					material->SetMetalness(TextureManager::Get()->GetTexture(texture));
				}
			}

			if (ImGui::BeginPopupContextWindow())
			{
				if (ImGui::MenuItem("Clear Texture"))
				{
					material->m_Metalness = nullptr;
				}
				ImGui::EndPopup();
			}

			ImGui::SameLine();
			ImGui::DragFloat("Value##4", &material->data.u_MetalnessValue, 0.01f, 0.0f, 1.0f);
        }
        ImGui::EndChild();
	}

	if (ImGui::CollapsingHeader("Roughness", ImGuiTreeNodeFlags_DefaultOpen))
	{
        ImGui::BeginChild("##roughness", TexturePanelSize, true);
        {
			uint32_t textureID = 0;
			if (material->HasRoughness())
			{
				textureID = material->m_Roughness->GetID();
			}

			if (ImGui::ImageButtonEx(ImGui::GetCurrentWindow()->GetID("#image5"), (void*)textureID, ImVec2(80, 80), ImVec2(0, 1), ImVec2(1, 0), ImVec4(0, 0, 0, 0), ImVec4(1, 1, 1, 1)))
			{
				std::string texture = FileDialog::OpenFile("*.png | *.jpg");
				if (texture != "")
				{
					material->SetRoughness(TextureManager::Get()->GetTexture(texture));
				}
			}

			if (ImGui::BeginPopupContextWindow())
			{
				if (ImGui::MenuItem("Clear Texture"))
				{
					material->m_Roughness = nullptr;
				}
				ImGui::EndPopup();
			}
        }
        ImGui::EndChild();
	}
}

void EditorSelectionPanel::DrawProjectPanel(Ref<Nuake::Project> project)
{
    ImGui::InputText("Project Name", &project->Name);
    ImGui::InputTextMultiline("Project Description", &project->Description);

    if (ImGui::Button("Locate"))
    {
		const std::string& locationPath = Nuake::FileDialog::OpenFile("TrenchBroom (.exe)\0TrenchBroom.exe\0");

		if (!locationPath.empty())
		{
			project->TrenchbroomPath = locationPath;
		}
    }

    ImGui::SameLine();
    ImGui::InputText("Trenchbroom Path", &project->TrenchbroomPath);
}

void EditorSelectionPanel::DrawNetScriptPanel(Ref<Nuake::File> file)
{
	auto filePath = file->GetRelativePath();
	std::string fileContent = Nuake::FileSystem::ReadFile(filePath);

	ImGui::Text("Content");
	ImGui::SameLine(ImGui::GetWindowWidth() - 90);
	if (ImGui::Button("Open..."))
	{
		Nuake::OS::OpenIn(file->GetAbsolutePath());
	}

	ImGui::Separator();

	ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + ImGui::GetWindowWidth());
	ImGui::Text(fileContent.c_str(), ImGui::GetWindowWidth());

	ImGui::PopTextWrapPos();
}

void EditorSelectionPanel::DrawComponent(Nuake::Entity& entity, entt::meta_any& component)
{
	ZoneScoped;

	// Call into custom component drawer if one is available for this component
	
	const auto componentIdHash = component.type().info().hash();
	if (ComponentTypeDrawers.contains(componentIdHash))
	{
		const auto drawerFn = ComponentTypeDrawers[componentIdHash];
		drawerFn(entity, component);
		
		return;
	}

	const entt::meta_type componentMeta = component.type();
	const std::string componentName = Component::GetName(componentMeta);

	UIFont* boldFont = new UIFont(Fonts::Bold);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, 0.f));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.f, 8.f));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
	bool removed = false;
	bool headerOpened = ImGui::CollapsingHeader(componentName.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
    	
	ImGui::PopStyleVar();
	if (strcmp(componentName.c_str(), "TRANSFORM") != 0 && ImGui::BeginPopupContextItem())
	{
		if (ImGui::Selectable("Remove")) { removed = true; }
		ImGui::EndPopup();
	}
	
	if(removed)
	{
		auto componentType = component.type();
		entity.RemoveComponent(componentType);
		ImGui::PopStyleVar();
		delete boldFont;
		Engine::GetProject()->IsDirty = true;
	}
	else if (headerOpened)
	{
		delete boldFont;
		ImGui::PopStyleVar();
		ImGui::Indent();
		
		if (ImGui::BeginTable(componentName.c_str(), 3, ImGuiTableFlags_SizingStretchProp))
		{
			ImGui::TableSetupColumn("name", 0, 0.25f);
			ImGui::TableSetupColumn("set", 0, 0.65f);
			ImGui::TableSetupColumn("reset", 0, 0.1f);
			
			ImGui::TableNextRow();
			
			DrawComponentContent(component);
			
			ImGui::EndTable();
		}
		ImGui::Unindent();
	}
	else
	{
		ImGui::PopStyleVar();
		delete boldFont;
	}
	ImGui::PopStyleVar();
}

void EditorSelectionPanel::DrawComponentContent(entt::meta_any& component)
{
	ZoneScoped;
	
	entt::meta_type componentMeta = component.type();

	// Draw component bound data
	for (auto [fst, dataType] : componentMeta.data())
	{
		const ComponentFieldTrait fieldTraits = dataType.traits<ComponentFieldTrait>();
		// Field marked as internal and thus not exposed to the inspector
		if ((fieldTraits & ComponentFieldTrait::Internal) == ComponentFieldTrait::Internal)
		{
			continue;
		}

		ImGui::TableSetColumnIndex(0);

		// Search for the appropriate drawer for the type
		entt::id_type dataId = dataType.type().id();
		if (FieldTypeDrawers.contains(dataId))
		{
			auto drawerFn = FieldTypeDrawers[dataId];
			drawerFn(dataType, component);
		}
		else
		{
			ImGui::Text("ERR");
		}

		ImGui::TableNextRow();
	}

	// Draw any actions bound to the component
	for (auto [fst, funcMeta] : componentMeta.func())
	{
		const ComponentFuncTrait funcTraits = funcMeta.traits<ComponentFuncTrait>();
		if ((funcTraits & ComponentFuncTrait::Action) == ComponentFuncTrait::Action)
		{
			ImGui::TableSetColumnIndex(0);
			
			std::string funcDisplayName = "";
			auto prop = funcMeta.prop(HashedName::DisplayName).value();
			if (prop)
			{
				funcDisplayName = std::string(*prop.try_cast<const char*>());
			}
			
			std::string buttonName = funcDisplayName;
			if (UI::SecondaryButton(buttonName.c_str()))
			{
				entt::meta_any result = funcMeta.invoke(component);
			}

			ImGui::TableNextRow();
		}
	}
}

void EditorSelectionPanel::DrawFieldTypeFloat(entt::meta_data& field, entt::meta_any& component)
{
	float stepSize = 1.f;
	if (auto prop = field.prop(HashedFieldPropName::FloatStep))
		stepSize = *prop.value().try_cast<float>();
	
	float min = 0.f;
	if (auto prop = field.prop(HashedFieldPropName::FloatMin))
		min = *prop.value().try_cast<float>();
	
	float max = 0.f;
	if (auto prop = field.prop(HashedFieldPropName::FloatMax))
		max = *prop.value().try_cast<float>();

	auto propDisplayName = field.prop(HashedName::DisplayName);
	const char* displayName = *propDisplayName.value().try_cast<const char*>();
	if (displayName != nullptr)
	{
		ImGui::Text(displayName);
		ImGui::TableNextColumn();

		auto fieldVal = field.get(component);
		float* floatPtr = fieldVal.try_cast<float>();
		if (floatPtr != nullptr)
		{
			float floatProxy = *floatPtr;
			const std::string controlId = std::string("##") + displayName;
			if (ImGui::DragFloat(controlId.c_str(), &floatProxy, stepSize, min, max))
			{
				field.set(component, floatProxy);
				Engine::GetProject()->IsDirty = true;
			}
		}
		else
		{
			ImGui::Text("ERR");
		}
	}
}

void EditorSelectionPanel::DrawFieldTypeBool(entt::meta_data& field, entt::meta_any& component)
{
	auto prop = field.prop(HashedName::DisplayName);
	auto propVal = prop.value();
	const char* displayName = *propVal.try_cast<const char*>();
		
	if (displayName != nullptr)
	{
		ImGui::Text(displayName);
		ImGui::TableNextColumn();

		auto fieldVal = field.get(component);
		bool* boolPtr = fieldVal.try_cast<bool>();
		if (boolPtr != nullptr)
		{
			bool boolProxy = *boolPtr;
			std::string controlId = std::string("##") + displayName;
			if (ImGui::Checkbox(controlId.c_str(), &boolProxy))
			{
				field.set(component, boolProxy);
				Engine::GetProject()->IsDirty = true;
			}
		}
		else
		{
			ImGui::Text("ERR");
		}
	}
}

void EditorSelectionPanel::DrawFieldTypeVector3(entt::meta_data& field, entt::meta_any& component)
{
	auto prop = field.prop(HashedName::DisplayName);
	auto propVal = prop.value();
	const char* displayName = *propVal.try_cast<const char*>();
		
	if (displayName != nullptr)
	{
		ImGui::Text(displayName);
		ImGui::TableNextColumn();

		auto fieldVal = field.get(component);
		Vector3* vec3Ptr = fieldVal.try_cast<Vector3>();
		std::string controlId = std::string("##") + displayName;
		ImGui::PushID(controlId.c_str());
		
		if (ImGuiHelper::DrawVec3(controlId, vec3Ptr, 0.5f, 100.0, 0.01f))
		{
			field.set(component, *vec3Ptr);
			Engine::GetProject()->IsDirty = true;
		}

		ImGui::PopID();
	}
}

void EditorSelectionPanel::DrawFieldTypeVector2(entt::meta_data& field, entt::meta_any& component)
{
	auto prop = field.prop(HashedName::DisplayName);
	auto propVal = prop.value();
	const char* displayName = *propVal.try_cast<const char*>();

	if (displayName != nullptr)
	{
		ImGui::Text(displayName);
		ImGui::TableNextColumn();

		auto fieldVal = field.get(component);
		Vector2* vec2Ptr = fieldVal.try_cast<Vector2>();
		std::string controlId = std::string("##") + displayName;
		ImGui::PushID(controlId.c_str());

		if (ImGuiHelper::DrawVec2(controlId, vec2Ptr, 0.5f, 100.0, 0.01f))
		{
			field.set(component, *vec2Ptr);
			Engine::GetProject()->IsDirty = true;
		}

		ImGui::PopID();
	}
}

void EditorSelectionPanel::DrawFieldTypeString(entt::meta_data& field, entt::meta_any& component)
{
	auto prop = field.prop(HashedName::DisplayName);
	auto propVal = prop.value();
	const char* displayName = *propVal.try_cast<const char*>();
		
	if (displayName != nullptr)
	{
		ImGui::Text(displayName);
		ImGui::TableNextColumn();

		auto fieldVal = field.get(component);
		std::string* fieldValPtr = fieldVal.try_cast<std::string>();
		if (fieldValPtr != nullptr)
		{
			std::string fieldValProxy = *fieldValPtr;
			std::string controlId = std::string("##") + displayName;
			ImGui::InputText(controlId.c_str(), &fieldValProxy);

			if (fieldValProxy != fieldVal)
			{
				Engine::GetProject()->IsDirty = true;
			}
		}
		else
		{
			ImGui::Text("ERR");
		}
	}
}

void EditorSelectionPanel::DrawFieldTypeResourceFile(entt::meta_data& field, entt::meta_any& component)
{
	const char* resourceRestrictedType = nullptr;
	if (auto prop = field.prop(HashedFieldPropName::ResourceFileType))
		resourceRestrictedType = *prop.value().try_cast<const char*>();
	
	auto propDisplayName = field.prop(HashedName::DisplayName);
	const char* displayName = *propDisplayName.value().try_cast<const char*>();
	if (displayName != nullptr)
	{
		ImGui::Text(displayName);
		ImGui::TableNextColumn();

		auto fieldVal = field.get(component);
		auto fieldValPtr = fieldVal.try_cast<ResourceFile>();
		if (fieldValPtr != nullptr)
		{
			auto fieldValProxy = *fieldValPtr;
			std::string filePath = fieldValProxy.file == nullptr ? "" : fieldValProxy.file->GetRelativePath();
			std::string controlName = filePath + std::string("##") + displayName;
			ImGui::Button(controlName.c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0));

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(resourceRestrictedType))
				{
					const char* payloadFilePath = static_cast<char*>(payload->Data);
					const std::string fullPath = std::string(payloadFilePath, 256);
					const Ref<Nuake::File> file = FileSystem::GetFile(FileSystem::AbsoluteToRelative(fullPath));
					field.set(component, ResourceFile{ file });
					Engine::GetProject()->IsDirty = true;
				}
				ImGui::EndDragDropTarget();
			}
		}
		else
		{
			ImGui::Text("ERR");
		}
	}
}

void EditorSelectionPanel::DrawFieldTypeDynamicItemList(entt::meta_data& field, entt::meta_any& component)
{
	auto propDisplayName = field.prop(HashedName::DisplayName);
	const char* displayName = *propDisplayName.value().try_cast<const char*>();
	if (displayName != nullptr)
	{
		ImGui::Text(displayName);
		ImGui::TableNextColumn();

		auto fieldVal = field.get(component);
		auto fieldValPtr = fieldVal.try_cast<DynamicItemList>();
		if (fieldValPtr == nullptr)
		{
			ImGui::Text("ERR");
		}

		const auto& items = fieldValPtr->items;
		const int index = fieldValPtr->index;

		// Check first to see if we are within the bounds
		std::string selectedStr = "";
		if (index >= 0 || index < items.size())
		{
			selectedStr = items[index];
		}
			
		std::string controlName = std::string("##") + displayName;
		if (ImGui::BeginCombo(controlName.c_str(), selectedStr.c_str()))
		{
			for (int i = 0; i < items.size(); i++)
			{
				bool isSelected = (index == i);
				std::string name = items[i];

				if (name.empty())
				{
					name = "Empty";
				}

				if (ImGui::Selectable(name.c_str(), isSelected))
				{
					field.set(component, i);
				}

				if (isSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
	}
}

