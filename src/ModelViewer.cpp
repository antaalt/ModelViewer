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
	Sampler shadowSampler{};
	shadowSampler.filterMag = Sampler::Filter::Nearest;
	shadowSampler.filterMin = Sampler::Filter::Nearest;
	m_shadowFramebuffer = Framebuffer::create(1024, 1024, attachments, sizeof(attachments) / sizeof(Framebuffer::AttachmentType), shadowSampler);

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

// TODO move in geometry
struct Frustum {
	float left;
	float right;
	float bottom;
	float top;
	float near;
	float far;

	//bool contain(const mat4f& view, const point3f& point) const;
};

// --- Shadows
// TODO mix the two algorithms ? Or just dynamically adapt projection matrix
// Compute the shadow projection around the object.
mat4f computeShadowViewProjectionMatrix(const aabbox<>& bbox, const vec3f& lightDir)
{
	vec3f halfExtent = bbox.extent() / 2.f;
	float maxHalfExtent = max(halfExtent.x, max(halfExtent.y, halfExtent.z));
	aabbox<> squaredBbox(bbox.center() - vec3f(maxHalfExtent), bbox.center() + vec3f(maxHalfExtent));
	float radius = (squaredBbox.extent() / 2.f).norm();
	aabbox<> frustumBbox(point3f(-radius), point3f(radius));
	mat4f lightViewMatrix = mat4f::inverse(mat4f::lookAt(bbox.center(), bbox.center() - lightDir, norm3f(0, 0, 1)));
	mat4f lightProjectionMatrix = mat4f::orthographic(frustumBbox.min.y, frustumBbox.max.y, frustumBbox.min.x, frustumBbox.max.x, frustumBbox.min.z, frustumBbox.max.z);
	return lightProjectionMatrix * lightViewMatrix;
}

// Compute the shadow projection around the view projection.
mat4f computeShadowViewProjectionMatrix(const mat4f& view, const mat4f& projection, float near, float far, const vec3f& lightDirWorld)
{
	mat4f viewProjectionInverseMatrix = mat4f::inverse(projection * view);
#if defined(GEOMETRY_CLIP_SPACE_NEGATIVE)
	float min = -1.f; // Maybe zero still ?
#else
	float min = 0.f;
#endif
	float max = 1.f;
	// https://gamedev.stackexchange.com/questions/183196/calculating-directional-shadow-map-using-camera-frustum
	vec4f corners[8] = {
		viewProjectionInverseMatrix * vec4f(min, min, min, 1),
		viewProjectionInverseMatrix * vec4f(min, min, max, 1),
		viewProjectionInverseMatrix * vec4f(max, min, min, 1),
		viewProjectionInverseMatrix * vec4f(min, max, min, 1),
		viewProjectionInverseMatrix * vec4f(max, min, max, 1),
		viewProjectionInverseMatrix * vec4f(min, max, max, 1),
		viewProjectionInverseMatrix * vec4f(max, max, min, 1),
		viewProjectionInverseMatrix * vec4f(max, max, max, 1),
	};
	point3f centerWorld = point3f(0.f);
	for (size_t i = 0; i < 8; i++)
	{
		centerWorld.x += corners[i].x / corners[i].w;
		centerWorld.y += corners[i].y / corners[i].w;
		centerWorld.z += corners[i].z / corners[i].w;
	}
	centerWorld /= 8.f;
	//float distance = far - near;
	mat4f lightViewMatrix = mat4f::inverse(mat4f::lookAt(centerWorld, centerWorld - lightDirWorld, norm3f(0, 0, 1)));
	// Set to light space.
	aabbox<> bbox;
	for (size_t i = 0; i < 8; i++)
	{
		corners[i] = lightViewMatrix * corners[i];
		bbox.include(point3f(corners[i] / corners[i].w));
	}
	mat4f lightProjectionMatrix = mat4f::orthographic(bbox.min.y, bbox.max.y, bbox.min.x, bbox.max.x, bbox.min.z, bbox.max.z);
	return lightProjectionMatrix * lightViewMatrix;
}

void Viewer::onRender()
{
	static aka::RenderPass renderPass{};
	aka::Framebuffer::Ptr backbuffer = aka::GraphicBackend::backbuffer();
	aka::mat4f view = aka::mat4f::inverse(m_camera.transform());
	// TODO use camera (Arcball inherit camera ?
	float near = 0.01f;
	float far = 100.f;
	aka::mat4f perspective = aka::mat4f::perspective(aka::anglef::degree(90.f), (float)backbuffer->width() / (float)backbuffer->height(), near, far);

#if 0
	mat4f worldToDepthMatrix = computeShadowViewProjectionMatrix(m_model->bbox, m_lightDir);
#else
	mat4f worldToDepthMatrix = computeShadowViewProjectionMatrix(view, perspective, near, far, m_lightDir);
#endif
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
		shadowPass.cull = m_model->nodes[i].material.doubleSided ? Culling{ CullMode::None, CullOrder::CounterClockWise } : Culling{ CullMode::BackFace, CullOrder::CounterClockWise };
		shadowPass.depth = Depth{ DepthCompare::LessOrEqual, true };
		shadowPass.stencil = Stencil::none();
		shadowPass.viewport = aka::Rect{ 0 };
		shadowPass.scissor = aka::Rect{ 0 };

		shadowPass.execute();
	}

	backbuffer->clear(color4f(0.f, 0.f, 0.f, 1.f), 1.f, 0, ClearMask::All);
	if (Keyboard::pressed(KeyboardKey::Space))
	{
		Renderer2D::draw(mat3f::scale(vec2f(backbuffer->width(), backbuffer->height())), Batch2D::Rect(vec2f(0.f), vec2f(1.f), m_shadowFramebuffer->attachment(Framebuffer::AttachmentType::Depth), 0));
		Renderer2D::render();
		Renderer2D::clear();
	}
	else
	{
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
			renderPass.blend = Blending::nonPremultiplied();
			renderPass.cull = m_model->nodes[i].material.doubleSided ? Culling{ CullMode::None, CullOrder::CounterClockWise } : Culling{ CullMode::BackFace, CullOrder::CounterClockWise };
			renderPass.depth = Depth{ DepthCompare::LessOrEqual, true };
			renderPass.stencil = Stencil::none();
			renderPass.viewport = aka::Rect{ 0 };
			renderPass.scissor = aka::Rect{ 0 };

			renderPass.execute();
		}
		// Debug pass
		// TODO add pipeline (default pipeline, debug pipeline...)
		// Debug pipeline : create origin mesh. for every mesh in scene, draw bbox & origin as line.
		{
			Renderer3D::drawAxis(mat4f::identity());
			Renderer3D::drawAxis(mat4f::inverse(worldToDepthMatrix));
			Renderer3D::drawAxis(mat4f::translate(vec3f(m_model->bbox.center())));
			Renderer3D::drawFrustum(worldToDepthMatrix);
			Renderer3D::drawTransform(mat4f::translate(vec3f(m_model->bbox.center())) * mat4f::scale(m_model->bbox.extent() / 2.f));
			Renderer3D::render(GraphicBackend::backbuffer(), view, perspective);
			Renderer3D::clear();
		}
	}
}

};
