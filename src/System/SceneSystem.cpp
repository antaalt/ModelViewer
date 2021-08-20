#include "SceneSystem.h"

#include "../Model/Model.h"

namespace viewer {

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
	//world.registry().on_construct<Hierarchy3DComponent>().connect<&onHierarchyAdd>();
	world.registry().on_destroy<Hierarchy3DComponent>().connect<&onHierarchyRemove>();
	//world.registry().on_update<Hierarchy3DComponent>().connect<&onHierarchyUpdate>();
}

void SceneSystem::onDestroy(aka::World& world)
{
	//world.registry().on_construct<Hierarchy3DComponent>().disconnect<&onHierarchyAdd>();
	world.registry().on_destroy<Hierarchy3DComponent>().disconnect<&onHierarchyRemove>();
	//world.registry().on_update<Hierarchy3DComponent>().disconnect<&onHierarchyUpdate>();
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
	auto arcballView = world.registry().view<Transform3DComponent, Camera3DComponent, ArcballCameraComponent>();
	for (entt::entity entity : arcballView)
	{
		Transform3DComponent& transform = world.registry().get<Transform3DComponent>(entity);
		ArcballCameraComponent& controller = world.registry().get<ArcballCameraComponent>(entity);
		Camera3DComponent& camera = world.registry().get<Camera3DComponent>(entity);
		if (!controller.active)
			return;
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
			vec3f upCamera = vec3f(0, 1, 0);
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
			// TODO make it scene graph compatible
			transform.transform = mat4f::lookAt(controller.position, controller.target, controller.up);
			camera.view = mat4f::inverse(transform.transform);
			world.registry().replace<Camera3DComponent>(entity, camera);
		}
	}
}

}