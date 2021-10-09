#include "Model.h"

// TODO move json serialization within aka. 
#include "json.hpp"

namespace app {

Entity Scene::getMainCamera(World& world)
{
	Entity cameraEntity = Entity::null();
	auto view = world.registry().view<Transform3DComponent, Camera3DComponent>();
	for (entt::entity entity : view)
	{
		if (world.registry().has<Camera3DComponent>(entity))
		{
			cameraEntity = Entity(entity, &world);
			break;
		}
	}
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
	Texture2D::Ptr blank = Texture2D::create(1, 1, TextureFormat::RGBA8, TextureFlag::None, data);
	uint8_t n[4]{ 128, 128, 255, 255 };
	Texture2D::Ptr normal = Texture2D::create(1, 1, TextureFormat::RGBA8, TextureFlag::None, n);
	TextureSampler s = TextureSampler::nearest;

	mat4f id = mat4f::identity();
	Entity mesh = world.createEntity("New uv sphere");
	mesh.add<Transform3DComponent>(Transform3DComponent{ id });
	mesh.add<Hierarchy3DComponent>(Hierarchy3DComponent{ Entity::null(), id });
	mesh.add<MeshComponent>(MeshComponent{ SubMesh{ m, PrimitiveType::Triangles, (uint32_t)m->getIndexCount(), 0 }, aabbox<>(point3f(-1), point3f(1)) });
	mesh.add<MaterialComponent>(MaterialComponent{ color4f(1.f), true, {blank, s}, {normal, s}, {blank, s} });
	return mesh;
}

Entity Scene::createCubeEntity(World& world)
{
	uint8_t colorData[4]{ 255, 255, 255, 255 };
	Texture2D::Ptr blank = Texture2D::create(1, 1, TextureFormat::RGBA8, TextureFlag::None, colorData);
	uint8_t normalData[4]{ 128, 128, 255, 255 };
	Texture2D::Ptr normal = Texture2D::create(1, 1, TextureFormat::RGBA8, TextureFlag::None, normalData);
	TextureSampler s = TextureSampler::nearest;

	Mesh::Ptr m = createCubeMesh(point3f(0.f), 1.f);
	mat4f id = mat4f::identity();
	Entity mesh = world.createEntity("New cube");
	mesh.add<Transform3DComponent>(Transform3DComponent{ id });
	mesh.add<Hierarchy3DComponent>(Hierarchy3DComponent{ Entity::null(), id });
	mesh.add<MeshComponent>(MeshComponent{ SubMesh{ m, PrimitiveType::Triangles, (uint32_t)m->getVertexCount(0), 0 }, aabbox<>(point3f(-1), point3f(1)) });
	mesh.add<MaterialComponent>(MaterialComponent{ color4f(1.f), true, {blank, s}, {normal, s}, {blank, s} });
	return mesh;
}

Entity Scene::createPointLightEntity(World& world)
{
	mat4f id = mat4f::identity();
	Entity light = world.createEntity("New point light");
	light.add<Transform3DComponent>(Transform3DComponent{ id });
	light.add<Hierarchy3DComponent>(Hierarchy3DComponent{ Entity::null(), id });
	light.add<PointLightComponent>(PointLightComponent{
		color3f(1.f),
		1.f,
		{ id, id, id, id, id, id },
		nullptr
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
			nullptr,
			nullptr,
			nullptr
		},
		{ 1.f, 1.f, 1.f }
	});
	return light;
}

