#include "Model.h"

#include "json.hpp"

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
	mesh.add<MeshComponent>(MeshComponent{ SubMesh{ m, PrimitiveType::Triangles, (uint32_t)m->getVertexCount(0), 0 }, aabbox<>(point3f(-1), point3f(1)) });
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


template <typename T>
nlohmann::json serialize(const entt::registry& r, entt::entity e);

template <typename T>
nlohmann::json serialize(const entt::registry& r, entt::entity e)
{
	Logger::error("Component serialization not implemented");
	return nlohmann::json();
}
template <>
nlohmann::json serialize<TagComponent>(const entt::registry& r, entt::entity e)
{
	const TagComponent& h = r.get<TagComponent>(e);
	nlohmann::json json = nlohmann::json::object();
	json["name"] = h.name.cstr();
	return json;
}
template <>
nlohmann::json serialize<Transform3DComponent>(const entt::registry& r, entt::entity e)
{
	const Transform3DComponent& t = r.get<Transform3DComponent>(e);
	mat4f local;
	if (r.has<Hierarchy3DComponent>(e))
		local = r.get<Hierarchy3DComponent>(e).inverseTransform * t.transform;
	else
		local = t.transform;
	nlohmann::json json = nlohmann::json::object();
	json["matrix"] = nlohmann::json::array({ 
		local[0][0], local[0][1], local[0][2], local[0][3],
		local[1][0], local[1][1], local[1][2], local[1][3],
		local[2][0], local[2][1], local[2][2], local[2][3],
		local[3][0], local[3][1], local[3][2], local[3][3],
	});
	return json;
}
template <>
nlohmann::json serialize<Hierarchy3DComponent>(const entt::registry& r, entt::entity e)
{
	const Hierarchy3DComponent& h = r.get<Hierarchy3DComponent>(e);
	nlohmann::json json = nlohmann::json::object();
	if (h.parent.valid())
		json["parent"] = (entt::id_type)h.parent.handle();
	else
		json["parent"] = nlohmann::json();
	// inverse transform is useless to store as we store local transform.
	return json;
}
template <>
nlohmann::json serialize<MeshComponent>(const entt::registry& r, entt::entity e)
{
	const MeshComponent& m = r.get<MeshComponent>(e);
	nlohmann::json json = nlohmann::json::object();
	json["bounds"]["min"] = { m.bounds.min.x, m.bounds.min.y, m.bounds.min.z };
	json["bounds"]["max"] = { m.bounds.max.x, m.bounds.max.y, m.bounds.max.z };
	json["mesh"] = ResourceManager::name<Mesh>(m.submesh.mesh).cstr();
	return json;
}
template <>
nlohmann::json serialize<MaterialComponent>(const entt::registry& r, entt::entity e)
{
	const MaterialComponent& m = r.get<MaterialComponent>(e);
	nlohmann::json json = nlohmann::json::object();
	json["color"] = { m.color.r, m.color.g, m.color.b, m.color.a };
	json["doublesided"] = m.doubleSided;
	json["textures"]["color"] = ResourceManager::name<Texture>(m.colorTexture).cstr();
	json["textures"]["normal"] = ResourceManager::name<Texture>(m.normalTexture).cstr();
	json["textures"]["material"] = ResourceManager::name<Texture>(m.materialTexture).cstr();
	return json;
}
template <>
nlohmann::json serialize<DirectionalLightComponent>(const entt::registry& r, entt::entity e)
{
	const DirectionalLightComponent& l = r.get<DirectionalLightComponent>(e);
	nlohmann::json json = nlohmann::json::object();
	json["direction"] = { l.direction.x, l.direction.y, l.direction.z };
	json["intensity"] = l.intensity;
	json["color"] = { l.color.r, l.color.g, l.color.b };
	return json;
}
template <>
nlohmann::json serialize<PointLightComponent>(const entt::registry& r, entt::entity e)
{
	const PointLightComponent& l = r.get<PointLightComponent>(e);
	nlohmann::json json = nlohmann::json::object();
	json["intensity"] = l.intensity;
	json["color"] = { l.color.r, l.color.g, l.color.b };
	return json;
}
template <>
nlohmann::json serialize<Camera3DComponent>(const entt::registry& r, entt::entity e)
{
	const Camera3DComponent& c = r.get<Camera3DComponent>(e);
	nlohmann::json json = nlohmann::json::object();
	// view is inverse transform, no need to store
	CameraPerspective* perspective = dynamic_cast<CameraPerspective*>(c.projection);
	CameraOrthographic* orthographic = dynamic_cast<CameraOrthographic*>(c.projection);
	if (perspective != nullptr)
	{
		json["perspective"]["near"] = perspective->nearZ;
		json["perspective"]["far"] = perspective->farZ;
		json["perspective"]["ratio"] = perspective->ratio;
		json["perspective"]["fov"] = perspective->hFov.degree();
	}
	else if (orthographic != nullptr)
	{
		// TODO use near, far, left, right, top bottom
		json["orthographic"]["x"] = orthographic->viewport.x;
		json["orthographic"]["y"] = orthographic->viewport.y;
	}
	return json;
}
template <>
nlohmann::json serialize<ArcballCameraComponent>(const entt::registry& r, entt::entity e)
{
	const ArcballCameraComponent& c = r.get<ArcballCameraComponent>(e);
	nlohmann::json json = nlohmann::json::object();
	json["position"] = { c.position.x, c.position.y, c.position.z };
	json["target"] = { c.target.x, c.target.y, c.target.z };
	json["up"] = { c.up.x, c.up.y, c.up.z };
	json["active"] = c.active;
	json["speed"] = c.speed;
	return json;
}

