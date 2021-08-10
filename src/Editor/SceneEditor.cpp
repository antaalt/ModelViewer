#include "SceneEditor.h"

#include <imgui.h>
#include <imguizmo.h>

#include "../Model/Model.h"

#include <Aka/Aka.h>

namespace viewer {

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
	CameraPerspective* p = dynamic_cast<CameraPerspective*>(camera.projection);
	if (p != nullptr)
	{
		float fov = p->hFov.radian();
		if (ImGui::SliderAngle("Fov", &fov, 10.f, 160.f))
		{
			p->hFov = anglef::radian(fov);
			updated = true;
		}
		updated |= ImGui::SliderFloat("Near", &p->nearZ, 0.001f, 10.f);
		updated |= ImGui::SliderFloat("Far", &p->farZ, 10.f, 1000.f);
	}
	return false;
}

template <> const char* ComponentNode<ArcballCameraComponent>::name() { return "Arcball controller"; }
template <> bool ComponentNode<ArcballCameraComponent>::draw(ArcballCameraComponent& controller)
{
	bool updated = false;
	updated |= ImGui::Checkbox("Active", &controller.active);
	updated |= ImGui::InputFloat3("Position", controller.position.data);
	updated |= ImGui::InputFloat3("Target", controller.target.data);
	updated |= ImGui::InputFloat3("Up", controller.up.data);
	updated |= ImGui::SliderFloat("Speed", &controller.speed, 0.1f, 100.f);
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
	ImGui::ColorEdit3("Color", light.color.data);
	ImGui::SliderFloat("Intensity", &light.intensity, 0.1f, 100.f);
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
	ImGui::ColorEdit3("Color", light.color.data);
	ImGui::SliderFloat("Intensity", &light.intensity, 0.1f, 100.f);
	return updated;
}

template <> const char* ComponentNode<MeshComponent>::name() { return "Mesh"; }
template <> bool ComponentNode<MeshComponent>::draw(MeshComponent& mesh) 
{ 
	ImGui::Text("Vertices : %u", mesh.submesh.mesh->getVertexCount());
	ImGui::Text("Indices : %u", mesh.submesh.mesh->getIndexCount());
	ImGui::Text("Index count : %u", mesh.submesh.indexCount);
	ImGui::Text("Index offset : %u", mesh.submesh.indexOffset);
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
	return false;
}