Entity Scene::createArcballCameraEntity(World& world)
{
	mat4f id = mat4f::identity();

	auto perspective = std::make_unique<CameraPerspective>();
	perspective->hFov = anglef::degree(60.f);
	perspective->nearZ = 0.1f;
	perspective->farZ = 100.f;
	perspective->ratio = 1.f;

	auto arcball = std::make_unique<CameraArcball>();
	arcball->set(aabbox<>(point3f(0.f), point3f(1.f)));

	Entity camera = world.createEntity("New arcball camera");
	camera.add<Transform3DComponent>(Transform3DComponent{ id });
	camera.add<Hierarchy3DComponent>(Hierarchy3DComponent{ Entity::null(), id });
	camera.add<Camera3DComponent>(Camera3DComponent{
		id,
		std::move(perspective),
		std::move(arcball),
		false
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
	ResourceManager* resource = Application::resource();
	const MeshComponent& m = r.get<MeshComponent>(e);
	nlohmann::json json = nlohmann::json::object();
	json["bounds"]["min"] = { m.bounds.min.x, m.bounds.min.y, m.bounds.min.z };
	json["bounds"]["max"] = { m.bounds.max.x, m.bounds.max.y, m.bounds.max.z };
	json["mesh"] = resource->name<Mesh>(m.submesh.mesh).cstr();
	return json;
}
template <>
nlohmann::json serialize<MaterialComponent>(const entt::registry& r, entt::entity e)
{
	ResourceManager* resource = Application::resource();
	const MaterialComponent& m = r.get<MaterialComponent>(e);
	nlohmann::json json = nlohmann::json::object();
	json["color"] = { m.color.r, m.color.g, m.color.b, m.color.a };
	json["doublesided"] = m.doubleSided;

	json["albedo"]["texture"] = resource->name<Texture>(m.albedo.texture).cstr();
	json["albedo"]["sampler"]["anisotropy"] = m.albedo.sampler.anisotropy;
	json["albedo"]["sampler"]["filterMin"] = m.albedo.sampler.filterMin;
	json["albedo"]["sampler"]["filterMag"] = m.albedo.sampler.filterMag;
	json["albedo"]["sampler"]["wrapU"] = m.albedo.sampler.wrapU;
	json["albedo"]["sampler"]["wrapV"] = m.albedo.sampler.wrapV;
	json["albedo"]["sampler"]["wrapW"] = m.albedo.sampler.wrapW;
	json["albedo"]["sampler"]["mipmapMode"] = m.albedo.sampler.mipmapMode;

	json["normal"]["texture"] = resource->name<Texture>(m.normal.texture).cstr();
	json["normal"]["sampler"]["anisotropy"] = m.normal.sampler.anisotropy;
	json["normal"]["sampler"]["filterMin"] = m.normal.sampler.filterMin;
	json["normal"]["sampler"]["filterMag"] = m.normal.sampler.filterMag;
	json["normal"]["sampler"]["wrapU"] = m.normal.sampler.wrapU;
	json["normal"]["sampler"]["wrapV"] = m.normal.sampler.wrapV;
	json["normal"]["sampler"]["wrapW"] = m.normal.sampler.wrapW;
	json["normal"]["sampler"]["mipmapMode"] = m.normal.sampler.mipmapMode;

	json["material"]["texture"] = resource->name<Texture>(m.material.texture).cstr();
	json["material"]["sampler"]["anisotropy"] = m.material.sampler.anisotropy;
	json["material"]["sampler"]["filterMin"] = m.material.sampler.filterMin;
	json["material"]["sampler"]["filterMag"] = m.material.sampler.filterMag;
	json["material"]["sampler"]["wrapU"] = m.material.sampler.wrapU;
	json["material"]["sampler"]["wrapV"] = m.material.sampler.wrapV;
	json["material"]["sampler"]["wrapW"] = m.material.sampler.wrapW;
	json["material"]["sampler"]["mipmapMode"] = m.material.sampler.mipmapMode;
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
	//json["view"] = c.view;
	json["active"] = c.active;
	CameraPerspective* perspective = dynamic_cast<CameraPerspective*>(c.projection.get());
	CameraOrthographic* orthographic = dynamic_cast<CameraOrthographic*>(c.projection.get());
	if (perspective != nullptr)
	{
		json["perspective"]["near"] = perspective->nearZ;
		json["perspective"]["far"] = perspective->farZ;
		json["perspective"]["ratio"] = perspective->ratio;
		json["perspective"]["fov"] = perspective->hFov.degree();
	}
	else if (orthographic != nullptr)
	{
		json["orthographic"]["left"] = orthographic->left;
		json["orthographic"]["right"] = orthographic->right;
		json["orthographic"]["bottom"] = orthographic->bottom;
		json["orthographic"]["top"] = orthographic->top;
		json["orthographic"]["near"] = orthographic->nearZ;
		json["orthographic"]["far"] = orthographic->farZ;
	}
	// Controller
	CameraArcball* arcball = dynamic_cast<CameraArcball*>(c.controller.get());
	if (arcball != nullptr)
	{
		json["arcball"]["position"] = { arcball->position.x, arcball->position.y, arcball->position.z };
		json["arcball"]["target"] = { arcball->target.x, arcball->target.y, arcball->target.z };
		json["arcball"]["up"] = { arcball->up.x, arcball->up.y, arcball->up.z };
		json["arcball"]["speed"] = arcball->speed;
	}
	return json;
}
template <>
nlohmann::json serialize<TextComponent>(const entt::registry& r, entt::entity e)
{
	ResourceManager* resource = Application::resource();
	const TextComponent& t = r.get<TextComponent>(e);
	nlohmann::json json = nlohmann::json::object();
	json["font"] = resource->name<Font>(t.font).cstr();
	json["text"] = t.text.cstr();
	json["color"] = { t.color.r, t.color.g, t.color.b, t.color.a };
	json["sampler"]["anisotropy"] = t.sampler.anisotropy;
	json["sampler"]["filterMin"] = t.sampler.filterMin;
	json["sampler"]["filterMag"] = t.sampler.filterMag;
	json["sampler"]["wrapU"] = t.sampler.wrapU;
	json["sampler"]["wrapV"] = t.sampler.wrapV;
	json["sampler"]["wrapW"] = t.sampler.wrapW;
	json["sampler"]["mipmapMode"] = t.sampler.mipmapMode;
	return json;
}

static uint16_t major = 0;
static uint16_t minor = 2;

void Scene::save(const Path& path, const World& world)
{
	try
	{
		const entt::registry& r = world.registry();
		nlohmann::json json = nlohmann::json::object();
		// --- Version
		json["asset"] = nlohmann::json::object();
		json["asset"]["version"] = std::to_string(major) + "." + std::to_string(minor);
		json["asset"]["exporter"] = "aka engine";
#if defined(GEOMETRY_LEFT_HANDED)
		json["asset"]["coordinate"] = "left";
#elif defined(GEOMETRY_RIGHT_HANDED)
		json["asset"]["coordinate"] = "right";
#else
		json["asset"]["coordinate"] = "unknown";
#endif
#if defined(AKA_ORIGIN_TOP_LEFT)
		json["asset"]["origin"] = "top";
#elif defined(AKA_ORIGIN_BOTTOM_LEFT)
		json["asset"]["origin"] = "bottom";
#else
		json["asset"]["origin"] = "unknown";
#endif
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
			std::string id = std::to_string((entt::id_type)e);
			json["entities"][id] = entity;
		});

		if (!OS::File::write(path, json.dump()))
		{
			Logger::error("Failed to write scene.");
		}
	}
	catch (const nlohmann::json::exception& e)
	{
		Logger::error("Failed to write JSON : ", e.what());
	}
}

