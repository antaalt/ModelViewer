#include "SceneEditor.h"

#include <imgui.h>
#include <imguizmo.h>

#include "../Model/Model.h"
#include "AssetViewerEditor.h"

#include <Aka/Aka.h>

namespace app {

void TextureDisplay(const String& name, Texture::Ptr texture, const ImVec2& size)
{
	// TODO open a window on click for a complete texture inspector ?
	if (texture == nullptr)
	{
		// TODO add texture loading here ?
		ImGui::Text("Missing texture : %s.", name.cstr());
	}
	else
	{
		ImTextureID textureID = (ImTextureID)(uintptr_t)texture->handle();
		ImGui::Text("%s", name.cstr());
		ImGui::Image(textureID, size);
	}
}
void TextureSamplerDisplay(TextureSampler& sampler)
{
	static const char* filters[] = {
		"Nearest",
		"Linear"
	};
	static const char* mipmaps[] = {
		"None",
		"Nearest",
		"Linear"
	};
	static const char* wraps[] = {
		"Repeat",
		"Mirror",
		"ClampToEdge",
		"ClampToBorder",
	};
	char buffer[256];
	int error = snprintf(buffer, 256, "Sampler##%p", &sampler);
	if (ImGui::TreeNode(buffer))
	{
		// Filters
		int current = (int)sampler.filterMin;
		if (ImGui::Combo("Filter min", &current, filters, 2))
			sampler.filterMin = (TextureFilter)current;
		current = (int)sampler.filterMag;
		if (ImGui::Combo("Filter mag", &current, filters, 2))
			sampler.filterMag = (TextureFilter)current;
		// Mips
		current = (int)sampler.mipmapMode;
		if (ImGui::Combo("Mips", &current, mipmaps, 3))
			sampler.mipmapMode = (TextureMipMapMode)current;
		// Wraps
		current = (int)sampler.wrapU;
		if (ImGui::Combo("WrapU", &current, wraps, 4))
			sampler.wrapU = (TextureWrap)current;
		current = (int)sampler.wrapV;
		if (ImGui::Combo("WrapV", &current, wraps, 4))
			sampler.wrapV = (TextureWrap)current;
		current = (int)sampler.wrapW;
		if (ImGui::Combo("WrapW", &current, wraps, 4))
			sampler.wrapW = (TextureWrap)current;
		// Anisotropy
		ImGui::SliderFloat("Anisotropy", &sampler.anisotropy, 1.f, 16.f);

		ImGui::TreePop();
	}
}

template <typename T>
struct ComponentNode {
	static const char* name() { return "Unknown"; }
	//static const char* icon() { return ""; }
	static bool draw(T& component) { Logger::error("Trying to draw an undefined component"); return false; }
};

template <> const char* ComponentNode<TagComponent>::name() { return "Tag"; }
template <> bool ComponentNode<TagComponent>::draw(TagComponent& tag)
{
	char buffer[256];
	String::copy(buffer, 256, tag.name.cstr());
	if (ImGui::InputText("Name", buffer, 256))
	{
		tag.name = buffer;
		return true;
	}
	return false;
}

template <> const char* ComponentNode<Camera3DComponent>::name() { return "Camera"; }
template <> bool ComponentNode<Camera3DComponent>::draw(Camera3DComponent& camera) 
{ 
	bool updated = false;
	updated |= ImGui::Checkbox("Active", &camera.active);
	// --- Projection
	const char* projectionType[] = {
		"Perspective",
		"Orthographic"
	};
	int currentProjection = (int)camera.projection->type();
	if (ImGui::Combo("Projection", &currentProjection, projectionType, 2))
	{
		if ((CameraProjectionType)currentProjection != camera.projection->type())
		{
			switch ((CameraProjectionType)currentProjection)
			{
			case CameraProjectionType::Orthographic:
				camera.projection = std::make_unique<CameraOrthographic>();
				break;
			case CameraProjectionType::Perpective:
				camera.projection = std::make_unique<CameraPerspective>();
				break;
			}
		}
	}
	switch ((CameraProjectionType)currentProjection)
	{
	case CameraProjectionType::Orthographic: {
		CameraOrthographic* o = reinterpret_cast<CameraOrthographic*>(camera.projection.get());
		updated |= ImGui::SliderFloat("Left", &o->left, -1.f, 1.f);
		updated |= ImGui::SliderFloat("Right", &o->right, -1.f, 1.f);
		updated |= ImGui::SliderFloat("Bottom", &o->bottom, -1.f, 1.f);
		updated |= ImGui::SliderFloat("Top", &o->top, -1.f, 1.f);
		updated |= ImGui::SliderFloat("Near", &o->nearZ, 0.001f, 10.f);
		updated |= ImGui::SliderFloat("Far", &o->farZ, 10.f, 1000.f);
		break;
	}
	case CameraProjectionType::Perpective: {
		CameraPerspective* p = reinterpret_cast<CameraPerspective*>(camera.projection.get());
		float fov = p->hFov.radian();
		if (ImGui::SliderAngle("Fov", &fov, 10.f, 160.f))
		{
			p->hFov = anglef::radian(fov);
			updated = true;
		}
		updated |= ImGui::SliderFloat("Near", &p->nearZ, 0.001f, 10.f);
		updated |= ImGui::SliderFloat("Far", &p->farZ, 10.f, 1000.f);
		break;
	}
	default:
		ImGui::Text("Invalid camera type.");
		break;
	}
	
	// --- Controller
	const char* controllerTypes[] = {
		"Arcball"
	};
	int currentController = (int)camera.controller->type();
	if (ImGui::Combo("Controller", &currentController, controllerTypes, 1))
	{
		if ((CameraControllerType)currentController != camera.controller->type())
		{
			switch ((CameraControllerType)currentController)
			{
			case CameraControllerType::Arcball:
				camera.controller = std::make_unique<CameraArcball>();
				break;
			}
		}
	}
	switch ((CameraControllerType)currentController)
	{
	case CameraControllerType::Arcball:
		CameraArcball* a = reinterpret_cast<CameraArcball*>(camera.controller.get());
		updated |= ImGui::InputFloat3("Position", a->position.data);
		updated |= ImGui::InputFloat3("Target", a->target.data);
		updated |= ImGui::InputFloat3("Up", a->up.data);
		updated |= ImGui::SliderFloat("Speed", &a->speed, 0.1f, 100.f);
		break;
	}
	return updated;
}

template <> const char* ComponentNode<Hierarchy3DComponent>::name() { return "Hierarchy"; }
template <> bool ComponentNode<Hierarchy3DComponent>::draw(Hierarchy3DComponent& hierarchy)
{
	if (hierarchy.parent.handle() == entt::null || !hierarchy.parent.valid())
	{
		ImGui::Text("Parent : None");
	}
	else
	{
		if (hierarchy.parent.has<TagComponent>())
			ImGui::Text("Parent : %s", hierarchy.parent.get<TagComponent>().name.cstr());
		else
			ImGui::Text("Parent : Unknown");
	}
	return false; 
}

template <> const char* ComponentNode<Transform3DComponent>::name() { return "Transform"; }
template <> bool ComponentNode<Transform3DComponent>::draw(Transform3DComponent& transform) 
{
	bool updated = false;
	float translation[3];
	float rotation[3];
	float scale[3];
	ImGuizmo::DecomposeMatrixToComponents(transform.transform.cols[0].data, translation, rotation, scale);
	updated |= ImGui::InputFloat3("Translation", translation);
	updated |= ImGui::InputFloat3("Rotation", rotation);
	updated |= ImGui::InputFloat3("Scale", scale);
	if (updated)
		ImGuizmo::RecomposeMatrixFromComponents(translation, rotation, scale, transform.transform.cols[0].data);
	return updated; 
}

template <> const char* ComponentNode<PointLightComponent>::name() { return "Point light"; }
template <> bool ComponentNode<PointLightComponent>::draw(PointLightComponent& light)
{
	bool updated = false;
	updated |= ImGui::ColorEdit3("Color", light.color.data);
	updated |= ImGui::SliderFloat("Intensity", &light.intensity, 0.1f, 100.f);
	return updated;
}

template <> const char* ComponentNode<DirectionalLightComponent>::name() { return "Directional light"; }
template <> bool ComponentNode<DirectionalLightComponent>::draw(DirectionalLightComponent& light)
{
	bool updated = false;
	if (ImGui::InputFloat3("Direction", light.direction.data))
	{
		updated = true;
		light.direction = vec3f::normalize(light.direction);
	}
	updated |= ImGui::ColorEdit3("Color", light.color.data);
	updated |= ImGui::SliderFloat("Intensity", &light.intensity, 0.1f, 100.f);
	TextureDisplay("CSM 0", light.shadowMap[0], ImVec2(100, 100));
	TextureDisplay("CSM 1", light.shadowMap[1], ImVec2(100, 100));
	TextureDisplay("CSM 2", light.shadowMap[2], ImVec2(100, 100));
	return updated;
}

template <> const char* ComponentNode<MeshComponent>::name() { return "Mesh"; }
template <> bool ComponentNode<MeshComponent>::draw(MeshComponent& mesh) 
{ 
	if (mesh.submesh.mesh != nullptr)
	{
		uint32_t sizeOfVertex = 0;
		for (uint32_t i = 0; i < mesh.submesh.mesh->getVertexAttributeCount(); i++)
			sizeOfVertex += mesh.submesh.mesh->getVertexAttribute(i).size();
		ImGui::Text("Vertices : %u", mesh.submesh.mesh->getVertexBuffer(0).size / sizeOfVertex);
		ImGui::Text("Index count : %u", mesh.submesh.count);
		ImGui::Text("Index offset : %u", mesh.submesh.offset);
		String type = "Undefined";
		switch (mesh.submesh.type)
		{
		case PrimitiveType::Lines:
			type = "Lines";
			break;
		case PrimitiveType::Triangles:
			type = "Triangles";
			break;
		case PrimitiveType::Points:
			type = "Points";
			break;
		}
		ImGui::Text("Primitive : %s", type.cstr());
		ImGui::Text("Bounds min : (%f, %f, %f)", mesh.bounds.min.x, mesh.bounds.min.y, mesh.bounds.min.z);
		ImGui::Text("Bounds max : (%f, %f, %f)", mesh.bounds.max.x, mesh.bounds.max.y, mesh.bounds.max.z);
	}
	else
	{
		ImGui::Text("No mesh data");
	}
	return false;
}

template <> const char* ComponentNode<MaterialComponent>::name() { return "Material"; }
template <> bool ComponentNode<MaterialComponent>::draw(MaterialComponent& material) 
{
	bool updated = false;
	updated |= ImGui::ColorEdit4("Color", material.color.data);
	updated |= ImGui::Checkbox("Double sided", &material.doubleSided);
	TextureDisplay("Color", material.albedo.texture, ImVec2(100, 100));
	TextureSamplerDisplay(material.albedo.sampler);
	TextureDisplay("Normal", material.normal.texture, ImVec2(100, 100));
	TextureSamplerDisplay(material.normal.sampler);
	TextureDisplay("Material", material.material.texture, ImVec2(100, 100));
	TextureSamplerDisplay(material.material.sampler);
	return updated; 
}

template <> const char* ComponentNode<TextComponent>::name() { return "Text"; }
template <> bool ComponentNode<TextComponent>::draw(TextComponent& text)
{
	bool updated = false;
	ResourceManager* resources = Application::resource();
	String name = resources->name<Font>(text.font);
	if (ImGui::BeginCombo("Font", name.cstr()))
	{
		for (auto& font : resources->allocator<Font>())
		{
			bool sameFont = (text.font == font.second.resource);
			if (ImGui::Selectable(font.first.cstr(), sameFont) && !sameFont)
				text.font = font.second.resource;
			if (sameFont)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	updated |= ImGui::ColorEdit4("Color", text.color.data);
	const size_t bufferSize = 512;
	char buffer[bufferSize];
	String::copy(buffer, bufferSize, text.text.cstr());
	if (ImGui::InputText("Text", buffer, bufferSize))
	{
		updated = true;
		text.text = buffer;
	}
	TextureSamplerDisplay(text.sampler);
	return updated;
}


template <typename T>
void component(World& world, entt::entity entity)
{
	static char buffer[256];
	if (world.registry().has<T>(entity))
	{
		T& component = world.registry().get<T>(entity);
		snprintf(buffer, 256, "%s##%p", ComponentNode<T>::name(), &component);
		if (ImGui::TreeNodeEx(buffer, ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_DefaultOpen))
		{
			snprintf(buffer, 256, "ClosePopUp##%p", &component);
			if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1))
				ImGui::OpenPopup(buffer);
			if (ComponentNode<T>::draw(component))
			{
				world.registry().patch<T>(entity);
			}
			if (ImGui::BeginPopupContextItem(buffer))
			{
				if (ImGui::MenuItem("Remove"))
					world.registry().remove<T>(entity);
				ImGui::EndPopup();
			}
			ImGui::TreePop();
		}
	}
}

SceneEditor::SceneEditor() :
	m_currentEntity(entt::null),
	m_gizmoOperation(ImGuizmo::TRANSLATE),
	m_entityName("")
{
}

void SceneEditor::onCreate(World& world)
{
	std::vector<VertexAttribute> att{
		VertexAttribute{ VertexSemantic::Position, VertexFormat::Float, VertexType::Vec3 },
		VertexAttribute{ VertexSemantic::Normal, VertexFormat::Float, VertexType::Vec3 },
		VertexAttribute{ VertexSemantic::TexCoord0, VertexFormat::Float, VertexType::Vec2 },
		VertexAttribute{ VertexSemantic::Color0, VertexFormat::Float, VertexType::Vec4 }
	};
	ProgramManager* program = Application::program();
	m_wireframeProgram = program->get("editor.wireframe");
	m_wireframeMaterial = Material::create(m_wireframeProgram);
	m_wireFrameUniformBuffer = Buffer::create(BufferType::Uniform, sizeof(mat4f), BufferUsage::Default, BufferCPUAccess::None);
	m_wireframeMaterial->set("ModelUniformBuffer", m_wireFrameUniformBuffer);
}

void SceneEditor::onDestroy(World& world)
{
}

bool intersectBounds(const aabbox<>& bounds, const point3f& origin, const vec3f& direction, float& tmin, float& tmax)
{
	tmin = (bounds.min.x - origin.x) / direction.x;
	tmax = (bounds.max.x - origin.x) / direction.x;
	if (tmin > tmax) std::swap(tmin, tmax);
	float tymin = (bounds.min.y - origin.y) / direction.y;
	float tymax = (bounds.max.y - origin.y) / direction.y;
	if (tymin > tymax) std::swap(tymin, tymax);
	if ((tmin > tymax) || (tymin > tmax)) return false;
	if (tymin > tmin) tmin = tymin;
	if (tymax < tmax) tmax = tymax;
	float tzmin = (bounds.min.z - origin.z) / direction.z;
	float tzmax = (bounds.max.z - origin.z) / direction.z;
	if (tzmin > tzmax) std::swap(tzmin, tzmax);
	if ((tmin > tzmax) || (tzmin > tmax)) return false;
	if (tzmin > tmin) tmin = tzmin;
	if (tzmax < tmax) tmax = tzmax;
	return true;
};

entt::entity pick(World& world)
{
	// Find a valid camera
	Entity cameraEntity = Scene::getMainCamera(world);
	if (!cameraEntity.valid())
		return entt::null;

	// Get projection & view
	Camera3DComponent& camera = cameraEntity.get<Camera3DComponent>();
	mat4f view = camera.view;
	mat4f projection = camera.projection->projection();

	ImGuiIO& io = ImGui::GetIO();
	// Generate pick ray in world space
	ImVec2 pos = ImGui::GetMousePos(); // pixel
	mat4f inverseView = mat4f::inverse(view);
	point3f origin = point3f(inverseView[3]);
	vec3f screenDirection = vec3f(
		(2.f * pos.x) / (float)io.DisplaySize.x -1.f,
		1.f - (2.f * pos.y) / (float)io.DisplaySize.y,
		-1.f // TODO opengl forward, flip for d3d ?
	);
	vec4f cameraDirection = mat4f::inverse(projection) * vec4f(screenDirection, 1.f);
	vec3f worldDirection = vec3f::normalize(inverseView.multiplyVector(vec3f(cameraDirection.x, cameraDirection.y, -1.f)));
	float tminWorld = std::numeric_limits<float>::max();
	float tmaxWorld = std::numeric_limits<float>::max();
	auto renderableView = world.registry().view<Transform3DComponent, MeshComponent, MaterialComponent>();
	entt::entity selected = entt::null;
	for (entt::entity e : renderableView)
	{
		Transform3DComponent& t = world.registry().get<Transform3DComponent>(e);
		MeshComponent& m = world.registry().get<MeshComponent>(e);
		mat4f inverseTransform = mat4f::inverse(t.transform);
		point3f localOrigin = inverseTransform.multiplyPoint(origin);
		vec3f localDirection = inverseTransform.multiplyVector(worldDirection);
		float tmin, tmax;
		if (intersectBounds(m.bounds, localOrigin, localDirection, tmin, tmax))
		{
			if (tminWorld > tmin)
			{
				// if we are closer and new one is not around current one
				if (tmaxWorld >= tmax)
				{
					tminWorld = tmin;
					tmaxWorld = tmax;
					selected = e;
				}
			}
		}
	}
	return selected;
}

void SceneEditor::onUpdate(World& world, Time deltaTime)
{
	ImGuiIO& io = ImGui::GetIO();
	ImVec2 v = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
	float l = sqrt(v.x * v.x + v.y * v.y);
	float threshold = 1.f;
	if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && l < threshold && !io.WantCaptureMouse)
		m_currentEntity = pick(world);
}

