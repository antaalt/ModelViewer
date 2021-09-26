#include "AssetEditor.h"

#include "../Model/Model.h"
#include "../Model/ModelLoader.h"

#include <imgui.h>

namespace viewer {

void AssetEditor::import(std::function<bool(const aka::Path&)> callback)
{
	m_currentPath = aka::ResourceManager::path("");
	m_selectedPath = nullptr;
	m_paths = aka::Path::enumerate(m_currentPath);
	m_importCallback = callback;
}

void AssetEditor::onCreate(aka::World& world)
{
	m_currentPath = aka::ResourceManager::path(""); // TODO request project path
	m_selectedPath = nullptr;
	m_paths = aka::Path::enumerate(m_currentPath);
}

void AssetEditor::onDestroy(aka::World& world)
{
}

void AssetEditor::onUpdate(aka::World& world)
{
	// TODO file watcher to ensure everything is up to date
}

template <typename T>
struct ResourceViewer
{
	static void draw(const aka::String& type, aka::ResourceAllocator<T>& resources)
	{
		static bool opened = false;
		static std::pair<String, Resource<T>> pair;
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		bool open = ImGui::TreeNodeEx(type.cstr(), ImGuiTreeNodeFlags_SpanFullWidth);
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
				ImGui::TextUnformatted(type.cstr());
			}
			ImGui::TreePop();
		}
		if (opened)
		{
			if (ImGui::Begin(type.cstr(), &opened))
			{
				draw(pair.first, pair.second);
				ImGui::Separator();
				if (ImGui::TreeNode("Resource"))
				{
					ImGui::Text("Path : %s", pair.second.path.cstr());
					ImGui::Text("Size : %zu", pair.second.size);
					ImGui::Text("Loaded : %zu", pair.second.loaded.milliseconds());
					ImGui::Text("Updated : %zu", pair.second.updated.milliseconds());
					ImGui::TreePop();
				}
			}
			ImGui::End();
		}
	}
	static void draw(const aka::String& name, aka::Resource<T>& resource)
	{
		static const ImVec4 color = ImVec4(0.93f, 0.04f, 0.26f, 1.f);
		ImGui::TextColored(color, name.cstr());
		// TODO we should be able to open this window from anywhere (mostly here and scene editor). 
	}
};

const char* toString(VertexFormat format)
{
	switch (format)
	{
	case aka::VertexFormat::Float:
		return "float";
	case aka::VertexFormat::Double:
		return "double";
	case aka::VertexFormat::Byte:
		return "byte";
	case aka::VertexFormat::UnsignedByte:
		return "unsigned byte";
	case aka::VertexFormat::Short:
		return "short";
	case aka::VertexFormat::UnsignedShort:
		return "unsigned short";
	case aka::VertexFormat::Int:
		return "int";
	case aka::VertexFormat::UnsignedInt:
		return "unsigned int";
	default:
		return "unknown";
	}
}

const char* toString(IndexFormat format)
{
	switch (format)
	{
	case IndexFormat::UnsignedByte:
		return "unsigned byte";
	case IndexFormat::UnsignedShort:
		return "unsigned short";
	case IndexFormat::UnsignedInt:
		return "unsigned int";
	default:
		return "unknown";
	}
}

const char* toString(VertexType type)
{
	switch (type)
	{
	case aka::VertexType::Vec2:
		return "vec2";
	case aka::VertexType::Vec3:
		return "vec3";
	case aka::VertexType::Vec4:
		return "vec4";
	case aka::VertexType::Mat2:
		return "mat2";
	case aka::VertexType::Mat3:
		return "mat3";
	case aka::VertexType::Mat4:
		return "mat4";
	case aka::VertexType::Scalar:
		return "scalar";
	default:
		return "unknown";
	}
}

