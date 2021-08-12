#include "Model.h"

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

void Scene::GraphSystem::create(World& world)
{
	//world.registry().on_construct<Hierarchy3DComponent>().connect<&onHierarchyAdd>();
	world.registry().on_destroy<Hierarchy3DComponent>().connect<&onHierarchyRemove>();
	//world.registry().on_update<Hierarchy3DComponent>().connect<&onHierarchyUpdate>();
}
void Scene::GraphSystem::destroy(World& world)
{
	//world.registry().on_construct<Hierarchy3DComponent>().disconnect<&onHierarchyAdd>();
	world.registry().on_destroy<Hierarchy3DComponent>().disconnect<&onHierarchyRemove>();
	//world.registry().on_update<Hierarchy3DComponent>().disconnect<&onHierarchyUpdate>();
}

// The update is deferred.
void Scene::GraphSystem::update(World& world, Time::Unit deltaTime)
{
	// TODO only update if a hierarchy node or transform node has been updated. use dirtyTransform ?
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

	// Move to light update.
	auto ligthView = world.registry().view<PointLightComponent>();
	for (entt::entity entity : ligthView)
	{
		PointLightComponent& l = r.get<PointLightComponent>(entity);
		// We are simply using 1/d2 as attenuation factor, and target for 5/256 as limit.
		l.radius = sqrt(l.intensity * 256.f / 5.f);
	}
}

void ArcballCameraComponent::set(const aabbox<>& bbox)
{
	float dist = bbox.extent().norm();
	position = bbox.max * 1.2f;
	target = bbox.center();
	up = norm3f(0, 1, 0);
	speed = dist;
}

