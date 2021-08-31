#include "Model.h"

namespace viewer {

void ArcballCameraComponent::set(const aabbox<>& bbox)
{
	float dist = bbox.extent().norm();
	position = bbox.max * 1.2f;
	target = bbox.center();
	up = norm3f(0, 1, 0);
	speed = dist;
}

Entity Scene::getMainCamera(World& world)
{
	Entity cameraEntity = Entity::null();
	world.each([&cameraEntity](Entity entity) {
		// TODO get main camera
		if (entity.has<Camera3DComponent>())
			cameraEntity = entity;
	});
	return cameraEntity;
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
		VertexAttribute{ VertexSemantic::Position, VertexFormat::Float, VertexType::Vec3},
		VertexAttribute{ VertexSemantic::Normal, VertexFormat::Float, VertexType::Vec3},
		VertexAttribute{ VertexSemantic::TexCoord0, VertexFormat::Float, VertexType::Vec2},
		VertexAttribute{ VertexSemantic::Color0, VertexFormat::Float, VertexType::Vec4}
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
		VertexAttribute{ VertexSemantic::Position, VertexFormat::Float, VertexType::Vec3},
		VertexAttribute{ VertexSemantic::Normal, VertexFormat::Float, VertexType::Vec3},
		VertexAttribute{ VertexSemantic::TexCoord0, VertexFormat::Float, VertexType::Vec2},
		VertexAttribute{ VertexSemantic::Color0, VertexFormat::Float, VertexType::Vec4}
	};
	Mesh::Ptr mesh = Mesh::create();
	mesh->uploadInterleaved(att, 4, vertices.data(), (uint32_t)vertices.size(), IndexFormat::UnsignedInt, indices.data(), (uint32_t)indices.size());
	return mesh;
}

Entity Scene::createSphereEntity(World& world, uint32_t segmentCount, uint32_t ringCount)
{
	Mesh::Ptr m = createSphereMesh(point3f(0.f), 1.f, segmentCount, ringCount);
	uint8_t data[4]{ 255, 255, 255, 255 };
	Texture::Ptr blank = Texture::create2D(1, 1, TextureFormat::RGBA8, TextureFlag::None, Sampler{}, data);
	uint8_t n[4]{ 128, 128, 255, 255 };
	Texture::Ptr normal = Texture::create2D(1, 1, TextureFormat::RGBA8, TextureFlag::None, Sampler{}, n);

	mat4f id = mat4f::identity();
	Entity mesh = world.createEntity("New uv sphere");
	mesh.add<Transform3DComponent>(Transform3DComponent{ id });
	mesh.add<Hierarchy3DComponent>(Hierarchy3DComponent{ Entity::null(), id });
	mesh.add<MeshComponent>(MeshComponent{ SubMesh{ m, PrimitiveType::Triangles, (uint32_t)m->getIndexCount(), 0 }, aabbox<>(point3f(-1), point3f(1)) });
	mesh.add<MaterialComponent>(MaterialComponent{ color4f(1.f), true, blank, normal, blank });
	return mesh;
}

Entity Scene::createCubeEntity(World& world)
{
	uint8_t colorData[4]{ 255, 255, 255, 255 };
	Texture::Ptr blank = Texture::create2D(1, 1, TextureFormat::RGBA8, TextureFlag::None, Sampler{}, colorData);
	uint8_t normalData[4]{ 128, 128, 255, 255 };
	Texture::Ptr normal = Texture::create2D(1, 1, TextureFormat::RGBA8, TextureFlag::None, Sampler{}, normalData);

	Mesh::Ptr m = createCubeMesh(point3f(0.f), 1.f);
	mat4f id = mat4f::identity();
	Entity mesh = world.createEntity("New cube");
	mesh.add<Transform3DComponent>(Transform3DComponent{ id });
	mesh.add<Hierarchy3DComponent>(Hierarchy3DComponent{ Entity::null(), id });
	mesh.add<MeshComponent>(MeshComponent{ SubMesh{ m, PrimitiveType::Triangles, (uint32_t)m->getVertexCount(), 0 }, aabbox<>(point3f(-1), point3f(1)) });
	mesh.add<MaterialComponent>(MaterialComponent{ color4f(1.f), true, blank, normal, blank });
	return mesh;
}

Entity Scene::createPointLightEntity(World& world)
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
		Texture::createCubemap(1024, 1024, TextureFormat::Depth16, TextureFlag::RenderTarget, shadowSampler)
	});
	return light;
}

Entity Scene::createDirectionalLightEntity(World& world)
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
			Texture::create2D(2048, 2048, TextureFormat::Depth16, TextureFlag::RenderTarget, Sampler::nearest()),
			Texture::create2D(2048, 2048, TextureFormat::Depth16, TextureFlag::RenderTarget, Sampler::nearest()),
			Texture::create2D(2048, 2048, TextureFormat::Depth16, TextureFlag::RenderTarget, Sampler::nearest())
		},
		{ 1.f, 1.f, 1.f }
	});
	return light;
}

Entity Scene::createArcballCameraEntity(World& world, CameraProjection* projection)
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