static uint16_t major = 0;
static uint16_t minor = 1;

void Scene::save(const Path& path, const World& world)
{
	const entt::registry& r = world.registry();
	nlohmann::json json = nlohmann::json::object();
	// --- Version
	json["asset"] = nlohmann::json::object();
	json["asset"]["version"] = std::to_string(major) + "." + std::to_string(minor);
	json["asset"]["exporter"] = "aka engine";
	// --- Entities
	json["entities"] = nlohmann::json::object();
	r.each([&](entt::entity e) {
		nlohmann::json entity;
		entity["components"] = nlohmann::json::object();
		if (r.has<TagComponent>(e))              entity["components"]["tag"] = serialize<TagComponent>(r, e);
		if (r.has<Transform3DComponent>(e))      entity["components"]["transform"] = serialize<Transform3DComponent>(r, e);
		if (r.has<Hierarchy3DComponent>(e))      entity["components"]["hierarchy"] = serialize<Hierarchy3DComponent>(r, e);
		if (r.has<MeshComponent>(e))             entity["components"]["mesh"] = serialize<MeshComponent>(r, e);
		if (r.has<MaterialComponent>(e))         entity["components"]["material"] = serialize<MaterialComponent>(r, e);
		if (r.has<DirectionalLightComponent>(e)) entity["components"]["dirlight"] = serialize<DirectionalLightComponent>(r, e);
		if (r.has<PointLightComponent>(e))       entity["components"]["pointlight"] = serialize<PointLightComponent>(r, e);
		if (r.has<Camera3DComponent>(e))         entity["components"]["camera"] = serialize<Camera3DComponent>(r, e);
		if (r.has<ArcballCameraComponent>(e))    entity["components"]["arcball"] = serialize<ArcballCameraComponent>(r, e);
		std::string id = std::to_string((entt::id_type)e);
		json["entities"][id] = entity;
		});

	if (!File::writeString(path, json.dump()))
	{
		Logger::error("Failed to write scene.");
	}
}

