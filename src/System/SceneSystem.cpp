#include "SceneSystem.h"

#include "../Model/Model.h"

namespace viewer {

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
	auto dirLights = registry.view<DirectionalLightComponent>();
	for (entt::entity e : dirLights)
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
		// TODO only update light affected by mesh bounds
		auto pointLightUpdate = registry.view<PointLightComponent>();
		for (entt::entity e : pointLightUpdate)
			if (!registry.has<DirtyLightComponent>(e))
				registry.emplace<DirtyLightComponent>(e);
	}
	// TODO handle empty node that hold meshes
}

void onHierarchyRemove(entt::registry& registry, entt::entity entity)
{
	if (!registry.has<Transform3DComponent>(entity))
		return;
	Transform3DComponent& t = registry.get<Transform3DComponent>(entity);
	Hierarchy3DComponent& h = registry.get<Hierarchy3DComponent>(entity);
	// Restore local transform
	t.transform = h.inverseTransform * t.transform;
}

void SceneSystem::onCreate(aka::World& world)
{
	world.registry().on_destroy<Hierarchy3DComponent>().connect<&onHierarchyRemove>();

	world.registry().on_update<Transform3DComponent>().connect<&onTransformUpdate>();
	world.registry().on_update<DirectionalLightComponent>().connect<&onDirLightUpdate>();
	world.registry().on_update<PointLightComponent>().connect<&onPointLightUpdate>();
	world.registry().on_update<Camera3DComponent>().connect<&onCameraUpdate>();
}

void SceneSystem::onDestroy(aka::World& world)
{
	world.registry().on_destroy<Hierarchy3DComponent>().disconnect<&onHierarchyRemove>();

	world.registry().on_update<Transform3DComponent>().disconnect<&onTransformUpdate>();
	world.registry().on_update<DirectionalLightComponent>().disconnect<&onDirLightUpdate>();
	world.registry().on_update<PointLightComponent>().disconnect<&onPointLightUpdate>();
	world.registry().on_update<Camera3DComponent>().disconnect<&onCameraUpdate>();
}

