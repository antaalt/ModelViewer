#include "Model.h"

namespace viewer {


void onHierarchyAdd(entt::registry& registry, entt::entity entity)
{
	// Set id matrix.
	registry.get<Hierarchy3DComponent>(entity).inverseTransform = mat4f::identity();
	registry.get<Hierarchy3DComponent>(entity).parent = Entity::null();
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

void SceneGraph::create(World& world)
{
	world.registry().on_construct<Hierarchy3DComponent>().connect<&onHierarchyAdd>();
	world.registry().on_destroy<Hierarchy3DComponent>().connect<&onHierarchyRemove>();
	//world.registry().on_update<Hierarchy3DComponent>().connect<&onHierarchyUpdate>();
}
void SceneGraph::destroy(World& world)
{
	world.registry().on_construct<Hierarchy3DComponent>().disconnect<&onHierarchyAdd>();
	world.registry().on_destroy<Hierarchy3DComponent>().disconnect<&onHierarchyRemove>();
	//world.registry().on_update<Hierarchy3DComponent>().disconnect<&onHierarchyUpdate>();
}

// The update is deferred.
void SceneGraph::update(World& world, Time::Unit deltaTime)
{
	entt::registry& r = world.registry();
	// Sort hierarchy to ensure correct order.
	// https://skypjack.github.io/2019-08-20-ecs-baf-part-4-insights/
	// https://wickedengine.net/2019/09/29/entity-component-system/
	r.sort<Hierarchy3DComponent>([&r](const entt::entity lhs, entt::entity rhs) {
		const Hierarchy3DComponent& clhs = r.get<Hierarchy3DComponent>(lhs);
		const Hierarchy3DComponent& crhs = r.get<Hierarchy3DComponent>(rhs);
		return lhs < rhs && clhs.parent.handle() != rhs;
		//return lhs != crhs.parent.handle();
	});
	// Compute transforms
	auto view = world.registry().view<Hierarchy3DComponent, Transform3DComponent>();
	for (entt::entity entity : view)
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
}

ArcballCamera::ArcballCamera() :
	ArcballCamera(aabbox(point3f(0.f), point3f(1.f)))
{
}

ArcballCamera::ArcballCamera(const aabbox<>& bbox)
{
	set(bbox);
}

void ArcballCamera::rotate(float x, float y)
{
	anglef pitch = anglef::radian(y);
	anglef yaw = anglef::radian(x);
	vec3f upCamera = vec3f(0, 1, 0);
	vec3f forwardCamera = vec3f::normalize(m_target - m_position);
	vec3f rightCamera = vec3f::normalize(vec3f::cross(forwardCamera, vec3f(upCamera)));
	m_position = mat4f::rotate(rightCamera, pitch).multiplyPoint(point3f(m_position - m_target)) + vec3f(m_target);
	m_position = mat4f::rotate(upCamera, yaw).multiplyPoint(point3f(m_position - m_target)) + vec3f(m_target);
}

void ArcballCamera::pan(float x, float y)
{
	vec3f upCamera = vec3f(0, 1, 0);
	vec3f forwardCamera = vec3f::normalize(m_target - m_position);
	vec3f rightCamera = vec3f::normalize(vec3f::cross(forwardCamera, vec3f(upCamera)));
	vec3f move = rightCamera * x * m_speed / 2.f + upCamera * y * m_speed / 2.f;
	m_target += move;
	m_position += move;
}

void ArcballCamera::zoom(float zoom)
{
	vec3f dir = vec3f::normalize(m_target - m_position);
	float dist = point3f::distance(m_target, m_position);
	float coeff = zoom * m_speed;
	if (dist - coeff > 1.5f)
		m_position = m_position + dir * coeff;
}

void ArcballCamera::set(const aabbox<>& bbox)
{
	float dist = bbox.extent().norm();
	m_position = bbox.max * 1.2f;
	m_target = bbox.center();
	m_up = norm3f(0,1,0);
	m_transform = aka::mat4f::lookAt(m_position, m_target, m_up);
	m_speed = dist;
}

void ArcballCamera::update(Time::Unit deltaTime)
{
	// https://gamedev.stackexchange.com/questions/53333/how-to-implement-a-basic-arcball-camera-in-opengl-with-glm
	if (Mouse::pressed(MouseButton::ButtonLeft))
		rotate(Mouse::delta().x * deltaTime.seconds(), -Mouse::delta().y * deltaTime.seconds());
	if (Mouse::pressed(MouseButton::ButtonRight))
		pan(-Mouse::delta().x * deltaTime.seconds(), -Mouse::delta().y * deltaTime.seconds());
	if (Mouse::scroll().y != 0.f)
		zoom(Mouse::scroll().y * deltaTime.seconds());
	m_transform = mat4f::lookAt(m_position, m_target, m_up);
}

};