const char* toString(VertexSemantic semantic)
{
	switch (semantic)
	{
	case aka::VertexSemantic::Position:
		return "position";
	case aka::VertexSemantic::Normal:
		return "normal";
	case aka::VertexSemantic::Tangent:
		return "tangent";
	case aka::VertexSemantic::TexCoord0:
		return "texcoord0";
	case aka::VertexSemantic::TexCoord1:
		return "texcoord1";
	case aka::VertexSemantic::TexCoord2:
		return "texcoord2";
	case aka::VertexSemantic::TexCoord3:
		return "texcoord3";
	case aka::VertexSemantic::Color0:
		return "color0";
	case aka::VertexSemantic::Color1:
		return "color1";
	case aka::VertexSemantic::Color2:
		return "color2";
	case aka::VertexSemantic::Color3:
		return "color3";
	default:
		return "unknown";
	}
}

const char* toString(TextureFormat format)
{
	switch (format)
	{
	case aka::TextureFormat::R8:
		return "R8";
	case aka::TextureFormat::R8U:
		return "R8U";
	case aka::TextureFormat::R16:
		return "R16";
	case aka::TextureFormat::R16U:
		return "R16U";
	case aka::TextureFormat::R16F:
		return "R16F";
	case aka::TextureFormat::R32F:
		return "R32F";
	case aka::TextureFormat::RG8:
		return "RG8";
	case aka::TextureFormat::RG8U:
		return "RG8U";
	case aka::TextureFormat::RG16U:
		return "RG16U";
	case aka::TextureFormat::RG16:
		return "RG16";
	case aka::TextureFormat::RG16F:
		return "RG16F";
	case aka::TextureFormat::RG32F:
		return "RG32F";
	case aka::TextureFormat::RGB8:
		return "RGB8";
	case aka::TextureFormat::RGB8U:
		return "RGB8U";
	case aka::TextureFormat::RGB16:
		return "RGB16";
	case aka::TextureFormat::RGB16U:
		return "RGB16U";
	case aka::TextureFormat::RGB16F:
		return "RGB16F";
	case aka::TextureFormat::RGB32F:
		return "RGB32F";
	case aka::TextureFormat::RGBA8:
		return "RGBA8";
	case aka::TextureFormat::RGBA8U:
		return "RGBA8U";
	case aka::TextureFormat::RGBA16:
		return "RGBA16";
	case aka::TextureFormat::RGBA16U:
		return "RGBA16U";
	case aka::TextureFormat::RGBA16F:
		return "RGBA16F";
	case aka::TextureFormat::RGBA32F:
		return "RGBA32F";
	case aka::TextureFormat::Depth:
		return "Depth";
	case aka::TextureFormat::Depth16:
		return "Depth16";
	case aka::TextureFormat::Depth24:
		return "Depth24";
	case aka::TextureFormat::Depth32:
		return "Depth32";
	case aka::TextureFormat::Depth32F:
		return "Depth32F";
	case aka::TextureFormat::DepthStencil:
		return "DepthStencil";
	case aka::TextureFormat::Depth0Stencil8:
		return "Depth0Stencil8";
	case aka::TextureFormat::Depth24Stencil8:
		return "Depth24Stencil8";
	case aka::TextureFormat::Depth32FStencil8:
		return "Depth32FStencil8";
	default:
		return "Unknown";
	}
}

const char* toString(TextureType type)
{
	switch (type)
	{
	case aka::TextureType::Texture2D:
		return "Texture2D";
	case aka::TextureType::Texture2DMultisample:
		return "Texture2DMultisample";
	case aka::TextureType::TextureCubeMap:
		return "TextureCubemap";
	default:
		return "Unknown";
	}
}

const char* toString(BufferType type)
{
	switch (type)
	{
	case aka::BufferType::Vertex:
		return "VertexBuffer";
	case aka::BufferType::Index:
		return "IndexBuffer";
	default:
		return "Unknown";
	}
}

const char* toString(BufferUsage usage)
{
	switch (usage)
	{
	case aka::BufferUsage::Staging:
		return "Staging";
	case aka::BufferUsage::Immutable:
		return "Immutable";
	case aka::BufferUsage::Dynamic:
		return "Dynamic";
	case aka::BufferUsage::Default:
		return "Default";
	default:
		return "Unknown";
	}
}

const char* toString(BufferCPUAccess access)
{
	switch (access)
	{
	case aka::BufferCPUAccess::Read:
		return "Read";
	case aka::BufferCPUAccess::ReadWrite:
		return "ReadWrite";
	case aka::BufferCPUAccess::Write:
		return "Write";
	case aka::BufferCPUAccess::None:
		return "None";
	default:
		return "Unknown";
	}
}