void ArcballCameraSystem::update(World& world, Time::Unit deltaTime)
{
	auto view = world.registry().view<Transform3DComponent, Camera3DComponent, ArcballCameraComponent>();
	for (entt::entity entity : view)
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

Mesh::Ptr Scene::createCubeMesh(const point3f& position, float size)
{
	struct Vertex {
		point3f position;
		norm3f normal;
		uv2f uv;
		color4f color;
	};
	std::vector<Vertex> vertices;
	//std::vector<uint32_t> indices;
	// FRONT     BACK
	// 2______3__4______5
	// |      |  |      |
	// |      |  |      |
	// |      |  |      |
	// 0______1__6______7
	point3f p[8] = {
		point3f(-1, -1,  1),
		point3f(1, -1,  1),
		point3f(-1,  1,  1),
		point3f(1,  1,  1),
		point3f(-1,  1, -1),
		point3f(1,  1, -1),
		point3f(-1, -1, -1),
		point3f(1, -1, -1)
	};
	norm3f n[6] = {
		norm3f(0.f,  0.f,  1.f),
		norm3f(0.f,  1.f,  0.f),
		norm3f(0.f,  0.f, -1.f),
		norm3f(0.f, -1.f,  0.f),
		norm3f(1.f,  0.f,  0.f),
		norm3f(-1.f,  0.f,  0.f)
	};
	uv2f u[4] = {
		uv2f(0.f, 0.f),
		uv2f(1.f, 0.f),
		uv2f(0.f, 1.f),
		uv2f(1.f, 1.f)
	};
	color4f color = color4f(1.f);
	// Face 1
	vertices.push_back(Vertex{ p[0], n[0], u[0], color });
	vertices.push_back(Vertex{ p[1], n[0], u[1], color });
	vertices.push_back(Vertex{ p[2], n[0], u[2], color });
	vertices.push_back(Vertex{ p[2], n[0], u[2], color });
	vertices.push_back(Vertex{ p[1], n[0], u[1], color });
	vertices.push_back(Vertex{ p[3], n[0], u[3], color });
	// Face 2
	vertices.push_back(Vertex{ p[2], n[1], u[0], color });
	vertices.push_back(Vertex{ p[3], n[1], u[1], color });
	vertices.push_back(Vertex{ p[4], n[1], u[2], color });
	vertices.push_back(Vertex{ p[4], n[1], u[2], color });
	vertices.push_back(Vertex{ p[3], n[1], u[1], color });
	vertices.push_back(Vertex{ p[5], n[1], u[3], color });
	// Face 3
	vertices.push_back(Vertex{ p[4], n[2], u[0], color });
	vertices.push_back(Vertex{ p[5], n[2], u[1], color });
	vertices.push_back(Vertex{ p[6], n[2], u[2], color });
	vertices.push_back(Vertex{ p[6], n[2], u[2], color });
	vertices.push_back(Vertex{ p[5], n[2], u[1], color });
	vertices.push_back(Vertex{ p[7], n[2], u[3], color });
	// Face 4
	vertices.push_back(Vertex{ p[6], n[3], u[0], color });
	vertices.push_back(Vertex{ p[7], n[3], u[1], color });
	vertices.push_back(Vertex{ p[0], n[3], u[2], color });
	vertices.push_back(Vertex{ p[0], n[3], u[2], color });
	vertices.push_back(Vertex{ p[7], n[3], u[1], color });
	vertices.push_back(Vertex{ p[1], n[3], u[3], color });
	// Face 5
	vertices.push_back(Vertex{ p[1], n[4], u[0], color });
	vertices.push_back(Vertex{ p[7], n[4], u[1], color });
	vertices.push_back(Vertex{ p[3], n[4], u[2], color });
	vertices.push_back(Vertex{ p[3], n[4], u[2], color });
	vertices.push_back(Vertex{ p[7], n[4], u[1], color });
	vertices.push_back(Vertex{ p[5], n[4], u[3], color });
	// Face 6
	vertices.push_back(Vertex{ p[6], n[5], u[0], color });
	vertices.push_back(Vertex{ p[0], n[5], u[1], color });
	vertices.push_back(Vertex{ p[4], n[5], u[2], color });
	vertices.push_back(Vertex{ p[4], n[5], u[2], color });
	vertices.push_back(Vertex{ p[0], n[5], u[1], color });
	vertices.push_back(Vertex{ p[2], n[5], u[3], color });

	VertexAttribute att[4] = {
		VertexAttribute{ VertexFormat::Float, VertexType::Vec3},
		VertexAttribute{ VertexFormat::Float, VertexType::Vec3},
		VertexAttribute{ VertexFormat::Float, VertexType::Vec2},
		VertexAttribute{ VertexFormat::Float, VertexType::Vec4}
	};
	Mesh::Ptr mesh = Mesh::create();
	mesh->uploadInterleaved(att, 4, vertices.data(), (uint32_t)vertices.size());
	return mesh;
}

Mesh::Ptr Scene::createSphereMesh(const point3f& position, float radius, uint32_t segmentCount, uint32_t ringCount)
{
	struct Vertex {
		point3f position;
		norm3f normal;
		uv2f uv;
		color4f color;
	};
	// http://www.songho.ca/opengl/gl_sphere.html
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	float length = 1.f / radius;
	anglef sectorStep = 2.f * pi<float> / (float)ringCount;
	anglef stackStep = pi<float> / (float)segmentCount;
	anglef ringAngle, segmentAngle;
	aabbox<> bounds;

	for (uint32_t i = 0; i <= segmentCount; ++i)
	{
		segmentAngle = pi<float> / 2.f - (float)i * stackStep; // starting from pi/2 to -pi/2
		float xy = radius * cos(segmentAngle); // r * cos(u)
		float z = radius * sin(segmentAngle); // r * sin(u)

		// add (ringCount+1) vertices per segment
		// the first and last vertices have same position and normal, but different uv
		for (uint32_t j = 0; j <= ringCount; ++j)
		{
			Vertex v;
			ringAngle = (float)j * sectorStep; // starting from 0 to 2pi

			v.position.x = xy * cos(ringAngle); // r * cos(u) * cos(v)
			v.position.y = xy * sin(ringAngle); // r * cos(u) * sin(v)
			v.position.z = z;

			v.normal = norm3f(v.position / radius);

			v.uv.u = (float)j / ringCount;
			v.uv.v = (float)i / segmentCount;
			v.color = color4f(1.f);
			vertices.push_back(v);
			bounds.include(v.position);
		}
	}
	for (uint32_t i = 0; i < segmentCount; ++i)
	{
		uint32_t k1 = i * (ringCount + 1);     // beginning of current stack
		uint32_t k2 = k1 + ringCount + 1;      // beginning of next stack

		for (uint32_t j = 0; j < ringCount; ++j, ++k1, ++k2)
		{
			// 2 triangles per sector excluding first and last stacks
			// k1 => k2 => k1+1
			if (i != 0)
			{
				indices.push_back(k1);
				indices.push_back(k2);
				indices.push_back(k1 + 1);
			}
			// k1+1 => k2 => k2+1
			if (i != (segmentCount - 1))
			{
				indices.push_back(k1 + 1);
				indices.push_back(k2);
				indices.push_back(k2 + 1);
			}
		}
	}
	VertexAttribute att[4] = {
		VertexAttribute{ VertexFormat::Float, VertexType::Vec3},
		VertexAttribute{ VertexFormat::Float, VertexType::Vec3},
		VertexAttribute{ VertexFormat::Float, VertexType::Vec2},
		VertexAttribute{ VertexFormat::Float, VertexType::Vec4}
	};
	Mesh::Ptr mesh = Mesh::create();
	mesh->uploadInterleaved(att, 4, vertices.data(), (uint32_t)vertices.size(), IndexFormat::UnsignedInt, indices.data(), (uint32_t)indices.size());
	return mesh;
}

Entity Scene::createSphere(World& world, uint32_t segmentCount, uint32_t ringCount)
{
	Mesh::Ptr m = createSphereMesh(point3f(0.f), 1.f, segmentCount, ringCount);
	uint8_t data[4]{ 255, 255, 255, 255 };
	Texture::Ptr blank = Texture::create2D(1, 1, TextureFormat::UnsignedByte, TextureComponent::RGBA, TextureFlag::None, Sampler{}, data);
	uint8_t n[4]{ 128, 128, 255, 255 };
	Texture::Ptr normal = Texture::create2D(1, 1, TextureFormat::UnsignedByte, TextureComponent::RGBA, TextureFlag::None, Sampler{}, n);

	mat4f id = mat4f::identity();
	Entity mesh = world.createEntity("New uv sphere");
	mesh.add<Transform3DComponent>(Transform3DComponent{ id });
	mesh.add<Hierarchy3DComponent>(Hierarchy3DComponent{ Entity::null(), id });
	mesh.add<MeshComponent>(MeshComponent{ SubMesh{ m, PrimitiveType::Triangles, (uint32_t)m->getIndexCount(), 0 }, aabbox<>(point3f(-1), point3f(1)) });
	mesh.add<MaterialComponent>(MaterialComponent{ color4f(1.f), true, blank, normal, blank });
	return mesh;
}

Entity Scene::createCube(World& world)
{
	uint8_t colorData[4]{ 255, 255, 255, 255 };
	Texture::Ptr blank = Texture::create2D(1, 1, TextureFormat::UnsignedByte, TextureComponent::RGBA, TextureFlag::None, Sampler{}, colorData);
	uint8_t normalData[4]{ 128, 128, 255, 255 };
	Texture::Ptr normal = Texture::create2D(1, 1, TextureFormat::UnsignedByte, TextureComponent::RGBA, TextureFlag::None, Sampler{}, normalData);

	Mesh::Ptr m = createCubeMesh(point3f(0.f), 1.f);
	mat4f id = mat4f::identity();
	Entity mesh = world.createEntity("New cube");
	mesh.add<Transform3DComponent>(Transform3DComponent{ id });
	mesh.add<Hierarchy3DComponent>(Hierarchy3DComponent{ Entity::null(), id });
	mesh.add<MeshComponent>(MeshComponent{ SubMesh{ m, PrimitiveType::Triangles, (uint32_t)m->getVertexCount(), 0 }, aabbox<>(point3f(-1), point3f(1)) });
	mesh.add<MaterialComponent>(MaterialComponent{ color4f(1.f), true, blank, normal, blank });
	return mesh;
}

Entity Scene::createPointLight(World& world)
{
	mat4f id = mat4f::identity();
	Sampler shadowSampler{};
	shadowSampler.filterMag = Sampler::Filter::Nearest;
	shadowSampler.filterMin = Sampler::Filter::Nearest;
	shadowSampler.wrapU = Sampler::Wrap::ClampToEdge;
	shadowSampler.wrapV = Sampler::Wrap::ClampToEdge;
	shadowSampler.wrapW = Sampler::Wrap::ClampToEdge;
	Entity light = world.createEntity("New point light");
	light.add<Transform3DComponent>(Transform3DComponent{ id });
	light.add<Hierarchy3DComponent>(Hierarchy3DComponent{ Entity::null(), id });
	light.add<PointLightComponent>(PointLightComponent{
		color3f(1.f),
		1.f,
		{ id, id, id, id, id, id },
		Texture::createCubemap(1024, 1024, TextureFormat::Float, TextureComponent::Depth, TextureFlag::RenderTarget, shadowSampler)
	});
	return light;
}

Entity Scene::createDirectionalLight(World& world)
{
	mat4f id = mat4f::identity();
	Entity light = world.createEntity("New directionnal light");
	light.add<Transform3DComponent>(Transform3DComponent{ id });
	light.add<Hierarchy3DComponent>(Hierarchy3DComponent{ Entity::null(), id });
	light.add<DirectionalLightComponent>(DirectionalLightComponent{
		vec3f(0.f, 1.f, 0.f),
		color3f(1.f),
		1.f,
		{ id, id, id },
		{
			Texture::create2D(2048, 2048, TextureFormat::Float, TextureComponent::Depth, TextureFlag::RenderTarget, Sampler::nearest()),
			Texture::create2D(2048, 2048, TextureFormat::Float, TextureComponent::Depth, TextureFlag::RenderTarget, Sampler::nearest()),
			Texture::create2D(2048, 2048, TextureFormat::Float, TextureComponent::Depth, TextureFlag::RenderTarget, Sampler::nearest())
		},
		{ 1.f, 1.f, 1.f }
	});
	return light;
}

Entity Scene::createArcballCamera(World& world, CameraProjection* projection)
{
	mat4f id = mat4f::identity();
	Entity camera = world.createEntity("New arcball camera");
	camera.add<Transform3DComponent>(Transform3DComponent{ id });
	camera.add<Hierarchy3DComponent>(Hierarchy3DComponent{ Entity::null(), id });
	camera.add<Camera3DComponent>(Camera3DComponent{
		id,
		projection
	});
	camera.add<ArcballCameraComponent>(ArcballCameraComponent{
		point3f(1.f),
		point3f(0.f),
		norm3f(0.f, 1.f, 0.f),
		1.f,
		true
	});
	return camera;
}

};