void Scene::load(World& world, const Path& path)
{
	try
	{
		ResourceManager* resource = Application::resource();
		String s;
		OS::File::read(path, &s);
		if (s.length() == 0)
		{
			Logger::error("File ", path, "not valid.");
			return;
		}
		nlohmann::json json = nlohmann::json::parse(s.cstr());
		std::string version = json["asset"]["version"].get<std::string>();
		if (version != std::to_string(major) + "." + std::to_string(minor))
		{
			Logger::error("Unsupported version : ", version);
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
					AKA_ASSERT(component["matrix"].size() == 16, "Invalid matrix");
					world.registry().get<Transform3DComponent>(entity).transform = mat4f(
						col4f(component["matrix"][0], component["matrix"][1], component["matrix"][2], component["matrix"][3]),
						col4f(component["matrix"][4], component["matrix"][5], component["matrix"][6], component["matrix"][7]),
						col4f(component["matrix"][8], component["matrix"][9], component["matrix"][10], component["matrix"][11]),
						col4f(component["matrix"][12], component["matrix"][13], component["matrix"][14], component["matrix"][15])
					);
				}
				else if (name == "mesh")
				{
					world.registry().emplace<MeshComponent>(entity);
					MeshComponent& mesh = world.registry().get<MeshComponent>(entity);
					mesh.submesh.mesh = resource->get<Mesh>(component["mesh"].get<std::string>());
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
					material.color = color4f(
						component["color"][0].get<float>(), 
						component["color"][1].get<float>(), 
						component["color"][2].get<float>(), 
						component["color"][3].get<float>()
					);
					material.doubleSided = component["doublesided"].get<bool>();
					material.albedo.texture = resource->get<Texture>(component["albedo"]["texture"].get<std::string>());
					material.albedo.sampler.anisotropy = component["albedo"]["sampler"]["anisotropy"].get<float>();
					material.albedo.sampler.wrapU = (TextureWrap)component["albedo"]["sampler"]["wrapU"].get<int>();
					material.albedo.sampler.wrapV = (TextureWrap)component["albedo"]["sampler"]["wrapV"].get<int>();
					material.albedo.sampler.wrapW = (TextureWrap)component["albedo"]["sampler"]["wrapW"].get<int>();
					material.albedo.sampler.filterMin = (TextureFilter)component["albedo"]["sampler"]["filterMin"].get<int>();
					material.albedo.sampler.filterMag = (TextureFilter)component["albedo"]["sampler"]["filterMag"].get<int>();
					material.albedo.sampler.mipmapMode = (TextureMipMapMode)component["albedo"]["sampler"]["mipmapMode"].get<int>();

					material.normal.texture = resource->get<Texture>(component["normal"]["texture"].get<std::string>());
					material.normal.sampler.anisotropy = component["normal"]["sampler"]["anisotropy"].get<float>();
					material.normal.sampler.wrapU = (TextureWrap)component["normal"]["sampler"]["wrapU"].get<int>();
					material.normal.sampler.wrapV = (TextureWrap)component["normal"]["sampler"]["wrapV"].get<int>();
					material.normal.sampler.wrapW = (TextureWrap)component["normal"]["sampler"]["wrapW"].get<int>();
					material.normal.sampler.filterMin = (TextureFilter)component["normal"]["sampler"]["filterMin"].get<int>();
					material.normal.sampler.filterMag = (TextureFilter)component["normal"]["sampler"]["filterMag"].get<int>();
					material.normal.sampler.mipmapMode = (TextureMipMapMode)component["normal"]["sampler"]["mipmapMode"].get<int>();

					material.material.texture = resource->get<Texture>(component["material"]["texture"].get<std::string>());
					material.material.sampler.anisotropy = component["material"]["sampler"]["anisotropy"].get<float>();
					material.material.sampler.wrapU = (TextureWrap)component["material"]["sampler"]["wrapU"].get<int>();
					material.material.sampler.wrapV = (TextureWrap)component["material"]["sampler"]["wrapV"].get<int>();
					material.material.sampler.wrapW = (TextureWrap)component["material"]["sampler"]["wrapW"].get<int>();
					material.material.sampler.filterMin = (TextureFilter)component["material"]["sampler"]["filterMin"].get<int>();
					material.material.sampler.filterMag = (TextureFilter)component["material"]["sampler"]["filterMag"].get<int>();
					material.material.sampler.mipmapMode = (TextureMipMapMode)component["material"]["sampler"]["mipmapMode"].get<int>();
				}
				else if (name == "pointlight")
				{
					world.registry().emplace<PointLightComponent>(entity);
					PointLightComponent& light = world.registry().get<PointLightComponent>(entity);
					light.color = color3f(component["color"][0].get<float>(), component["color"][1].get<float>(), component["color"][2].get<float>());
					light.intensity = component["intensity"];
					light.radius = 1.f;
				}
				else if (name == "dirlight")
				{
					world.registry().emplace<DirectionalLightComponent>(entity);
					DirectionalLightComponent& light = world.registry().get<DirectionalLightComponent>(entity);
					light.color = color3f(component["color"][0].get<float>(), component["color"][1].get<float>(), component["color"][2].get<float>());
					light.direction = vec3f(component["direction"][0].get<float>(), component["direction"][1].get<float>(), component["direction"][2].get<float>());
					light.intensity = component["intensity"];
				}
				else if (name == "camera")
				{
					world.registry().emplace<Camera3DComponent>(entity);
					Camera3DComponent& camera = world.registry().get<Camera3DComponent>(entity);
					// projection
					if (component.find("perspective") != component.end())
					{
						auto persp = std::make_unique<CameraPerspective>();
						persp->hFov = anglef::degree(component["perspective"]["fov"].get<float>());
						persp->nearZ = component["perspective"]["near"].get<float>();
						persp->farZ = component["perspective"]["far"].get<float>();
						persp->ratio = component["perspective"]["ratio"].get<float>();
						camera.projection = std::move(persp);
					}
					else if (component.find("orthographic") != component.end())
					{
						auto ortho = std::make_unique<CameraOrthographic>();
						ortho->left = component["orthographic"]["left"].get<float>();
						ortho->right = component["orthographic"]["right"].get<float>();
						ortho->bottom = component["orthographic"]["bottom"].get<float>();
						ortho->top = component["orthographic"]["top"].get<float>();
						ortho->nearZ = component["orthographic"]["near"].get<float>();
						ortho->farZ = component["orthographic"]["far"].get<float>();
						camera.projection = std::move(ortho);
					}
					else
					{
						Logger::warn("No projection found for camera. Default to perspective");
						auto persp = std::make_unique<CameraPerspective>();
						persp->hFov = anglef::degree(60.f);
						persp->nearZ = 0.1f;
						persp->farZ = 100.f;
						persp->ratio = 1.f;
						camera.projection = std::move(persp);
					}
					// controller
					if (component.find("arcball") != component.end())
					{
						nlohmann::json& a = component["arcball"];
						auto arcball = std::make_unique<CameraArcball>();
						arcball->position = point3f(a["position"][0].get<float>(), a["position"][1].get<float>(), a["position"][2].get<float>());
						arcball->target = point3f(a["target"][0].get<float>(), a["target"][1].get<float>(), a["target"][2].get<float>());
						arcball->up = norm3f(a["up"][0].get<float>(), a["up"][1].get<float>(), a["up"][2].get<float>());
						arcball->speed = a["speed"].get<float>();
						camera.controller = std::move(arcball);
					}
					else 
					{
						Logger::warn("No controller found for camera. Default to arcball.");
						auto arcball = std::make_unique<CameraArcball>();
						arcball->position = point3f(1.f);
						arcball->target = point3f(0.f);
						arcball->up = norm3f(0.f, 1.f, 0.f);
						arcball->speed = 1.f;
						camera.controller = std::move(arcball);
					}
					camera.view; // auto set
				}
				else if (name == "text")
				{
					world.registry().emplace<TextComponent>(entity);
					TextComponent& text = world.registry().get<TextComponent>(entity);
					text.font = resource->get<Font>(component["font"].get<std::string>());
					text.color = color4f(
						component["color"][0].get<float>(),
						component["color"][1].get<float>(),
						component["color"][2].get<float>(),
						component["color"][3].get<float>()
					);
					text.sampler.anisotropy = component["sampler"]["anisotropy"].get<float>();
					text.sampler.wrapU = (TextureWrap)component["sampler"]["wrapU"].get<int>();
					text.sampler.wrapV = (TextureWrap)component["sampler"]["wrapV"].get<int>();
					text.sampler.wrapW = (TextureWrap)component["sampler"]["wrapW"].get<int>();
					text.sampler.filterMin = (TextureFilter)component["sampler"]["filterMin"].get<int>();
					text.sampler.filterMag = (TextureFilter)component["sampler"]["filterMag"].get<int>();
					text.sampler.mipmapMode = (TextureMipMapMode)component["sampler"]["mipmapMode"].get<int>();
					text.text = component["text"].get<std::string>();
				}
			}
		}
	}
	catch (const nlohmann::json::exception& e)
	{
		Logger::error("Failed to write JSON : ", e.what());
	}
}

};