void SceneSystem::onUpdate(aka::World& world, aka::Time::Unit deltaTime)
{
	// --- Update hierarchy transfom.
	// TODO only update if a hierarchy node or transform node has been updated. use dirtyTransform ?
	entt::registry& r = world.registry();
	// Sort hierarchy to ensure correct order.
	// https://skypjack.github.io/2019-08-20-ecs-baf-part-4-insights/
	// https://wickedengine.net/2019/09/29/entity-component-system/
	r.sort<Hierarchy3DComponent>([&r](const entt::entity lhs, entt::entity rhs) {
		const Hierarchy3DComponent& clhs = r.get<Hierarchy3DComponent>(lhs);
		const Hierarchy3DComponent& crhs = r.get<Hierarchy3DComponent>(rhs);
		return lhs < rhs&& clhs.parent.handle() != rhs;
	});
	// Compute transforms
	auto transformView = world.registry().view<Hierarchy3DComponent, Transform3DComponent>();
	for (entt::entity entity : transformView)
	{
		Transform3DComponent& t = r.get<Transform3DComponent>(entity);
		Hierarchy3DComponent& h = r.get<Hierarchy3DComponent>(entity);
		if (h.parent != Entity::null())
		{
			mat4f localTransform = h.inverseTransform * t.transform;
			t.transform = h.parent.get<Transform3DComponent>().transform * localTransform;
			h.inverseTransform = mat4f::inverse(h.parent.get<Transform3DComponent>().transform);
		}
		else
		{
			h.inverseTransform = mat4f::identity();
		}
	}

	// --- Update light volumes.
	auto ligthView = world.registry().view<PointLightComponent>();
	for (entt::entity entity : ligthView)
	{
		PointLightComponent& l = r.get<PointLightComponent>(entity);
		// We are simply using 1/d2 as attenuation factor, and target for 5/256 as limit.
		l.radius = sqrt(l.intensity * 256.f / 5.f);
	}

	// --- Update arball camera
	// TODO move to separate camera system, rename this HierarchySystem
	auto arcballView = world.registry().view<Transform3DComponent, Camera3DComponent, ArcballCameraComponent>();
	for (entt::entity entity : arcballView)
	{
		Transform3DComponent& transform = world.registry().get<Transform3DComponent>(entity);
		ArcballCameraComponent& controller = world.registry().get<ArcballCameraComponent>(entity);
		Camera3DComponent& camera = world.registry().get<Camera3DComponent>(entity);
		if (!controller.active)
			continue;
		bool dirty = false;
		// https://gamedev.stackexchange.com/questions/53333/how-to-implement-a-basic-arcball-camera-in-opengl-with-glm
		if (Mouse::pressed(MouseButton::ButtonLeft) && (Mouse::delta().x != 0.f || Mouse::delta().y != 0.f))
		{
			float x = Mouse::delta().x * deltaTime.seconds();
			float y = -Mouse::delta().y * deltaTime.seconds();
			anglef pitch = anglef::radian(y);
			anglef yaw = anglef::radian(x);
			vec3f upCamera = vec3f(0, 1, 0);
			vec3f forwardCamera = vec3f::normalize(controller.target - controller.position);
			vec3f rightCamera = vec3f::normalize(vec3f::cross(forwardCamera, vec3f(upCamera)));
			controller.position = mat4f::rotate(rightCamera, pitch).multiplyPoint(point3f(controller.position - controller.target)) + vec3f(controller.target);
			controller.position = mat4f::rotate(upCamera, yaw).multiplyPoint(point3f(controller.position - controller.target)) + vec3f(controller.target);
			dirty = true;
		}
		if (Mouse::pressed(MouseButton::ButtonRight) && (Mouse::delta().x != 0.f || Mouse::delta().y != 0.f))
		{
			float x = -Mouse::delta().x * deltaTime.seconds();
			float y = -Mouse::delta().y * deltaTime.seconds();
			vec3f upCamera = vec3f(0, 1, 0); // TODO change it when close to up
			vec3f forwardCamera = vec3f::normalize(controller.target - controller.position);
			vec3f rightCamera = vec3f::normalize(vec3f::cross(forwardCamera, vec3f(upCamera)));
			vec3f move = rightCamera * x * controller.speed / 2.f + upCamera * y * controller.speed / 2.f;
			controller.target += move;
			controller.position += move;
			dirty = true;
		}
		if (Mouse::scroll().y != 0.f)
		{
			float zoom = Mouse::scroll().y * deltaTime.seconds();
			vec3f dir = vec3f::normalize(controller.target - controller.position);
			float dist = point3f::distance(controller.target, controller.position);
			float coeff = zoom * controller.speed;
			if (dist - coeff > 1.5f)
			{
				controller.position = controller.position + dir * coeff;
				dirty = true;
			}
		}
		if (dirty)
		{
			mat4f parentMatrix;
			if (world.registry().has<Hierarchy3DComponent>(entity))
			{
				Hierarchy3DComponent& h = world.registry().get<Hierarchy3DComponent>(entity);
				if (h.parent != Entity::null() && world.registry().valid(h.parent.handle()))
					parentMatrix = h.parent.get<Transform3DComponent>().transform;
				else
					parentMatrix = mat4f::identity();
			}
			else
				parentMatrix = mat4f::identity();
			transform.transform = parentMatrix * mat4f::lookAt(controller.position, controller.target, controller.up);
			camera.view = mat4f::inverse(transform.transform);
			world.registry().replace<Camera3DComponent>(entity, camera);
		}
	}
	
	// --- Update camera ratio
	auto cameraView = world.registry().view<Camera3DComponent>();
	Framebuffer::Ptr backbuffer = GraphicBackend::backbuffer();
	for (entt::entity entity : cameraView)
	{
		Camera3DComponent& camera = world.registry().get<Camera3DComponent>(entity);
		aka::CameraPerspective* proj = dynamic_cast<aka::CameraPerspective*>(camera.projection);
		if (proj != nullptr)
			proj->ratio = backbuffer->width() / (float)backbuffer->height();
	}
}

}