void Scene::load(World& world, const Path& path)
{
	std::string s = File::readString(path);
	nlohmann::json json = nlohmann::json::parse(s);
	std::string version = json["asset"]["version"].get<std::string>();
	if (version != std::to_string(major) + "." + std::to_string(minor))
	{
		Logger::error("Unsupported version");
		return;
	}
	// --- Entities
	std::unordered_map<uint32_t, entt::entity> entityMap;
	for (auto& jsonEntityKV : json["entities"].items())
	{
		uint32_t entityID = std::stoi(jsonEntityKV.key());
		nlohmann::json& jsonEntity = jsonEntityKV.value();
		// TODO enforce tag component
		entt::entity entity = world.registry().create();
		entityMap.insert(std::make_pair(entityID, entity));
		for (auto& componentKV : jsonEntity["components"].items())
		{
			std::string name = componentKV.key();
			nlohmann::json& component = componentKV.value();
			if (name == "tag")
			{
				world.registry().emplace<TagComponent>(entity);
				world.registry().get<TagComponent>(entity).name = component["name"].get<std::string>();
			}
			else if (name == "hierarchy")
			{
				world.registry().emplace<Hierarchy3DComponent>(entity);
				Hierarchy3DComponent& h = world.registry().get<Hierarchy3DComponent>(entity);
				nlohmann::json& jsonParent = component["parent"];
				if (jsonParent.is_null())
					h.parent = Entity::null();
				else
					h.parent = Entity(entityMap.find(component["parent"].get<uint32_t>())->second, &world);				
				h.inverseTransform = mat4f::identity();
			}
			else if (name == "transform")
			{
				world.registry().emplace<Transform3DComponent>(entity);
				world.registry().get<Transform3DComponent>(entity).transform = mat4f::identity();
			}
			else if (name == "mesh")
			{
				world.registry().emplace<MeshComponent>(entity);
				MeshComponent& mesh = world.registry().get<MeshComponent>(entity);
				mesh.submesh.mesh = ResourceManager::get<Mesh>(component["mesh"].get<std::string>());
				mesh.submesh.offset = 0;
				mesh.submesh.count = mesh.submesh.mesh->getIndexCount();
				mesh.submesh.type = PrimitiveType::Triangles;
				mesh.bounds.min = point3f(
					component["bounds"]["min"][0].get<float>(),
					component["bounds"]["min"][1].get<float>(),
					component["bounds"]["min"][2].get<float>()
				);
				mesh.bounds.max = point3f(
					component["bounds"]["max"][0].get<float>(),
					component["bounds"]["max"][1].get<float>(),
					component["bounds"]["max"][2].get<float>()
				);
			}
			else if (name == "material")
			{
				world.registry().emplace<MaterialComponent>(entity);
				MaterialComponent& material = world.registry().get<MaterialComponent>(entity);
				uint8_t data[] = { 255, 255, 255, 255 };
				uint8_t dataNormal[] = { 128, 128, 255, 255 };
				material.color = color4f(
					component["color"][0].get<float>(), 
					component["color"][1].get<float>(), 
					component["color"][2].get<float>(), 
					component["color"][3].get<float>()
				);
				material.doubleSided = component["doublesided"].get<bool>();
				material.colorTexture = ResourceManager::get<Texture>(component["textures"]["color"].get<std::string>());
				material.normalTexture = ResourceManager::get<Texture>(component["textures"]["normal"].get<std::string>());
				material.materialTexture = ResourceManager::get<Texture>(component["textures"]["material"].get<std::string>());
			}
			else if (name == "pointlight")
			{
				world.registry().emplace<PointLightComponent>(entity);
				PointLightComponent& light = world.registry().get<PointLightComponent>(entity);
				light.color = color3f(1);
				light.radius = 1.f;
				light.intensity = 1.f;
				light.shadowMap = Texture::createCubemap(1024, 1024, TextureFormat::Depth, TextureFlag::RenderTarget, Sampler{});
			}
			else if (name == "dirlight")
			{
				world.registry().emplace<DirectionalLightComponent>(entity);
				DirectionalLightComponent& light = world.registry().get<DirectionalLightComponent>(entity);
				light.color = color3f(1);
				light.direction = vec3f::normalize(vec3f(1.f));
				light.intensity = 1.f;
				light.shadowMap[0] = Texture::create2D(1024, 1024, TextureFormat::Depth, TextureFlag::RenderTarget, Sampler{});
				light.shadowMap[1] = Texture::create2D(1024, 1024, TextureFormat::Depth, TextureFlag::RenderTarget, Sampler{});
				light.shadowMap[2] = Texture::create2D(1024, 1024, TextureFormat::Depth, TextureFlag::RenderTarget, Sampler{});
			}
			else if (name == "camera")
			{
				world.registry().emplace<Camera3DComponent>(entity);
				Camera3DComponent& camera = world.registry().get<Camera3DComponent>(entity);
				CameraPerspective* p = new CameraPerspective;
				p->nearZ = 0.01f;
				p->farZ = 100.f;
				p->hFov = anglef::degree(60.f);
				p->ratio = 16.f / 9.f;
				camera.projection = p; // leak
				camera.view; // auto set
			}
			else if (name == "arcball")
			{
				world.registry().emplace<ArcballCameraComponent>(entity);
				ArcballCameraComponent& camera = world.registry().get<ArcballCameraComponent>(entity);
				camera.position = point3f(1.f);
				camera.target = point3f(0.f);
				camera.up = norm3f(0.f, 1.f, 0.f);
				camera.active = false;
				camera.speed = 1.f;
			}
		}
		
	}
}

};