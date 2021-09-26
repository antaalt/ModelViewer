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
	Camera3DComponent& c = registry.get<Camera3DComponent>(entity);
	// Update transform & view
	// Controller return a world transform. Make it local ?
	registry.get<Transform3DComponent>(entity).transform = c.controller->transform();
	c.view = mat4f::inverse(registry.get<Transform3DComponent>(entity).transform);
	if (c.active)
	{
		// Update directionnal light that depend on view matrix.
		auto dirLights = registry.view<DirectionalLightComponent>();
		for (entt::entity e : dirLights)
			if (!registry.has<DirtyLightComponent>(e))
				registry.emplace<DirtyLightComponent>(e);
	}
}

void onTransformUpdate(entt::registry& registry, entt::entity entity)
{
	// Update point light if we moved it
	if (registry.has<PointLightComponent>(entity))
		registry.patch<PointLightComponent>(entity);
	// Update lights if we changed the scene
	if (registry.has<MeshComponent>(entity))
	{
		auto dirLightUpdate = registry.view<DirectionalLightComponent>();
		for (entt::entity e : dirLightUpdate)
			if (!registry.has<DirtyLightComponent>(e))
				registry.emplace<DirtyLightComponent>(e);
		MeshComponent& m = registry.get<MeshComponent>(entity);
		auto pointLightUpdate = registry.view<Transform3DComponent, PointLightComponent>();
		for (entt::entity e : pointLightUpdate)
		{
			point3f c(registry.get<Transform3DComponent>(e).transform.cols[3]);
			if (m.bounds.overlap(c, registry.get<PointLightComponent>(e).radius))
				if (!registry.has<DirtyLightComponent>(e))
					registry.emplace<DirtyLightComponent>(e);
		}
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
		if (h.parent.valid())
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
	auto cameraTransformView = world.registry().view<Transform3DComponent, Camera3DComponent>();
	for (entt::entity entity : cameraTransformView)
	{
		Transform3DComponent& transform = world.registry().get<Transform3DComponent>(entity);
		Camera3DComponent& camera = world.registry().get<Camera3DComponent>(entity);
		if (!camera.active || camera.controller == nullptr)
			continue;
		if (camera.controller->update(deltaTime))
		{
			world.registry().patch<Camera3DComponent>(entity);
		}
	}
	
	// --- Update camera ratio
	// TODO use event instead
	auto cameraView = world.registry().view<Camera3DComponent>();
	GraphicDevice* device = GraphicBackend::device();
	Framebuffer::Ptr backbuffer = device->backbuffer();
	for (entt::entity entity : cameraView)
	{
		Camera3DComponent& camera = world.registry().get<Camera3DComponent>(entity);
		aka::CameraPerspective* proj = dynamic_cast<aka::CameraPerspective*>(camera.projection.get());
		if (proj != nullptr)
			proj->ratio = backbuffer->width() / (float)backbuffer->height();
	}
}

}