#pragma once

#include "EditorWindow.h"

#include <imgui.h>

namespace app {

struct MenuEntry 
{
	aka::String name;
	std::function<void(void)> callback;
};

template <typename T>
class AssetViewerEditor : public EditorWindow
{
public:
	AssetViewerEditor(const char* type);
	virtual ~AssetViewerEditor() {}
	virtual void onRender(aka::World& world) override;
	// Set the resource for the viewer.
	void set(const aka::String& name, const aka::Resource<T>& resource) { m_opened = true; m_name = name; m_resource = resource; onResourceChange(); }
protected:
	virtual void draw(const aka::String& name, aka::Resource<T>& resource) = 0;
	virtual void onResourceChange() {}
protected:
	const char* m_type;
	bool m_opened;
	aka::String m_name;
	aka::Resource<T> m_resource;
};

class FontViewerEditor : public AssetViewerEditor<aka::Font>
{
public:
	FontViewerEditor();
protected:
	void draw(const aka::String& name, aka::Resource<aka::Font>& resource) override;
};

class MeshViewerEditor : public AssetViewerEditor<aka::Mesh>
{
public:
	MeshViewerEditor();
	void onCreate(aka::World& world) override;
	void onDestroy(aka::World& world) override;
	void onUpdate(aka::World& world, aka::Time deltaTime) override;
protected:
	void draw(const aka::String& name, aka::Resource<aka::Mesh>& resource) override;
	void onResourceChange() override;
	void drawMesh(const aka::Mesh::Ptr& mesh);
private:
	const uint32_t m_width = 512;
	const uint32_t m_height = 512;
	aka::mat4f m_projection;
	aka::Texture2D::Ptr m_renderTarget;
	aka::Framebuffer::Ptr m_target;
	aka::Material::Ptr m_material;
	aka::Buffer::Ptr m_uniform;
	aka::CameraArcball m_arcball;
};
class BufferViewerEditor : public AssetViewerEditor<aka::Buffer>
{
public:
	BufferViewerEditor();
protected:
	void draw(const aka::String& name, aka::Resource<aka::Buffer>& resource) override;
};

class TextureViewerEditor : public AssetViewerEditor<aka::Texture>
{
public:
	TextureViewerEditor();
protected:
	void draw(const aka::String& name, aka::Resource<aka::Texture>& resource) override;
};

class AudioViewerEditor : public AssetViewerEditor<aka::AudioStream>
{
public:
	AudioViewerEditor();
protected:
	void draw(const aka::String& name, aka::Resource<aka::AudioStream>& resource) override;
};

template<typename T>
inline AssetViewerEditor<T>::AssetViewerEditor(const char* type) :
	m_type(type),
	m_opened(false),
	m_name("None"),
	m_resource{}
{
}

template<typename T>
inline void AssetViewerEditor<T>::onRender(aka::World& world)
{
	if (m_opened)
	{
		// TODO add tabs when multiple resources opened
		if (ImGui::Begin(m_type, &m_opened, ImGuiWindowFlags_MenuBar))
		{
			if (ImGui::BeginMenuBar())
			{
				if (ImGui::BeginMenu("File"))
				{
					// TODO check if dirty
					if (ImGui::MenuItem("Save"))
					{
						if (!aka::Resource<T>::save(m_resource))
							aka::Logger::error("Failed to save resource ", m_name);
					}
					if (ImGui::MenuItem("Reload"))
					{
						auto res = aka::Resource<T>::load(m_resource.path);
						if (res.resource == nullptr)
							aka::Logger::error("Failed to load resource ", m_name);
						else
							for (auto& element : ResourceManager::allocator<T>())
								if (element.first == m_name)
									element.second = res;
					}
					ImGui::EndMenu();
				}
				ImGui::EndMenuBar();
			}
			draw(m_name, m_resource);
			ImGui::Separator();
			if (ImGui::TreeNode("Resource"))
			{
				ImGui::Text("Path : %s", m_resource.path.cstr());
				ImGui::Text("Size : %zu", m_resource.size);
				ImGui::Text("Loaded : %zu", m_resource.loaded.milliseconds());
				ImGui::Text("Updated : %zu", m_resource.updated.milliseconds());
				ImGui::TreePop();
			}
		}
		ImGui::End();
	}
}

};