template <>
static void ResourceViewer<Buffer>::draw(const aka::String& name, aka::Resource<Buffer>& resource)
{
	static const ImVec4 color = ImVec4(0.93f, 0.04f, 0.26f, 1.f);
	ImGui::TextColored(color, name.cstr());

	Buffer::Ptr buffer = resource.resource;
	ImGui::Text("Type : %s", toString(buffer->type()));
	ImGui::Text("Usage : %s", toString(buffer->usage()));
	ImGui::Text("Access : %s", toString(buffer->access()));
	ImGui::Text("Size : %u", buffer->size());
}

template <>
static void ResourceViewer<Mesh>::draw(const aka::String& name, aka::Resource<Mesh>& resource)
{
	static const ImVec4 color = ImVec4(0.93f, 0.04f, 0.26f, 1.f);
	ImGui::TextColored(color, name.cstr());
	aka::Mesh::Ptr mesh = resource.resource;
	ImGui::Text("Vertices");

	for (uint32_t i = 0; i < mesh->getVertexAttributeCount(); i++)
	{
		char buffer[256];
		snprintf(buffer, 256, "Attribute %u", i);
		if (ImGui::TreeNode(buffer))
		{
			ImGui::BulletText("Format : %s", toString(mesh->getVertexAttribute(i).format));
			ImGui::BulletText("Semantic : %s", toString(mesh->getVertexAttribute(i).semantic));
			ImGui::BulletText("Type : %s", toString(mesh->getVertexAttribute(i).type));
			ImGui::BulletText("Count : %u", mesh->getVertexCount(i));
			ImGui::BulletText("Offset : %u", mesh->getVertexOffset(i));
			ImGui::BulletText("Buffer : %s", ResourceManager::name<Buffer>(mesh->getVertexBuffer(i).buffer).cstr());
			ImGui::TreePop();
		}
	}
	ImGui::Separator();
	ImGui::Text("Indices");
	ImGui::BulletText("Format : %s", toString(mesh->getIndexFormat()));
	ImGui::BulletText("Count : %u", mesh->getIndexCount());
	ImGui::BulletText("Buffer : %s", ResourceManager::name<Buffer>(mesh->getIndexBuffer().buffer).cstr());
	// TODO mini mesh viewer
}

template <>
static void ResourceViewer<Texture>::draw(const aka::String& name, aka::Resource<Texture>& resource)
{
	static const ImVec4 color = ImVec4(0.93f, 0.04f, 0.26f, 1.f);
	ImGui::TextColored(color, name.cstr());
	Texture::Ptr texture = resource.resource;
	ImGui::Text("Type : %s", toString(texture->type()));
	ImGui::Text("Format : %s", toString(texture->format()));
	bool isRenderTarget = (texture->flags() & TextureFlag::RenderTarget) == TextureFlag::RenderTarget;
	ImGui::Checkbox("Render target", &isRenderTarget);
	ImGui::Text("Width : %u", texture->width());
	ImGui::Text("Height : %u", texture->height());

	ImTextureID textureID = (ImTextureID)(uintptr_t)texture->handle();
	float ratio = texture->width() / (float)texture->height();
	ImGui::Image(textureID, ImVec2(500 * ratio, 500));
}

