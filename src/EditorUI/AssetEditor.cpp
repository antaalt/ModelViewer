#include "AssetEditor.h"

#include "../Model/Model.h"
#include "../Model/Importer.h"

#include <imgui.h>

namespace app {

void AssetEditor::import(std::function<bool(const aka::Path&)> callback)
{
	m_currentPath = ResourceManager::path("");
	m_selectedPath = nullptr;
	m_paths = aka::OS::enumerate(m_currentPath);
	m_importCallback = callback;
}

void AssetEditor::onCreate(aka::World& world)
{
	m_currentPath = ResourceManager::path(""); // TODO request project path
	m_selectedPath = nullptr;
	m_paths = aka::OS::enumerate(m_currentPath);
	m_viewers.push_back(&m_textureEditor);
	m_viewers.push_back(&m_bufferEditor);
	m_viewers.push_back(&m_meshEditor);
	m_viewers.push_back(&m_audioEditor);
	m_viewers.push_back(&m_fontEditor);
	for (EditorWindow* viewer : m_viewers)
		viewer->onCreate(world);
}

void AssetEditor::onDestroy(aka::World& world)
{
	for (EditorWindow* viewer : m_viewers)
		viewer->onDestroy(world);
}

void AssetEditor::onUpdate(aka::World& world, Time deltaTime)
{
	// TODO file watcher to ensure everything is up to date
	for (EditorWindow* viewer : m_viewers)
		viewer->onUpdate(world, deltaTime);
}

template <typename T>
static void drawResource(const char* type, AssetViewerEditor<T>* viewer)
{
	Application* app = Application::app();
	aka::ResourceAllocator<T>& resources = app->resource()->allocator<T>();
	bool opened = false;
	std::pair<String, Resource<T>> pair;
	ImGui::TableNextRow();
	ImGui::TableNextColumn();
	bool open = ImGui::TreeNodeEx(type, ImGuiTreeNodeFlags_SpanFullWidth);
	ImGui::TableNextColumn();
	ImGui::TextDisabled("--");
	ImGui::TableNextColumn();
	ImGui::TextDisabled("%zu", resources.count());
	ImGui::TableNextColumn();
	ImGui::TextDisabled("--"); // TODO compute total size ?
	ImGui::TableNextColumn();
	ImGui::TextDisabled("--");
	if (open)
	{
		for (auto& element : resources)
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TreeNodeEx(element.first.cstr(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanFullWidth);
			if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(0))
			{
				opened = true;
				pair = element;
			}
			ImGui::TableNextColumn();
			ImGui::Text("%s", element.second.path.cstr());
			ImGui::TableNextColumn();
			ImGui::Text("%ld", element.second.resource.use_count() - 1); // Remove self use (within allocator)
			ImGui::TableNextColumn();
			ImGui::Text("%ld bytes", element.second.size);
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(type);
		}
		ImGui::TreePop();
	}
	if (opened)
		viewer->set(pair.first, pair.second);
}