void recurse(World& world, entt::entity entity, const std::map<entt::entity, std::vector<entt::entity>>& childrens, entt::entity& current)
{
	char buffer[256];
	const TagComponent& tag = world.registry().get<TagComponent>(entity);

	auto it = childrens.find(entity);
	if (it != childrens.end())
	{
		int err = snprintf(buffer, 256, "%s##%p", tag.name.cstr(), &tag);
		ImGuiTreeNodeFlags flags = 0;
		if (entity == current)
			flags |= ImGuiTreeNodeFlags_Selected;
		if (ImGui::TreeNodeEx(buffer, flags))
		{
			err = snprintf(buffer, 256, "ClosePopUp##%p", &tag);
			if (ImGui::IsItemClicked())
				current = entity;
			if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(1))
				ImGui::OpenPopup(buffer);
			if (ImGui::BeginPopupContextItem(buffer))
			{
				if (ImGui::MenuItem("Delete"))
					world.registry().destroy(entity);
				ImGui::EndPopup();
			}
			for (entt::entity e : it->second)
				recurse(world, e, childrens, current);
			ImGui::TreePop();
		}
	}
	else
	{
		int err = snprintf(buffer, 256, "ClosePopUp##%p", &tag);
		ImGui::Bullet();
		bool isSelected = current == entity;
		if (ImGui::Selectable(tag.name.cstr(), &isSelected))
			current = entity;
		if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(1))
			ImGui::OpenPopup(buffer);
		if (ImGui::BeginPopupContextItem(buffer))
		{
			if (ImGui::MenuItem("Delete"))
				world.registry().destroy(entity);
			ImGui::EndPopup();
		}
	}
}

