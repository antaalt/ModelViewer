#include "AssetViewerEditor.h"

#include <Aka/Aka.h>

#include <imgui.h>

namespace app {

using namespace aka;

template class AssetViewerEditor<Buffer>;
template class AssetViewerEditor<Texture>;
template class AssetViewerEditor<Mesh>;
template class AssetViewerEditor<AudioStream>;
template class AssetViewerEditor<Font>;

const char* toString(VertexFormat format)
{
	switch (format)
	{
	case VertexFormat::Float:
		return "float";
	case VertexFormat::Double:
		return "double";
	case VertexFormat::Byte:
		return "byte";
	case VertexFormat::UnsignedByte:
		return "unsigned byte";
	case VertexFormat::Short:
		return "short";
	case VertexFormat::UnsignedShort:
		return "unsigned short";
	case VertexFormat::Int:
		return "int";
	case VertexFormat::UnsignedInt:
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
	case VertexType::Vec2:
		return "vec2";
	case VertexType::Vec3:
		return "vec3";
	case VertexType::Vec4:
		return "vec4";
	case VertexType::Mat2:
		return "mat2";
	case VertexType::Mat3:
		return "mat3";
	case VertexType::Mat4:
		return "mat4";
	case VertexType::Scalar:
		return "scalar";
	default:
		return "unknown";
	}
}

const char* toString(VertexSemantic semantic)
{
	switch (semantic)
	{
	case VertexSemantic::Position:
		return "position";
	case VertexSemantic::Normal:
		return "normal";
	case VertexSemantic::Tangent:
		return "tangent";
	case VertexSemantic::TexCoord0:
		return "texcoord0";
	case VertexSemantic::TexCoord1:
		return "texcoord1";
	case VertexSemantic::TexCoord2:
		return "texcoord2";
	case VertexSemantic::TexCoord3:
		return "texcoord3";
	case VertexSemantic::Color0:
		return "color0";
	case VertexSemantic::Color1:
		return "color1";
	case VertexSemantic::Color2:
		return "color2";
	case VertexSemantic::Color3:
		return "color3";
	default:
		return "unknown";
	}
}

const char* toString(TextureFormat format)
{
	switch (format)
	{
	case TextureFormat::R8:
		return "R8";
	case TextureFormat::R8U:
		return "R8U";
	case TextureFormat::R16:
		return "R16";
	case TextureFormat::R16U:
		return "R16U";
	case TextureFormat::R16F:
		return "R16F";
	case TextureFormat::R32F:
		return "R32F";
	case TextureFormat::RG8:
		return "RG8";
	case TextureFormat::RG8U:
		return "RG8U";
	case TextureFormat::RG16U:
		return "RG16U";
	case TextureFormat::RG16:
		return "RG16";
	case TextureFormat::RG16F:
		return "RG16F";
	case TextureFormat::RG32F:
		return "RG32F";
	case TextureFormat::RGB8:
		return "RGB8";
	case TextureFormat::RGB8U:
		return "RGB8U";
	case TextureFormat::RGB16:
		return "RGB16";
	case TextureFormat::RGB16U:
		return "RGB16U";
	case TextureFormat::RGB16F:
		return "RGB16F";
	case TextureFormat::RGB32F:
		return "RGB32F";
	case TextureFormat::RGBA8:
		return "RGBA8";
	case TextureFormat::RGBA8U:
		return "RGBA8U";
	case TextureFormat::RGBA16:
		return "RGBA16";
	case TextureFormat::RGBA16U:
		return "RGBA16U";
	case TextureFormat::RGBA16F:
		return "RGBA16F";
	case TextureFormat::RGBA32F:
		return "RGBA32F";
	case TextureFormat::Depth:
		return "Depth";
	case TextureFormat::Depth16:
		return "Depth16";
	case TextureFormat::Depth24:
		return "Depth24";
	case TextureFormat::Depth32:
		return "Depth32";
	case TextureFormat::Depth32F:
		return "Depth32F";
	case TextureFormat::DepthStencil:
		return "DepthStencil";
	case TextureFormat::Depth0Stencil8:
		return "Depth0Stencil8";
	case TextureFormat::Depth24Stencil8:
		return "Depth24Stencil8";
	case TextureFormat::Depth32FStencil8:
		return "Depth32FStencil8";
	default:
		return "Unknown";
	}
}

const char* toString(TextureType type)
{
	switch (type)
	{
	case TextureType::Texture2D:
		return "Texture2D";
	case TextureType::Texture2DMultisample:
		return "Texture2DMultisample";
	case TextureType::TextureCubeMap:
		return "TextureCubemap";
	default:
		return "Unknown";
	}
}

const char* toString(BufferType type)
{
	switch (type)
	{
	case BufferType::Vertex:
		return "VertexBuffer";
	case BufferType::Index:
		return "IndexBuffer";
	default:
		return "Unknown";
	}
}

const char* toString(BufferUsage usage)
{
	switch (usage)
	{
	case BufferUsage::Staging:
		return "Staging";
	case BufferUsage::Immutable:
		return "Immutable";
	case BufferUsage::Dynamic:
		return "Dynamic";
	case BufferUsage::Default:
		return "Default";
	default:
		return "Unknown";
	}
}

const char* toString(BufferCPUAccess access)
{
	switch (access)
	{
	case BufferCPUAccess::Read:
		return "Read";
	case BufferCPUAccess::ReadWrite:
		return "ReadWrite";
	case BufferCPUAccess::Write:
		return "Write";
	case BufferCPUAccess::None:
		return "None";
	default:
		return "Unknown";
	}
}

void BufferViewerEditor::draw(const String& name, Resource<Buffer>& resource)
{
	/*static const ImVec4 color = ImVec4(0.93f, 0.04f, 0.26f, 1.f);
	ImGui::TextColored(color, name.cstr());

	Buffer::Ptr buffer = resource.resource;
	ImGui::Text("Type : %s", toString(buffer->type()));
	ImGui::Text("Usage : %s", toString(buffer->usage()));
	ImGui::Text("Access : %s", toString(buffer->access()));
	ImGui::Text("Size : %u", buffer->size());*/
}
void MeshViewerEditor::onCreate(World& world)
{
	//// TODO if no data, bug ?
	//std::vector<uint8_t> data(m_width * m_height * 4, 0xff);
	//m_renderTarget = Texture2D::create(m_width, m_height, TextureFormat::RGBA8, TextureFlag::RenderTarget | TextureFlag::ShaderResource, data.data());
	//Attachment attachments[2] = {
	//	{ AttachmentType::Depth, Texture2D::create(m_width, m_height, TextureFormat::Depth, TextureFlag::RenderTarget), AttachmentFlag::None, 0, 0 },
	//	{ AttachmentType::Color0, m_renderTarget, AttachmentFlag::None, 0, 0 },
	//};
	//m_target = Framebuffer::create(attachments, 2);

	//ProgramManager* program = Application::program();
	//Program::Ptr p = program->get("editor.basic");
	//m_material = Material::create(p);
	//m_uniform = Buffer::create(BufferType::Uniform, sizeof(mat4f), BufferUsage::Default, BufferCPUAccess::None);
	//m_material->set("CameraUniformBuffer", m_uniform);
	//m_arcball.set(aabbox<>(point3f(-20.f), point3f(20.f)));
	//m_projection = mat4f::perspective(anglef::degree(90.f), m_width / (float)m_height, 0.1f, 100.f);
}
void MeshViewerEditor::onDestroy(World& world)
{

}
void MeshViewerEditor::onUpdate(aka::World& world, aka::Time deltaTime)
{

}
void MeshViewerEditor::onResourceChange()
{
	// TODO compute mesh bounds from mesh
	m_arcball.set(aabbox<>(point3f(-20.f), point3f(20.f)));
}

void MeshViewerEditor::drawMesh(const Mesh* mesh)
{
	/*RenderPass pass;
	pass.framebuffer = m_target;
	pass.submesh.mesh = mesh;
	pass.submesh.count = mesh->isIndexed() ? mesh->getIndexCount() : mesh->getVertexCount(0);
	pass.submesh.offset = 0;
	pass.submesh.type = PrimitiveType::Triangles;
	pass.material = m_material;
	pass.depth = Depth{ DepthCompare::LessOrEqual, true };
	pass.clear = Clear{ ClearMask::Depth | ClearMask::Color, color4f(0.f), 1.f, 1 };
	mat4f mvp = m_projection * m_arcball.view();
	m_uniform->upload(&mvp);
	pass.execute();*/
}

void MeshViewerEditor::draw(const String& name, Resource<Mesh>& resource)
{
	//ResourceManager* resources = Application::resource();
	//static const ImVec4 color = ImVec4(0.93f, 0.04f, 0.26f, 1.f);
	//ImGui::TextColored(color, name.cstr());
	//Mesh::Ptr mesh = resource.resource;
	//ImGui::Text("Vertices");

	//for (uint32_t i = 0; i < mesh->getVertexAttributeCount(); i++)
	//{
	//	char buffer[256];
	//	snprintf(buffer, 256, "Attribute %u", i);
	//	if (ImGui::TreeNode(buffer))
	//	{
	//		ImGui::BulletText("Format : %s", toString(mesh->getVertexAttribute(i).format));
	//		ImGui::BulletText("Semantic : %s", toString(mesh->getVertexAttribute(i).semantic));
	//		ImGui::BulletText("Type : %s", toString(mesh->getVertexAttribute(i).type));
	//		ImGui::BulletText("Count : %u", mesh->getVertexCount(i));
	//		ImGui::BulletText("Offset : %u", mesh->getVertexOffset(i));
	//		ImGui::BulletText("Buffer : %s", resources->name<Buffer>(mesh->getVertexBuffer(i).buffer).cstr());
	//		ImGui::TreePop();
	//	}
	//}
	//ImGui::Separator();
	//ImGui::Text("Indices");
	//ImGui::BulletText("Format : %s", toString(mesh->getIndexFormat()));
	//ImGui::BulletText("Count : %u", mesh->getIndexCount());
	//ImGui::BulletText("Buffer : %s", resources->name<Buffer>(mesh->getIndexBuffer().buffer).cstr());
	//ImGui::Separator();

	//// Mesh viewer
	//ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	//if (ImGui::BeginChild("MeshDisplayChild", ImVec2(0.f, (float)m_height + 5.f), false, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize))
	//{
	//	if (ImGui::IsWindowHovered() && (ImGui::IsMouseDragging(0, 0.0f) || ImGui::IsMouseDragging(1, 0.0f) || !ImGui::IsAnyItemActive()))
	//	{
	//		// TODO use real deltatime
	//		m_arcball.update(Time::milliseconds(10));
	//	}
	//	drawMesh(resource.resource);

	//	ImGui::Image((ImTextureID)(uintptr_t)m_renderTarget->handle().value(), ImVec2((float)m_width, (float)m_height), ImVec2(0, 0), ImVec2(1, 1), ImVec4(1, 1, 1, 1), ImVec4(1, 1, 1, 1));
	//}
	//ImGui::EndChild();
	//ImGui::PopStyleVar();
}

void TextureViewerEditor::draw(const String& name, Resource<Texture>& resource)
{
	//static const ImVec4 color = ImVec4(0.93f, 0.04f, 0.26f, 1.f);
	//ImGui::TextColored(color, name.cstr());
	//Texture::Ptr texture = resource.resource;
	//bool isRenderTarget = (texture->flags() & TextureFlag::RenderTarget) == TextureFlag::RenderTarget;
	//bool isShaderResource = (texture->flags() & TextureFlag::ShaderResource) == TextureFlag::ShaderResource;
	//bool hasMips = (texture->flags() & TextureFlag::GenerateMips) == TextureFlag::GenerateMips;
	//ImGui::Text("%s - %u x %u (%s)", toString(texture->type()), texture->width(), texture->height(), toString(texture->format()));
	//ImGui::Checkbox("Render target", &isRenderTarget); ImGui::SameLine();
	//ImGui::Checkbox("Shader resource", &isShaderResource); ImGui::SameLine();
	//ImGui::Checkbox("Mips", &hasMips);

	//// TODO add zoom and mip viewer
	//ImGui::Separator();
	//static bool red = true;
	//static bool green = true;
	//static bool blue = true;
	//static bool alpha = true;
	//static int zoomInt = 100;
	//static const int minZoom = 1;
	//static const int maxZoom = 500;
	//static ImVec2 scrollPosition = ImVec2(0.f, 0.f);
	//ImGui::Checkbox("R", &red); ImGui::SameLine();
	//ImGui::Checkbox("G", &green); ImGui::SameLine();
	//ImGui::Checkbox("B", &blue); ImGui::SameLine();
	//ImGui::Checkbox("A", &alpha); ImGui::SameLine();
	//ImGui::DragInt("##Zoom", &zoomInt, 1, minZoom, maxZoom, "%d %%");
	//ImTextureID textureID = (ImTextureID)(uintptr_t)texture->handle().value();
	//ImVec4 mask = ImVec4(
	//	red ? 1.f : 0.f,
	//	green ? 1.f : 0.f,
	//	blue ? 1.f : 0.f,
	//	alpha ? 1.f : 0.f
	//);
	//// Image explorer
	//ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	//if (ImGui::BeginChild("TextureDisplayChild", ImVec2(0, 0), true, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar))
	//{
	//	if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive())
	//	{
	//		zoomInt = min(max(zoomInt + (int)(ImGui::GetIO().MouseWheel * 2.f), minZoom), maxZoom);
	//	}
	//	float zoom = zoomInt / 100.f;
	//	if (ImGui::IsWindowHovered() && (ImGui::IsMouseDragging(0, 0.0f) || ImGui::IsMouseDragging(1, 0.0f)))
	//	{
	//		ImVec2 windowSize = ImGui::GetWindowSize();
	//		ImVec2 maxScroll(
	//			zoom * texture->width() - windowSize.x,
	//			zoom * texture->height() - windowSize.y
	//		);
	//		scrollPosition.x = fminf(fmaxf(scrollPosition.x - ImGui::GetIO().MouseDelta.x, 0.f), maxScroll.x);
	//		scrollPosition.y = fminf(fmaxf(scrollPosition.y - ImGui::GetIO().MouseDelta.y, 0.f), maxScroll.y);
	//	}
	//	ImGui::SetScrollX(scrollPosition.x);
	//	ImGui::SetScrollY(scrollPosition.y);
	//	ImGui::Image(textureID, ImVec2(zoom * texture->width(), zoom * texture->height()), ImVec2(0, 0), ImVec2(1, 1), mask);
	//}
	//ImGui::EndChild();
	//ImGui::PopStyleVar();
}

void FontViewerEditor::draw(const String& name, Resource<Font>& resource)
{
//	static const ImVec4 color = ImVec4(0.93f, 0.04f, 0.26f, 1.f);
//	ImGui::TextColored(color, name.cstr());
//	Font::Ptr font = resource.resource;
//
//	ImGui::Text("Family : %s", font->family().cstr());
//	ImGui::Text("Style : %s", font->style().cstr());
//	ImGui::Text("Count : %zu", font->count());
//	int height = font->height();
//	if (ImGui::SliderInt("Height", &height, 1, 50) && height != font->height())
//	{
//		// Resize font
//		// TODO This will invalid previous font.
//		FontStorage storage;
//		if (storage.load(resource.path))
//		{
//			resource.resource = Font::create(storage.ttf.data(), storage.ttf.size(), height);
//		}
//	}
//	// Display atlas
//	Texture::Ptr atlas = font->atlas();
//	float uvx = 1.f / (atlas->height() / font->height());
//	float uvy = 1.f / (atlas->width() / font->height());
//
//	uint32_t lineCount = 0;
//	for (uint32_t i = 0; i < (uint32_t)font->count(); i++)
//	{
//		const Character& character = font->getCharacter(i);
//		ImGui::Image(
//			(ImTextureID)character.texture.texture->handle().value(),
//			ImVec2(30, 30),
//#if defined(ORIGIN_BOTTOM_LEFT)
//			ImVec2(character.texture.get(0).u, character.texture.get(0).v + uvy),
//			ImVec2(character.texture.get(0).u + uvx, character.texture.get(0).v),
//#else
//			ImVec2(character.texture.get(0).u, character.texture.get(0).v),
//			ImVec2(character.texture.get(0).u + uvx, character.texture.get(0).v + uvy),
//#endif
//			ImVec4(1, 1, 1, 1),
//			ImVec4(1, 1, 1, 1)
//		);
//		if (ImGui::IsItemHovered())
//		{
//			ImGui::BeginTooltip();
//			ImGui::Text("Advance : %u", character.advance);
//			ImGui::Text("Size : (%d, %d)", character.size.x, character.size.y);
//			ImGui::Text("Bearing : (%d, %d)", character.bearing.x, character.bearing.y);
//			ImVec2 size = ImVec2(300, 300);
//			ImGui::Image(
//				(ImTextureID)character.texture.texture->handle().value(),
//				size,
//				ImVec2(0, 0),
//				ImVec2(1, 1),
//				ImVec4(1, 1, 1, 1),
//				ImVec4(1, 1, 1, 1)
//			);
//			uv2f start = character.texture.get(0);
//			uv2f end = character.texture.get(1);
//			ImVec2 startVec = ImVec2(ImGui::GetItemRectMin().x + start.u * size.x, ImGui::GetItemRectMin().y + start.v * size.y);
//			ImVec2 endVec = ImVec2(ImGui::GetItemRectMin().x + end.u * size.x + 1, ImGui::GetItemRectMin().y + end.v * size.y + 1);
//			ImU32 red = (93 << 24) | (4 << 16) | (26 << 8) | (255 << 0);
//			ImGui::GetWindowDrawList()->AddRect(startVec, endVec, ImU32(red), 0.f, ImDrawCornerFlags_All, 2.f);
//			ImGui::EndTooltip();
//		}
//		if (lineCount++ < 10)
//			ImGui::SameLine();
//		else
//			lineCount = 0;
//	}
}

FontViewerEditor::FontViewerEditor() :
	AssetViewerEditor("Font")
{
}

MeshViewerEditor::MeshViewerEditor() :
	AssetViewerEditor("Mesh")
{
}

TextureViewerEditor::TextureViewerEditor() :
	AssetViewerEditor("Texture")
{
}

AudioViewerEditor::AudioViewerEditor() :
	AssetViewerEditor("Audio")
{
}

BufferViewerEditor::BufferViewerEditor() :
	AssetViewerEditor("Buffer")
{
}

void AudioViewerEditor::draw(const String& name, Resource<AudioStream>& resource)
{
	ImGui::Text(name.cstr());
	// TODO
}

};