void AssetEditor::onRender(aka::World& world, aka::gfx::Frame* frame)
{
	Application* app = Application::app();
	ResourceManager* resources = app->resource();
	// store every texture, mesh & things, responsible of loading and unloading files
	if (ImGui::Begin("Assets", nullptr, ImGuiWindowFlags_MenuBar))
	{
		bool openImportWindow = false;
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Load"))
				{
					resources->parse("library/library.json");
				}
				if (ImGui::MenuItem("Save"))
				{
					resources->serialize("library/library.json");
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Import"))
			{
				// Importing something will create a custom file depending on type and add it to library folder.
				// It will be added to a manifest.json, with a name, a path, and a description of the imported file.
				if (ImGui::MenuItem("Scene"))
				{
					openImportWindow = true;
					import([&](const aka::Path& path) -> bool{
						aka::Logger::info("Scene : ", path);
						return Importer::importScene(path, world);
					});
				}
				if (ImGui::MenuItem("Mesh"))
				{
					openImportWindow = true;
					import([&](const aka::Path& path) -> bool{
						aka::Logger::info("Mesh : ", path);
						return Importer::importMesh(OS::File::basename(path), path);
					});
				}
				if (ImGui::MenuItem("Texture2D"))
				{
					openImportWindow = true;
					import([&](const aka::Path& path) -> bool {
						aka::Logger::info("Image : ", path);
						return Importer::importTexture2D(OS::File::basename(path), path, gfx::TextureFlag::ShaderResource);
					});
				}
				if (ImGui::MenuItem("Cubemap"))
				{
					openImportWindow = true;
					import([&](const aka::Path& path) -> bool{
						aka::Logger::info("Image : ", path);
						//return Importer::importTextureCubemap(File::basename(path), path);
						return false;
					});
				}
				if (ImGui::MenuItem("Audio"))
				{
					openImportWindow = true;
					import([&](const aka::Path& path) -> bool{
						aka::Logger::info("Audio : ", path);
						return Importer::importAudio(OS::File::basename(path), path);
					});
				}
				if (ImGui::MenuItem("Font"))
				{
					openImportWindow = true;
					import([&](const aka::Path& path) -> bool{
						aka::Logger::info("Font : ", path);
						return Importer::importFont(OS::File::basename(path), path);
					});
				}
				// animation
				// sprite
				// script
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Create"))
			{
				if (ImGui::MenuItem("Mesh"))
				{
					// TODO open a dialog to setup the mesh ?
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}
		// --- Then we list all imported resources stored in folder
		static ImGuiTableFlags flags = ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody;

		if (ImGui::BeginTable("Resources", 5, flags))
		{
			ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
			ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_NoHide);
			ImGui::TableSetupColumn("Count", ImGuiTableColumnFlags_WidthFixed, 40.f);
			ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 40.f);
			ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 90.f);
			ImGui::TableHeadersRow();
			drawResource<gfx::Texture>("textures", &m_textureEditor);
			drawResource<AudioStream>("audios", &m_audioEditor);
			drawResource<Mesh>("meshes", &m_meshEditor);
			drawResource<Font>("fonts", &m_fontEditor);
			drawResource<gfx::Buffer>("buffers", &m_bufferEditor);
			// TODO support other resources
			ImGui::EndTable();
		}

		// --- We draw the import window
		if (openImportWindow)
			ImGui::OpenPopup("Import##Popup");
		bool openFlag = true;
		if (ImGui::BeginPopupModal("Import##Popup", &openFlag, ImGuiWindowFlags_AlwaysAutoResize))
		{
			bool updated = false;
			if (ImGui::Button("^"))
			{
				m_currentPath = m_currentPath.up();
				updated = true;
			}
			// Refresh directory
			ImGui::SameLine();
			if (ImGui::Button("*"))
			{
				updated = true;
			}
			ImGui::SameLine();
			// Path
			char currentPathBuffer[256];
			aka::String::copy(currentPathBuffer, 256, m_currentPath.cstr());
			if (ImGui::InputText("##Path", currentPathBuffer, 256))
			{
				m_currentPath = currentPathBuffer;
				updated = true;
			}
			if (ImGui::BeginChild("##files", ImVec2(0, 200), true))
			{
				char buffer[256];
				for (aka::Path& path : m_paths)
				{
					bool selected = (&path == m_selectedPath);
					bool isFolder = OS::Directory::exist(m_currentPath + path);
					if (isFolder)
					{
						int err = snprintf(buffer, 256, "%s %s", "D", path.cstr());
						if (ImGui::Selectable(buffer, &selected))
						{
							m_selectedPath = &path;
						}
						if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
						{
							m_currentPath += path;
							updated = true;
						}
					}
					else
					{
						int err = snprintf(buffer, 256, "%s %s", "F", path.cstr());
						if (ImGui::Selectable(buffer, &selected))
						{
							m_selectedPath = &path;
						}
						if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
						{
							if (m_importCallback(m_currentPath + path))
								ImGui::CloseCurrentPopup();
						}
					}
				}
			}
			ImGui::EndChild();
			if (updated)
			{
				m_paths = OS::enumerate(m_currentPath);
			}
			if (ImGui::Button("Import"))
			{
				if (m_selectedPath != nullptr)
				{
					if (m_importCallback(m_currentPath + *m_selectedPath))
						ImGui::CloseCurrentPopup();
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Close"))
			{
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
	}
	ImGui::End();

	for (EditorWindow* viewer : m_viewers)
		viewer->onRender(world, frame);
}

};