void SceneEditor::drawWireFrame(const mat4f& model, const mat4f& view, const mat4f& projection, const SubMesh& submesh)
{
	GraphicDevice* device = Application::graphic();
	RenderPass r;
	r.framebuffer = device->backbuffer();
	r.material = m_wireframeMaterial;
	r.clear = Clear::none;
	r.blend = Blending::none;
	r.depth = Depth{ DepthCompare::LessOrEqual, false };
	r.cull = Culling{ CullMode::BackFace, CullOrder::CounterClockWise };
	r.stencil = Stencil::none;
	r.viewport = aka::Rect{ 0 };
	r.scissor = aka::Rect{ 0 };
	r.submesh = submesh;
	if (r.submesh.type == PrimitiveType::Triangles)
	{
		r.submesh.type = PrimitiveType::LineStrip;
		mat4f mvp = projection * view * model;
		m_wireFrameUniformBuffer->upload(&mvp);
		r.execute();
	}
}

void SceneEditor::onRender(World& world)
{
	// TODO draw a grid here and origin of the world
	ResourceManager* resources = Application::resource();

	ImGuizmo::BeginFrame();
	if (ImGui::Begin("Scene", nullptr, ImGuiWindowFlags_MenuBar))
	{
		static const ImVec4 color = ImVec4(0.93f, 0.04f, 0.26f, 1.f);
		// --- Menu
		Entity e = Entity(m_currentEntity, &world);
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("World"))
			{
				if (ImGui::MenuItem("Save"))
				{
					Scene::save("library/scene.json", world);
				}
				if (ImGui::MenuItem("Load"))
				{
					Scene::load(world, "library/scene.json");
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Entity"))
			{
				if (ImGui::BeginMenu("Create"))
				{
					mat4f id = mat4f::identity();
					if (ImGui::BeginMenu("Mesh"))
					{
						if (ImGui::MenuItem("Cube"))
						{
							m_currentEntity = Scene::createCubeEntity(world).handle();
						}
						if (ImGui::MenuItem("UV Sphere"))
						{
							m_currentEntity = Scene::createSphereEntity(world, 32, 16).handle();
						}
						ImGui::EndMenu();
					}
					if (ImGui::BeginMenu("Light"))
					{
						if (ImGui::MenuItem("Point light"))
						{
							m_currentEntity = Scene::createPointLightEntity(world).handle();
						}
						if (ImGui::MenuItem("Directional light"))
						{
							m_currentEntity = Scene::createDirectionalLightEntity(world).handle();
						}
						ImGui::EndMenu();
					}
					
					if (ImGui::MenuItem("Camera"))
					{
						m_currentEntity = Scene::createArcballCameraEntity(world).handle();
					}
					if (ImGui::MenuItem("Empty"))
					{
						m_currentEntity = world.createEntity("New empty").handle();
					}
					ImGui::EndMenu();
				}
				if (ImGui::MenuItem("Destroy", nullptr, nullptr, e.valid()))
					e.destroy();
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Component", e.valid()))
			{
				if (ImGui::BeginMenu("Add", e.valid()))
				{
					if (ImGui::MenuItem("Transform", nullptr, nullptr, !e.has<Transform3DComponent>()))
						e.add<Transform3DComponent>(Transform3DComponent{ mat4f::identity() });
					if (ImGui::MenuItem("Hierarchy", nullptr, nullptr, !e.has<Hierarchy3DComponent>()))
						e.add<Hierarchy3DComponent>(Hierarchy3DComponent{ Entity::null(), mat4f::identity() });
					if (ImGui::MenuItem("Camera", nullptr, nullptr, !e.has<Camera3DComponent>()))
					{
						auto p = std::make_unique<CameraPerspective>();
						p->hFov = anglef::degree(60.f);
						p->nearZ = 0.01f;
						p->farZ = 100.f;
						p->ratio = 1.f;
						auto c = std::make_unique<CameraArcball>();
						c->set(aabbox<>(point3f(0.f), point3f(1.f)));
						e.add<Camera3DComponent>(Camera3DComponent{
							mat4f::identity(),
							std::move(p),
							std::move(c)
						});
					}
					if (ImGui::MenuItem("Mesh", nullptr, nullptr, !e.has<MeshComponent>()))
						e.add<MeshComponent>(MeshComponent{});
					if (ImGui::MenuItem("Material", nullptr, nullptr, !e.has<MaterialComponent>()))
						e.add<MaterialComponent>(MaterialComponent{ color4f(1.f), false, { nullptr, TextureSampler::nearest}, { nullptr, TextureSampler::nearest}, { nullptr, TextureSampler::nearest} });
					if (ImGui::MenuItem("Point light", nullptr, nullptr, !e.has<PointLightComponent>()))
						e.add<PointLightComponent>(PointLightComponent{
							color3f(1.f), 1.f, {},
							TextureCubeMap::create(1024, 1024, TextureFormat::Depth, TextureFlag::RenderTarget)
						});
					if (ImGui::MenuItem("Directional light", nullptr, nullptr, !e.has<DirectionalLightComponent>()))
						e.add<DirectionalLightComponent>(DirectionalLightComponent{
							vec3f(0,1,0),
							color3f(1.f), 1.f, {}, {
								Texture2D::create(1024, 1024, TextureFormat::Depth, TextureFlag::RenderTarget),
								Texture2D::create(1024, 1024, TextureFormat::Depth, TextureFlag::RenderTarget),
								Texture2D::create(2048, 2048, TextureFormat::Depth, TextureFlag::RenderTarget)
							}, {} }
						);
					if (ImGui::BeginMenu("Text", !e.has<TextComponent>()))
					{
						FontAllocator& allocator = resources->allocator<Font>();
						for (auto& r : allocator)
						{
							if (ImGui::MenuItem(r.first.cstr(), nullptr, nullptr, !e.has<TextComponent>()))
								e.add<TextComponent>(TextComponent{ r.second.resource, TextureSampler::nearest, "", color4f(1.f) });
						}
						ImGui::EndMenu();
					}
					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu("Remove", e.valid()))
				{
					if (ImGui::MenuItem("Transform", nullptr, nullptr, e.has<Transform3DComponent>()))
						e.remove<Transform3DComponent>();
					if (ImGui::MenuItem("Hierarchy", nullptr, nullptr, e.has<Hierarchy3DComponent>()))
						e.remove<Hierarchy3DComponent>();
					if (ImGui::MenuItem("Camera", nullptr, nullptr, e.has<Camera3DComponent>()))
						e.remove<Camera3DComponent>();
					if (ImGui::MenuItem("Mesh", nullptr, nullptr, e.has<MeshComponent>()))
						e.remove<MeshComponent>();
					if (ImGui::MenuItem("Material", nullptr, nullptr, e.has<MaterialComponent>()))
						e.remove<MaterialComponent>();
					if (ImGui::MenuItem("Point light", nullptr, nullptr, e.has<PointLightComponent>()))
						e.remove<PointLightComponent>();
					if (ImGui::MenuItem("Directional light", nullptr, nullptr, e.has<DirectionalLightComponent>()))
						e.remove<DirectionalLightComponent>();
					if (ImGui::MenuItem("Text", nullptr, nullptr, e.has<TextComponent>()))
						e.remove<TextComponent>();
					ImGui::EndMenu();
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Transform operation"))
			{
				bool enabled = m_gizmoOperation == ImGuizmo::TRANSLATE;
				if (ImGui::MenuItem("Translate", nullptr, &enabled)) 
					m_gizmoOperation = ImGuizmo::TRANSLATE;
				enabled = m_gizmoOperation == ImGuizmo::ROTATE;
				if (ImGui::MenuItem("Rotate", nullptr, &enabled))
					m_gizmoOperation = ImGuizmo::ROTATE;
				enabled = m_gizmoOperation == ImGuizmo::SCALE;
				if (ImGui::MenuItem("Scale", nullptr, &enabled))
					m_gizmoOperation = ImGuizmo::SCALE;
				ImGui::Separator();
				enabled = m_gizmoOperation == ImGuizmo::TRANSLATE_X;
				if (ImGui::MenuItem("TranslateX", nullptr, &enabled))
					m_gizmoOperation = ImGuizmo::TRANSLATE_X;
				enabled = m_gizmoOperation == ImGuizmo::TRANSLATE_Y;
				if (ImGui::MenuItem("TranslateY", nullptr, &enabled))
					m_gizmoOperation = ImGuizmo::TRANSLATE_Y;
				enabled = m_gizmoOperation == ImGuizmo::TRANSLATE_Z;
				if (ImGui::MenuItem("TranslateZ", nullptr, &enabled))
					m_gizmoOperation = ImGuizmo::TRANSLATE_Z;
				ImGui::Separator();
				enabled = m_gizmoOperation == ImGuizmo::ROTATE_X;
				if (ImGui::MenuItem("RotateX", nullptr, &enabled))
					m_gizmoOperation = ImGuizmo::ROTATE_X;
				enabled = m_gizmoOperation == ImGuizmo::ROTATE_Y;
				if (ImGui::MenuItem("RotateY", nullptr, &enabled))
					m_gizmoOperation = ImGuizmo::ROTATE_Y;
				enabled = m_gizmoOperation == ImGuizmo::ROTATE_Z;
				if (ImGui::MenuItem("RotateZ", nullptr, &enabled))
					m_gizmoOperation = ImGuizmo::ROTATE_Z;
				ImGui::Separator();
				enabled = m_gizmoOperation == ImGuizmo::SCALE_X;
				if (ImGui::MenuItem("ScaleX", nullptr, &enabled))
					m_gizmoOperation = ImGuizmo::SCALE_X;
				enabled = m_gizmoOperation == ImGuizmo::SCALE_Y;
				if (ImGui::MenuItem("ScaleY", nullptr, &enabled))
					m_gizmoOperation = ImGuizmo::SCALE_Y;
				enabled = m_gizmoOperation == ImGuizmo::SCALE_Z;
				if (ImGui::MenuItem("ScaleZ", nullptr, &enabled))
					m_gizmoOperation = ImGuizmo::SCALE_Z;
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}
		// --- Graph
		// TODO do not compute child map every cycle
		// listen to event ?
		std::map<entt::entity, std::vector<entt::entity>> childrens;
		std::vector<entt::entity> roots;
		world.registry().each([&](entt::entity entity) {
			if (world.registry().has<Hierarchy3DComponent>(entity))
			{
				const Hierarchy3DComponent& h = world.registry().get<Hierarchy3DComponent>(entity);
				if (!h.parent.valid())
					roots.push_back(entity);
				else
					childrens[h.parent.handle()].push_back(entity);
			}
			else
				roots.push_back(entity);
		});
		ImGui::TextColored(color, "Graph");
		if (ImGui::BeginChild("##list", ImVec2(0, 200), true))
		{
			for (entt::entity e : roots)
				recurse(world, e, childrens, m_currentEntity);
		}
		ImGui::EndChild();

		// --- Add entity
		ImGui::InputTextWithHint("##entityName", "Entity name", m_entityName, 256);
		ImGui::SameLine();
		if (ImGui::Button("Create entity"))
		{
			m_currentEntity = world.createEntity(m_entityName).handle();
		}
		ImGui::Separator();

		// --- Entity info
		ImGui::TextColored(color, "Entity");
		if (m_currentEntity != entt::null && world.registry().valid(m_currentEntity))
		{
			if (world.registry().orphan(m_currentEntity))
			{
				ImGui::Text("Add a component to the entity.");
			}
			else
			{
				// Draw every component.
				component<TagComponent>(world, m_currentEntity);
				component<Transform3DComponent>(world, m_currentEntity);
				component<Hierarchy3DComponent>(world, m_currentEntity);
				component<MeshComponent>(world, m_currentEntity);
				component<MaterialComponent>(world, m_currentEntity);
				component<DirectionalLightComponent>(world, m_currentEntity);
				component<PointLightComponent>(world, m_currentEntity);
				component<Camera3DComponent>(world, m_currentEntity);
				component<TextComponent>(world, m_currentEntity);
			}
			// --- Gizmo
			Entity cameraEntity = Scene::getMainCamera(world);
			if (cameraEntity.valid() && world.registry().has<Transform3DComponent>(m_currentEntity))
			{
				// Get camera data for rendering on screen
				Camera3DComponent& camera = cameraEntity.get<Camera3DComponent>();
				mat4f view = camera.view;
				mat4f projection = camera.projection->projection();

				// Draw gizmo axis
				Transform3DComponent& transform = world.registry().get<Transform3DComponent>(m_currentEntity);
				ImGuiIO& io = ImGui::GetIO();
				ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
				if (ImGuizmo::Manipulate(view[0].data, projection[0].data, (ImGuizmo::OPERATION)m_gizmoOperation, ImGuizmo::MODE::WORLD, transform.transform[0].data))
					world.registry().patch<Transform3DComponent>(m_currentEntity);
				
				// Draw debug views
				if (world.registry().has<MeshComponent>(m_currentEntity))
				{
					drawWireFrame(transform.transform, view, projection, world.registry().get<MeshComponent>(m_currentEntity).submesh);
				}
				if (world.registry().has<Camera3DComponent>(m_currentEntity))
				{
					// Draw a camera ?
				}

				if (world.registry().has<PointLightComponent>(m_currentEntity))
				{
					const PointLightComponent& l = world.registry().get<PointLightComponent>(m_currentEntity);
					mat4f model = transform.transform * mat4f::scale(vec3f(l.radius));
					// TODO render a sphere instead.
					Renderer3D::drawTransform(model);
				}
				if (world.registry().has<DirectionalLightComponent>(m_currentEntity))
				{
					// TODO draw an arrow.
					const DirectionalLightComponent& l = world.registry().get<DirectionalLightComponent>(m_currentEntity);
					Renderer3D::Line line{};
					line.vertices[0].position = point3f(transform.transform[3]);
					line.vertices[0].color = color4f(1, 1, 1, 1);
					line.vertices[1].position = point3f(transform.transform[3]) + l.direction * 3.f;
					line.vertices[1].color = color4f(1, 0, 0, 1);
					Renderer3D::draw(mat4f::identity(), line);
				}

				// Render
				GraphicDevice* device = Application::graphic();
				Renderer3D::render(device->backbuffer(), view, projection);
				Renderer3D::clear();
			}
		
		}
	}
	ImGui::End();
}

};