template <>
static void ResourceViewer<Font>::draw(const aka::String& name, aka::Resource<Font>& resource)
{
	static const ImVec4 color = ImVec4(0.93f, 0.04f, 0.26f, 1.f);
	ImGui::TextColored(color, name.cstr());
	Font::Ptr font = resource.resource;
	
	ImGui::Text("Family : %s", font->family().cstr());
	ImGui::Text("Style : %s", font->style().cstr());
	ImGui::Text("Count : %zu", font->count());
	int height = font->height();
	if (ImGui::SliderInt("Height", &height, 1, 50) && height != font->height())
	{
		// Resize font
		// TODO This will invalid previous font.
		FontStorage storage;
		if (storage.load(resource.path))
		{
			resource.resource = Font::create(storage.ttf.data(), storage.ttf.size(), height);
		}
	}
	// Display atlas
	Texture::Ptr atlas = font->atlas();
	float uvx = 1.f / (atlas->height() / font->height());
	float uvy = 1.f / (atlas->width() / font->height());

	uint32_t lineCount = 0;
	for (uint32_t i = 0; i < (uint32_t)font->count(); i++)
	{
		const Character& character = font->getCharacter(i);
		ImGui::Image(
			(ImTextureID)character.texture.texture->handle().value(),
			ImVec2(30, 30),
#if defined(ORIGIN_BOTTOM_LEFT)
			ImVec2(character.texture.get(0).u, character.texture.get(0).v + uvy),
			ImVec2(character.texture.get(0).u + uvx, character.texture.get(0).v),
#else
			ImVec2(character.texture.get(0).u, character.texture.get(0).v),
			ImVec2(character.texture.get(0).u + uvx, character.texture.get(0).v + uvy),
#endif
			ImVec4(1, 1, 1, 1),
			ImVec4(1, 1, 1, 1)
		);
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::Text("Advance : %u", character.advance);
			ImGui::Text("Size : (%d, %d)", character.size.x, character.size.y);
			ImGui::Text("Bearing : (%d, %d)", character.bearing.x, character.bearing.y);
			ImVec2 size = ImVec2(300, 300);
			ImGui::Image(
				(ImTextureID)character.texture.texture->handle().value(),
				size,
				ImVec2(0, 0),
				ImVec2(1, 1),
				ImVec4(1, 1, 1, 1),
				ImVec4(1, 1, 1, 1)
			);
			uv2f start = character.texture.get(0);
			uv2f end = character.texture.get(1);
			ImVec2 startVec = ImVec2(ImGui::GetItemRectMin().x + start.u * size.x, ImGui::GetItemRectMin().y + start.v * size.y);
			ImVec2 endVec = ImVec2(ImGui::GetItemRectMin().x + end.u * size.x + 1, ImGui::GetItemRectMin().y + end.v * size.y + 1);
			ImU32 red = (93 << 24) | (4 << 16) | (26 << 8) | (255 << 0);
			ImGui::GetWindowDrawList()->AddRect(startVec, endVec, ImU32(red), 0.f, ImDrawCornerFlags_All, 2.f);
			ImGui::EndTooltip();
		}
		if (lineCount++ < 10)
			ImGui::SameLine();
		else
			lineCount = 0;
	}
}

void AssetEditor::onRender(aka::World& world)
{
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
					aka::ResourceManager::parse("library/library.json");
				}
				if (ImGui::MenuItem("Save"))
				{
					aka::ResourceManager::serialize("library/library.json");
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
						return Importer::importMesh(File::basename(path), path);
					});
				}
				if (ImGui::MenuItem("Texture2D"))
				{
					openImportWindow = true;
					import([&](const aka::Path& path) -> bool {
						aka::Logger::info("Image : ", path);
						return Importer::importTexture2D(File::basename(path), path, TextureFlag::ShaderResource);
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
						return Importer::importAudio(File::basename(path), path);
					});
				}
				if (ImGui::MenuItem("Font"))
				{
					openImportWindow = true;
					import([&](const aka::Path& path) -> bool{
						aka::Logger::info("Font : ", path);
						return Importer::importFont(File::basename(path), path);
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
			ResourceViewer<aka::Texture>::draw("textures", aka::ResourceManager::allocator<aka::Texture>());
			ResourceViewer<aka::AudioStream>::draw("audios", aka::ResourceManager::allocator<aka::AudioStream>());
			ResourceViewer<aka::Mesh>::draw("meshes", aka::ResourceManager::allocator<aka::Mesh>());
			ResourceViewer<aka::Font>::draw("fonts", aka::ResourceManager::allocator<aka::Font>());
			ResourceViewer<aka::Buffer>::draw("buffers", aka::ResourceManager::allocator<aka::Buffer>());
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
					bool isFolder = Directory::exist(m_currentPath + path);
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
				m_paths = aka::Path::enumerate(m_currentPath);
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
}

};