template <> const char* ComponentNode<MaterialComponent>::name() { return "Material"; }
template <> bool ComponentNode<MaterialComponent>::draw(MaterialComponent& material) 
{
	ImGui::ColorEdit4("Color", material.color.data);
	ImGui::Checkbox("Double sided", &material.doubleSided);
	// TODO improve texture display
	ImGui::Image((ImTextureID)(uintptr_t)material.colorTexture->handle(), ImVec2(100, 100));
	ImGui::Image((ImTextureID)(uintptr_t)material.normalTexture->handle(), ImVec2(100, 100));
	ImGui::Image((ImTextureID)(uintptr_t)material.roughnessTexture->handle(), ImVec2(100, 100));
	return false; 
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
				world.registry().replace<T>(entity, component);
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

void onDirLightUpdate(entt::registry& registry, entt::entity entity)
{
	if (!registry.has<DirtyLightComponent>(entity))
		registry.emplace<DirtyLightComponent>(entity);
}

void onPointLightUpdate(entt::registry& registry, entt::entity entity)
{
	if (!registry.has<DirtyLightComponent>(entity))
		registry.emplace<DirtyLightComponent>(entity);
}

void onCameraUpdate(entt::registry& registry, entt::entity entity)
{
	auto dirLightUpdate = registry.view<DirectionalLightComponent>();
	for (entt::entity e : dirLightUpdate)
		if (!registry.has<DirtyLightComponent>(e))
			registry.emplace<DirtyLightComponent>(e);
}

void onTransformUpdate(entt::registry& registry, entt::entity entity)
{
	// Update point light if we moved it
	if (registry.has<PointLightComponent>(entity))
		registry.replace<PointLightComponent>(entity, registry.get<PointLightComponent>(entity));
	// Update lights if we changed the scene
	if (registry.has<MeshComponent>(entity))
	{
		auto dirLightUpdate = registry.view<DirectionalLightComponent>();
		for (entt::entity e : dirLightUpdate)
			if (!registry.has<DirtyLightComponent>(e))
				registry.emplace<DirtyLightComponent>(e);
		auto pointLightUpdate = registry.view<PointLightComponent>();
		for (entt::entity e : pointLightUpdate)
			if (!registry.has<DirtyLightComponent>(e))
				registry.emplace<DirtyLightComponent>(e);
	}
	// TODO handle empty node that hold meshes
}

SceneEditor::SceneEditor() :
	m_currentEntity(entt::null),
	m_gizmoOperation(ImGuizmo::TRANSLATE)
{
}

void SceneEditor::onCreate(World& world)
{
	world.registry().on_update<Transform3DComponent>().connect<&onTransformUpdate>();
	world.registry().on_update<DirectionalLightComponent>().connect<&onDirLightUpdate>();
	world.registry().on_update<PointLightComponent>().connect<&onPointLightUpdate>();
	world.registry().on_update<Camera3DComponent>().connect<&onCameraUpdate>();
}

void SceneEditor::onDestroy(World& world)
{
	world.registry().on_update<Transform3DComponent>().disconnect<&onTransformUpdate>();
	world.registry().on_update<DirectionalLightComponent>().disconnect<&onDirLightUpdate>();
	world.registry().on_update<PointLightComponent>().disconnect<&onPointLightUpdate>();
	world.registry().on_update<Camera3DComponent>().connect<&onCameraUpdate>();
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

Entity getMainCamera(World& world)
{
	Entity cameraEntity = Entity::null();
	world.each([&cameraEntity](Entity entity) {
		// TODO get main camera
		if (entity.has<Camera3DComponent>())
			cameraEntity = entity;
	});
	return cameraEntity;
}

entt::entity pick(World& world)
{
	// Find a valid camera
	Entity cameraEntity = getMainCamera(world);
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

void SceneEditor::onUpdate(World& world)
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
	const TagComponent& tag = world.registry().get<TagComponent>(entity);

	auto it = childrens.find(entity);
	if (it != childrens.end())
	{
		char buffer[256];
		int err = snprintf(buffer, 256, "%s##%p", tag.name.cstr(), &tag);
		ImGuiTreeNodeFlags flags = 0;
		if (entity == current)
			flags |= ImGuiTreeNodeFlags_Selected;
		if (ImGui::TreeNodeEx(buffer, flags))
		{
			if (ImGui::IsItemClicked())
				current = entity;
			if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(1))
				ImGui::OpenPopup("ClosePopup");
			if (ImGui::BeginPopupContextItem("ClosePopup"))
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
		ImGui::Bullet();
		bool isSelected = current == entity;
		if (ImGui::Selectable(tag.name.cstr(), &isSelected))
			current = entity;
		if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(1))
			ImGui::OpenPopup("ClosePopup");
		if (ImGui::BeginPopupContextItem("ClosePopup"))
		{
			if (ImGui::MenuItem("Delete"))
				world.registry().destroy(entity);
			ImGui::EndPopup();
		}
	}
}

void SceneEditor::onRender(World& world)
{
	ImGuizmo::BeginFrame();
	if (ImGui::Begin("Scene", nullptr, ImGuiWindowFlags_MenuBar))
	{
		static const ImVec4 color = ImVec4(0.93f, 0.04f, 0.26f, 1.f);
		// --- Menu
		Entity e = Entity(m_currentEntity, &world);
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("Entity"))
			{
				if (ImGui::BeginMenu("Create"))
				{
					static char buffer[256];
					mat4f id = mat4f::identity();
					ImGui::InputTextWithHint("Name", "entity name", buffer, 256);
					if (ImGui::MenuItem("Mesh"))
					{
						// TODO load a mesh here
						m_currentEntity = Scene::createMesh(world, nullptr, nullptr).handle();
					}
					if (ImGui::MenuItem("Point light"))
					{
						m_currentEntity = Scene::createPointLight(world).handle();
					}
					if (ImGui::MenuItem("Directional light"))
					{
						m_currentEntity = Scene::createDirectionalLight(world).handle();
					}
					if (ImGui::MenuItem("Camera"))
					{
						m_currentEntity = Scene::createArcballCamera(world, new CameraPerspective).handle();// TODO leak here
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
						e.add<Transform3DComponent>();
					if (ImGui::MenuItem("Hierarchy", nullptr, nullptr, !e.has<Hierarchy3DComponent>()))
						e.add<Hierarchy3DComponent>();
					if (ImGui::MenuItem("Camera", nullptr, nullptr, !e.has<Camera3DComponent>()))
						e.add<Camera3DComponent>();
					if (ImGui::MenuItem("Arcball", nullptr, nullptr, !e.has<ArcballCameraComponent>()))
						e.add<ArcballCameraComponent>();
					if (ImGui::MenuItem("Mesh", nullptr, nullptr, !e.has<MeshComponent>()))
						e.add<MeshComponent>();
					if (ImGui::MenuItem("Material", nullptr, nullptr, !e.has<MaterialComponent>()))
						e.add<MaterialComponent>();
					if (ImGui::MenuItem("Point light", nullptr, nullptr, !e.has<PointLightComponent>()))
						e.add<PointLightComponent>();
					if (ImGui::MenuItem("Directional light", nullptr, nullptr, !e.has<DirectionalLightComponent>()))
						e.add<DirectionalLightComponent>();
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
					if (ImGui::MenuItem("Arcball", nullptr, nullptr, e.has<ArcballCameraComponent>()))
						e.remove<ArcballCameraComponent>();
					if (ImGui::MenuItem("Mesh", nullptr, nullptr, e.has<MeshComponent>()))
						e.remove<MeshComponent>();
					if (ImGui::MenuItem("Material", nullptr, nullptr, e.has<MaterialComponent>()))
						e.remove<MaterialComponent>();
					if (ImGui::MenuItem("Point light", nullptr, nullptr, e.has<PointLightComponent>()))
						e.remove<PointLightComponent>();
					if (ImGui::MenuItem("Directional light", nullptr, nullptr, e.has<DirectionalLightComponent>()))
						e.remove<DirectionalLightComponent>();
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
				if (h.parent == Entity::null())
					roots.push_back(entity);
				else
					childrens[h.parent.handle()].push_back(entity);
			}
			else
				roots.push_back(entity);
		});
		ImGui::TextColored(ImVec4(0.93f, 0.04f, 0.26f, 1.f), "Graph");
		if (ImGui::BeginChild("##list", ImVec2(0, 200), true))
		{
			for (entt::entity e : roots)
				recurse(world, e, childrens, m_currentEntity);
		}
		ImGui::EndChild();

		// --- Add entity
		static char entityName[256];
		ImGui::InputTextWithHint("##entityName", "Entity name", entityName, 256);
		ImGui::SameLine();
		if (ImGui::Button("Create entity"))
		{
			m_currentEntity = world.createEntity(entityName).handle();
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
				component<ArcballCameraComponent>(world, m_currentEntity);
			}
			// --- Gizmo
			Entity cameraEntity = getMainCamera(world);
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
					world.registry().replace<Transform3DComponent>(m_currentEntity, transform);
				
				// Draw debug views
				if (world.registry().has<MeshComponent>(m_currentEntity))
				{
					aabbox<> bounds(transform.transform * world.registry().get<MeshComponent>(m_currentEntity).bounds);
					Renderer3D::drawTransform(mat4f::translate(vec3f(bounds.center()))* mat4f::scale(bounds.extent() / 2.f));
				}
				if (world.registry().has<Camera3DComponent>(m_currentEntity))
				{
					// Draw a camera ?
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
				Renderer3D::render(GraphicBackend::backbuffer(), view, projection);
				Renderer3D::clear();
			}
		
		}
	}
	ImGui::End();
}

};