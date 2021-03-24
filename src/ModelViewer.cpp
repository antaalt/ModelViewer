#include "ModelViewer.h"

namespace viewer {

void Viewer::loadShader()
{
#if defined(AKA_USE_OPENGL)
	
	aka::ShaderID vert = aka::Shader::compile(File::readString(Asset::path("shaders/GL/gltf.vert")), aka::ShaderType::Vertex);
	aka::ShaderID frag = aka::Shader::compile(File::readString(Asset::path("shaders/GL/gltf.frag")), aka::ShaderType::Fragment);
	std::vector<aka::Attributes> attributes;
#else
	std::string str = TextFile::load(Asset::path("shaders/D3D/shader.hlsl"));
	aka::ShaderID vert = aka::Shader::compile(str, aka::ShaderType::Vertex);
	aka::ShaderID frag = aka::Shader::compile(str, aka::ShaderType::Fragment);
	std::vector<aka::Attributes> attributes;
	attributes.push_back(aka::Attributes{ aka::AttributeID(0), "POS"});
	attributes.push_back(aka::Attributes{ aka::AttributeID(0), "NORM" });
	attributes.push_back(aka::Attributes{ aka::AttributeID(0), "TEX" });
	attributes.push_back(aka::Attributes{ aka::AttributeID(0), "COL" });
#endif
	if (vert == ShaderID(0) || frag == ShaderID(0))
	{
		aka::Logger::error("Failed to compile shader");
	}
	else
	{
		aka::Shader::Ptr shader = aka::Shader::create(vert, frag, aka::ShaderID(0), attributes);
		if (shader->valid())
		{
			m_shader = shader;
			m_material = aka::ShaderMaterial::create(shader);
		}
	}
}

void Viewer::onCreate()
{
	StopWatch<> stopWatch;
	// TODO use args
	//m_model = ModelLoader::load(Asset::path("glTF-Sample-Models/2.0/Sponza/glTF/Sponza.gltf"), point3f(0.f), 1.f);
	//m_model = ModelLoader::load(Asset::path("glTF-Sample-Models/2.0/AlphaBlendModeTest/glTF/AlphaBlendModeTest.gltf"));
	//m_model = ModelLoader::load(Asset::path("glTF-Sample-Models/2.0/CesiumMilkTruck/glTF/CesiumMilkTruck.gltf"));
	m_model = ModelLoader::load(Asset::path("glTF-Sample-Models/2.0/Lantern/glTF/Lantern.gltf"), point3f(0.f), 1.f);
	//m_model = ModelLoader::load(Asset::path("Sponza/Sponza.obj"));
	//m_model = ModelLoader::load(Asset::path("Dragon/dragon.obj"), point3f(0.f), 1.f);
	if (m_model == nullptr)
		throw std::runtime_error("Could not load model.");
	aka::Logger::info("Model loaded : ", stopWatch.elapsed(), "ms");
	aka::Logger::info("Scene Bounding box : ", m_model->bbox.min, " - ", m_model->bbox.max);

	loadShader();

	Framebuffer::AttachmentType attachments[] = {
		Framebuffer::AttachmentType::Depth
	};
	m_shadowFramebuffer = Framebuffer::create(1024, 1024, attachments, sizeof(attachments) / sizeof(Framebuffer::AttachmentType), Sampler{});

	m_lightDir = vec3f(0.1f, 1.f, 0.1f);

	m_camera.set(m_model->bbox);
}

void Viewer::onDestroy()
{
}

void Viewer::onUpdate(aka::Time::Unit deltaTime)
{
	using namespace aka;

	// Arcball
	m_camera.update(deltaTime);

	// TOD
	if (Mouse::pressed(MouseButton::ButtonMiddle))
	{
		const Position& pos = Mouse::position();
		float x = pos.x / (float)GraphicBackend::backbuffer()->width();
		m_lightDir = vec3f::normalize(lerp(vec3f(1, 1, 1), vec3f(-1, 1, -1), x));
	}

	// Reset
	if (Keyboard::down(KeyboardKey::R))
	{
		m_camera.set(m_model->bbox);
	}
	// Hot reload
	if (Keyboard::down(KeyboardKey::Space))
	{
		Logger::info("Reloading shaders...");
		loadShader();
	}
	// Quit the app if requested
	if (Keyboard::pressed(KeyboardKey::Escape))
	{
		EventDispatcher<QuitEvent>::emit();
	}
}

/*
	FRONT     BACK
	2______3__4______5
	|      |  |      |
	|      |  |      |
	|      |  |      |
	0______1__6______7
*/
Mesh::Ptr cube(
	const point3f& p0,
	const point3f& p1,
	const point3f& p2,
	const point3f& p3,
	const point3f& p4,
	const point3f& p5,
	const point3f& p6,
	const point3f& p7,
	const color4f& color
)
{
	std::vector<Vertex> vertices;
	std::vector<uint8_t> indices;

	uv2f u[4] = { uv2f(0.f, 0.f), uv2f(1.f, 0.f), uv2f(0.f, 1.f), uv2f(1.f, 1.f) };
	norm3f normal;
	uint8_t offset = 0;
	// Face 1
	normal = norm3f(vec3f::normalize(vec3f::cross(p1 - p0, p2 - p0)));
	vertices.push_back(Vertex{ p0, normal, u[0], color });
	vertices.push_back(Vertex{ p1, normal, u[1], color });
	vertices.push_back(Vertex{ p2, normal, u[2], color });
	vertices.push_back(Vertex{ p3, normal, u[3], color });
	indices.push_back(offset + 0);
	indices.push_back(offset + 1);
	indices.push_back(offset + 2);
	indices.push_back(offset + 2);
	indices.push_back(offset + 1);
	indices.push_back(offset + 3);
	offset += 4;

	// Face 2
	normal = norm3f(vec3f::normalize(vec3f::cross(p3 - p2, p4 - p2)));
	vertices.push_back(Vertex{ p2, normal, u[0], color });
	vertices.push_back(Vertex{ p3, normal, u[1], color });
	vertices.push_back(Vertex{ p4, normal, u[2], color });
	vertices.push_back(Vertex{ p5, normal, u[3], color });
	indices.push_back(offset + 0);
	indices.push_back(offset + 1);
	indices.push_back(offset + 2);
	indices.push_back(offset + 2);
	indices.push_back(offset + 1);
	indices.push_back(offset + 3);
	offset += 4;

	// Face 3
	normal = norm3f(vec3f::normalize(vec3f::cross(p5 - p4, p6 - p4)));
	vertices.push_back(Vertex{ p4, normal, u[0], color });
	vertices.push_back(Vertex{ p5, normal, u[1], color });
	vertices.push_back(Vertex{ p6, normal, u[2], color });
	vertices.push_back(Vertex{ p7, normal, u[3], color });
	indices.push_back(offset + 0);
	indices.push_back(offset + 1);
	indices.push_back(offset + 2);
	indices.push_back(offset + 2);
	indices.push_back(offset + 1);
	indices.push_back(offset + 3);
	offset += 4;

	// Face 4
	normal = norm3f(vec3f::normalize(vec3f::cross(p7 - p6, p0 - p6)));
	vertices.push_back(Vertex{ p6, normal, u[0], color });
	vertices.push_back(Vertex{ p7, normal, u[1], color });
	vertices.push_back(Vertex{ p0, normal, u[2], color });
	vertices.push_back(Vertex{ p1, normal, u[3], color });
	indices.push_back(offset + 0);
	indices.push_back(offset + 1);
	indices.push_back(offset + 2);
	indices.push_back(offset + 2);
	indices.push_back(offset + 1);
	indices.push_back(offset + 3);
	offset += 4;

	// Face 5
	normal = norm3f(vec3f::normalize(vec3f::cross(p7 - p1, p3 - p1)));
	vertices.push_back(Vertex{ p1, normal, u[0], color });
	vertices.push_back(Vertex{ p7, normal, u[1], color });
	vertices.push_back(Vertex{ p3, normal, u[2], color });
	vertices.push_back(Vertex{ p5, normal, u[3], color });
	indices.push_back(offset + 0);
	indices.push_back(offset + 1);
	indices.push_back(offset + 2);
	indices.push_back(offset + 2);
	indices.push_back(offset + 1);
	indices.push_back(offset + 3);
	offset += 4;

	// Face 6
	normal = norm3f(vec3f::normalize(vec3f::cross(p0 - p6, p4 - p6)));
	vertices.push_back(Vertex{ p6, normal, u[0], color });
	vertices.push_back(Vertex{ p0, normal, u[1], color });
	vertices.push_back(Vertex{ p4, normal, u[2], color });
	vertices.push_back(Vertex{ p2, normal, u[3], color });
	indices.push_back(offset + 0);
	indices.push_back(offset + 1);
	indices.push_back(offset + 2);
	indices.push_back(offset + 2);
	indices.push_back(offset + 1);
	indices.push_back(offset + 3);
	offset += 4;

	VertexData data;
	data.attributes.push_back(VertexData::Attribute{ 0, VertexFormat::Float, VertexType::Vec3 });
	data.attributes.push_back(VertexData::Attribute{ 1, VertexFormat::Float, VertexType::Vec3 });
	data.attributes.push_back(VertexData::Attribute{ 2, VertexFormat::Float, VertexType::Vec2 });
	data.attributes.push_back(VertexData::Attribute{ 3, VertexFormat::Float, VertexType::Vec4 });

	Mesh::Ptr mesh = Mesh::create();
	mesh->vertices(data, vertices.data(), vertices.size());
	mesh->indices(IndexFormat::UnsignedByte, indices.data(), indices.size());
	return mesh;
}

Mesh::Ptr referential(const point3f& origin)
{
	std::vector<Vertex> vertices;

	vertices.push_back(Vertex{ origin + vec3f(0, 0, 0), norm3f(0, 1, 0), uv2f(0, 1), color4f(0,0,0,1) });
	vertices.push_back(Vertex{ origin + vec3f(1, 0, 0), norm3f(0, 1, 0), uv2f(0, 1), color4f(1,0,0,1) });
	vertices.push_back(Vertex{ origin + vec3f(0, 0, 0), norm3f(0, 1, 0), uv2f(0, 1), color4f(0,0,0,1) });
	vertices.push_back(Vertex{ origin + vec3f(0, 1, 0), norm3f(0, 1, 0), uv2f(0, 1), color4f(0,1,0,1) });
	vertices.push_back(Vertex{ origin + vec3f(0, 0, 0), norm3f(0, 1, 0), uv2f(0, 1), color4f(0,0,0,1) });
	vertices.push_back(Vertex{ origin + vec3f(0, 0, 1), norm3f(0, 1, 0), uv2f(0, 1), color4f(0,0,1,1) });

	VertexData data;
	data.attributes.push_back(VertexData::Attribute{ 0, VertexFormat::Float, VertexType::Vec3 });
	data.attributes.push_back(VertexData::Attribute{ 1, VertexFormat::Float, VertexType::Vec3 });
	data.attributes.push_back(VertexData::Attribute{ 2, VertexFormat::Float, VertexType::Vec2 });
	data.attributes.push_back(VertexData::Attribute{ 3, VertexFormat::Float, VertexType::Vec4 });

	Mesh::Ptr mesh = Mesh::create();
	mesh->vertices(data, vertices.data(), vertices.size());
	return mesh;
}

// Draw an aabbox
Mesh::Ptr cube(const aabbox<>& bbox, const color4f& color)
{
	return cube(
		point3f(bbox.min.x, bbox.min.y, bbox.max.z),
		point3f(bbox.max.x, bbox.min.y, bbox.max.z),
		point3f(bbox.min.x, bbox.max.y, bbox.max.z),
		point3f(bbox.max),
		point3f(bbox.min.x, bbox.max.y, bbox.min.z),
		point3f(bbox.max.x, bbox.max.y, bbox.min.z),
		point3f(bbox.min),
		point3f(bbox.max.x, bbox.min.y, bbox.min.z),
		color
	);
}
// Draw a mvp frustum
Mesh::Ptr cube(const mat4f& mvp, const color4f& color)
{
	mat4f mvpInverse = mat4f::inverse(mvp);
	float min = -1.f;
	float max = 1.f;
	return cube(
		mvpInverse.multiplyPoint(point3f(min, min, max)),
		mvpInverse.multiplyPoint(point3f(max, min, max)),
		mvpInverse.multiplyPoint(point3f(min, max, max)),
		mvpInverse.multiplyPoint(point3f(max, max, max)),
		mvpInverse.multiplyPoint(point3f(min, max, min)),
		mvpInverse.multiplyPoint(point3f(max, max, min)),
		mvpInverse.multiplyPoint(point3f(min, min, min)),
		mvpInverse.multiplyPoint(point3f(max, min, min)),
		color
	);
}

void Viewer::onRender()
{
	static aka::RenderPass renderPass{};
	static Culling doubleSide = Culling{ CullMode::None, CullOrder::CounterClockWise };
	static Culling singleSide = Culling{ CullMode::BackFace, CullOrder::CounterClockWise };
	static Blending blending = Blending::nonPremultiplied();
	static Depth depth = Depth{ DepthCompare::LessOrEqual, true };
	static Stencil stencil = Stencil::none();

	// Shadows
	// Compute the MVP matrix from the light's point of view
	// Get the bbox of the sphere containing the bbox. It is the ortho depth
	vec3f halfExtent = m_model->bbox.extent() / 2.f;
	float maxHalfExtent = max(halfExtent.x, max(halfExtent.y, halfExtent.z));
	aabbox<> squaredBbox(m_model->bbox.center() - vec3f(maxHalfExtent), m_model->bbox.center() + vec3f(maxHalfExtent));
	float radius = (squaredBbox.extent() / 2.f).norm();
	aabbox<> bbox(point3f(-radius), point3f(radius));
	// Compute transform.
	mat4f depthProjectionMatrix = mat4f::orthographic(bbox.min.y, bbox.max.y, bbox.min.x, bbox.max.x , bbox.min.z, bbox.max.z);
	mat4f depthViewMatrix = mat4f::inverse(mat4f::lookAt(squaredBbox.center(), squaredBbox.center() - m_lightDir, norm3f(0, 0, 1)));
	mat4f worldToDepthMatrix = depthProjectionMatrix * depthViewMatrix;
	mat4f projectionToTextureCoordinateMatrix(
		col4f(0.5, 0.0, 0.0, 0.0),
		col4f(0.0, 0.5, 0.0, 0.0),
		col4f(0.0, 0.0, 0.5, 0.0),
		col4f(0.5, 0.5, 0.5, 1.0)
	);
	mat4f worldToDepthTextureMatrix = projectionToTextureCoordinateMatrix * worldToDepthMatrix;

	if (m_shadowShader == nullptr)
	{
		const char* vertShader = ""
			"#version 330 core\n"
			"layout(location = 0) in vec3 a_position;\n"
			"uniform mat4 u_light;\n"
			"uniform mat4 u_model;\n"
			"void main() {\n"
			"	gl_Position = u_light * u_model * vec4(a_position, 1.0);\n"
			"}\n";
		const char* fragShader = ""
			"#version 330 core\n"
			"void main() {\n"
			"	//gl_FragDepth = gl_FragCoord.z;\n"
			"}\n";
		std::vector<Attributes> attributes;
		ShaderID vert = Shader::compile(vertShader, ShaderType::Vertex);
		ShaderID frag = Shader::compile(fragShader, ShaderType::Fragment);
		m_shadowShader = Shader::create(vert, frag, ShaderID(0), attributes);
		m_shadowMaterial = ShaderMaterial::create(m_shadowShader);
	}
	static RenderPass shadowPass;
	m_shadowFramebuffer->clear(color4f(1.f), 1.f, 0, ClearMask::Depth);
	for (size_t i = 0; i < m_model->nodes.size(); i++)
	{
		aka::mat4f model = m_model->nodes[i].transform;
		m_shadowMaterial->set<mat4f>("u_light", worldToDepthMatrix);
		m_shadowMaterial->set<mat4f>("u_model", model);
		shadowPass.framebuffer = m_shadowFramebuffer;
		shadowPass.mesh = m_model->nodes[i].mesh;
		shadowPass.primitive = PrimitiveType::Triangles;
		shadowPass.indexCount = m_model->nodes[i].mesh->getIndexCount(); // TODO set zero means all ?
		shadowPass.indexOffset = 0;
		shadowPass.material = m_shadowMaterial;
		shadowPass.clear = Clear{ ClearMask::None, color4f(1.f), 1.f, 0 };
		shadowPass.blend = Blending::none();
		shadowPass.cull = m_model->nodes[i].material.doubleSided ? doubleSide : singleSide;
		shadowPass.depth = Depth{ DepthCompare::LessOrEqual, true };
		shadowPass.stencil = Stencil::none();
		shadowPass.viewport = aka::Rect{ 0 };
		shadowPass.scissor = aka::Rect{ 0 };

		shadowPass.execute();
	}
	/*ImageHDR img(1024, 1024, 1);
	m_shadowFramebuffer->attachment(Framebuffer::AttachmentType::Depth)->download(img.bytes.data());
	img.save("shadow.hdr");*/

	aka::Framebuffer::Ptr backbuffer = aka::GraphicBackend::backbuffer();
	backbuffer->clear(color4f(0.f, 0.f, 0.f, 1.f), 1.f, 0, ClearMask::All);
	if (Keyboard::pressed(KeyboardKey::Space))
	{
		static Batch b;
		b.draw(mat3f::scale(vec2f(backbuffer->width(), backbuffer->height())), Batch::Rect(vec2f(0.f), vec2f(1.f), m_shadowFramebuffer->attachment(Framebuffer::AttachmentType::Depth), 0));
		b.render();
		b.clear();
	}
	else
	{
		aka::mat4f view = aka::mat4f::inverse(m_camera.transform());
		aka::mat4f perspective = aka::mat4f::perspective(aka::anglef::degree(90.f), (float)backbuffer->width() / (float)backbuffer->height(), 0.01f, 10000.f);
		for (size_t i = 0; i < m_model->nodes.size(); i++)
		{
			aka::mat4f model = m_model->nodes[i].transform;
			aka::mat3f normal = aka::mat3f::transpose(aka::mat3f::inverse(mat3f(model)));
			aka::color4f color = m_model->nodes[i].material.color;
			m_material->set<mat4f>("u_projection", perspective);
			m_material->set<mat4f>("u_model", model);
			m_material->set<mat4f>("u_view", view);
			m_material->set<mat4f>("u_light", worldToDepthTextureMatrix);
			m_material->set<mat3f>("u_normalMatrix", normal);
			m_material->set<vec3f>("u_lightDir", m_lightDir);
			m_material->set<color4f>("u_color", color);
			m_material->set<Texture::Ptr>("u_colorTexture", m_model->nodes[i].material.colorTexture);
			m_material->set<Texture::Ptr>("u_normalTexture", m_model->nodes[i].material.normalTexture);
			m_material->set<Texture::Ptr>("u_shadowTexture", m_shadowFramebuffer->attachment(Framebuffer::AttachmentType::Depth));
			renderPass.framebuffer = backbuffer;
			renderPass.mesh = m_model->nodes[i].mesh;
			renderPass.primitive = PrimitiveType::Triangles;
			renderPass.indexCount = m_model->nodes[i].mesh->getIndexCount(); // TODO set zero means all ?
			renderPass.indexOffset = 0;
			renderPass.material = m_material;
			renderPass.clear = Clear{ ClearMask::None, color4f(1.f), 1.f, 0 };
			renderPass.blend = blending;
			renderPass.cull = m_model->nodes[i].material.doubleSided ? doubleSide : singleSide;
			renderPass.depth = depth;
			renderPass.stencil = stencil;
			renderPass.viewport = aka::Rect{ 0 };
			renderPass.scissor = aka::Rect{ 0 };

			renderPass.execute();
		}
		//if (Keyboard::pressed(KeyboardKey::ControlLeft))
		{
			// BBOx debug
			static Mesh::Ptr boxMeshboxSphere, boxMeshSphere, boxMesh;
			static Mesh::Ptr meshShadow;
			static Mesh::Ptr origin, originbbox;
			static Shader::Ptr shader;
			static ShaderMaterial::Ptr material;
			aka::mat4f view = aka::mat4f::inverse(m_camera.transform());
			aka::mat4f model = mat4f::identity();
			aka::mat4f perspective = aka::mat4f::perspective(aka::anglef::degree(90.f), (float)backbuffer->width() / (float)backbuffer->height(), 0.01f, 10000.f);
			if (material == nullptr)
			{
				std::vector<Attributes> att;
				ShaderID vert = Shader::compile(File::readString(Asset::path("shaders/GL/draw.vert")).c_str(), ShaderType::Vertex);
				ShaderID frag = Shader::compile(File::readString(Asset::path("shaders/GL/draw.frag")).c_str(), ShaderType::Fragment);
				shader = Shader::create(vert, frag, ShaderID(0), att);
				material = ShaderMaterial::create(shader);
			}
			{
				boxMesh = cube(m_model->bbox, color4f(0.f, 0.f, 1.f, 0.1f));
				boxMeshSphere = cube(squaredBbox, color4f(0.f, 1.f, 0.f, 0.1f));
				boxMeshboxSphere = cube(bbox, color4f(1.f, 0.f, 0.f, 0.1f));
				meshShadow = cube(worldToDepthMatrix, color4f(1.f, 1.f, 1.f, 0.1f));
				origin = referential(point3f(0));
				originbbox = referential(bbox.center());
			}
			static RenderPass pass;
			aka::mat3f normal = aka::mat3f::transpose(aka::mat3f::inverse(mat3f(model)));
			material->set<mat4f>("u_projection", perspective);
			material->set<mat4f>("u_model", model);
			material->set<mat4f>("u_view", view);
			material->set<mat3f>("u_normalMatrix", normal);

			pass.framebuffer = backbuffer;
			pass.mesh = boxMesh;
			shadowPass.primitive = PrimitiveType::Triangles;
			pass.indexCount = boxMesh->getIndexCount();
			pass.indexOffset = 0;
			pass.material = material;
			pass.clear = Clear{ ClearMask::None, color4f(1.f), 1.f, 0 };
			pass.blend = blending;
			pass.cull = doubleSide;
			pass.depth = depth;
			pass.stencil = stencil;
			pass.viewport = aka::Rect{ 0 };
			pass.scissor = aka::Rect{ 0 };
			//pass.execute();

			pass.mesh = boxMeshSphere;
			pass.indexCount = boxMeshSphere->getIndexCount();
			pass.execute();

			pass.mesh = boxMeshboxSphere;
			pass.indexCount = boxMeshboxSphere->getIndexCount();
			pass.execute();

			pass.mesh = meshShadow;
			pass.indexCount = meshShadow->getIndexCount();
			pass.execute();

			pass.depth = Depth{ DepthCompare::None, false };
			pass.primitive = PrimitiveType::Lines;
			pass.mesh = origin;
			pass.indexCount = origin->getIndexCount();
			pass.execute();
			material->set<>("u_model", mat4f::translate(vec3f(bbox.center())));
			pass.execute();
			material->set<>("u_model", mat4f::inverse(depthViewMatrix));
			pass.execute();
			pass.depth = depth;
			pass.primitive = PrimitiveType::Triangles;
		}
	}
}

};
