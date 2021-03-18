#include "ModelViewer.h"

#include "Model/GLTF/GLTFLoader.h"
#include "Model/OBJ/OBJLoader.h"

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
	GLTFLoader gltfLloader;
	OBJLoader objLoader;
	// TODO use args
	//m_model = gltfLloader.load(Asset::path("glTF-Sample-Models/2.0/Sponza/glTF/Sponza.gltf"));
	//m_model = gltfLloader.load(Asset::path("glTF-Sample-Models/2.0/AlphaBlendModeTest/glTF/AlphaBlendModeTest.gltf"));
	//m_model = gltfLloader.load(Asset::path("glTF-Sample-Models/2.0/CesiumMilkTruck/glTF/CesiumMilkTruck.gltf"));
	//m_model = gltfLloader.load(Asset::path("glTF-Sample-Models/2.0/Lantern/glTF/Lantern.gltf"));
	m_model = objLoader.load(Asset::path("Sponza/Sponza.obj"));
	if (m_model == nullptr)
		throw std::runtime_error("Could not load model.");
	aka::Logger::info("Scene Bounding box : ", m_model->bbox.min, " - ", m_model->bbox.max);

	loadShader();

	Framebuffer::AttachmentType attachments[] = {
		Framebuffer::AttachmentType::Depth
	};
	m_shadowFramebuffer = Framebuffer::create(1024, 1024, attachments, sizeof(attachments) / sizeof(Framebuffer::AttachmentType), Sampler{});

	m_lightDir = vec3f(0.1f, 1.f, 0.1f);

	m_camera.set(m_model->bbox);
	//aka::Logger::info("Camera : ", m_camera.transform);
}

void Viewer::onDestroy()
{
}

void Viewer::onUpdate(aka::Time::Unit deltaTime)
{
	using namespace aka;

	// Arcball
	{
		// https://gamedev.stackexchange.com/questions/53333/how-to-implement-a-basic-arcball-camera-in-opengl-with-glm
		if (input::pressed(input::Button::Button1))
		{
			const input::Position& delta = input::delta();
			radianf pitch = radianf(-delta.y * deltaTime.seconds());
			radianf yaw = radianf(-delta.x * deltaTime.seconds());
			m_camera.position = mat4f::rotate(vec3f(1, 0, 0), pitch).multiplyPoint(point3f(m_camera.position - m_camera.target)) + vec3f(m_camera.target);
			m_camera.position = mat4f::rotate(vec3f(0, 1, 0), yaw).multiplyPoint(point3f(m_camera.position - m_camera.target)) + vec3f(m_camera.target);
		}
		const input::Position& scroll = input::scroll();
		if (scroll.y != 0.f)
		{
			vec3f dir = vec3f::normalize(vec3f(m_camera.target - m_camera.position));
			m_camera.position = m_camera.position + dir * (scroll.y * m_camera.speed * deltaTime.seconds());
		}
		m_camera.transform = mat4f::lookAt(m_camera.position, m_camera.target, m_camera.up);
	}

	// TOD
	if (input::pressed(input::Button::ButtonMiddle))
	{
		const input::Position& pos = input::mouse();
		float x = pos.x / (float)GraphicBackend::backbuffer()->width();
		m_lightDir = vec3f::normalize(lerp(vec3f(1, 1, 1), vec3f(-1, 1, -1), x));
	}

	// Reset
	if (input::down(input::Key::R))
	{
		m_camera.set(m_model->bbox);
	}
	// Hot reload
	if (input::down(input::Key::Space))
	{
		Logger::info("Reloading shaders...");
		loadShader();
	}
	// Quit the app if requested
	if (aka::input::pressed(aka::input::Key::Escape))
	{
		EventDispatcher<QuitEvent>::emit();
	}
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
	// TODO compute frustum dynamically
	mat4f depthProjectionMatrix = mat4f::orthographic(-3000.f, 3000.f, -3000.f, 3000.f, -3000.f, 3000.f);
	mat4f depthViewMatrix = mat4f::inverse(mat4f::lookAt(point3f(m_lightDir), point3f(0), norm3f(0, 0, 1)));
	mat4f depthModelMatrix = mat4f::identity();
	mat4f depthMVP = depthProjectionMatrix * depthViewMatrix * depthModelMatrix;
	mat4f biasMatrix(
		col4f(0.5, 0.0, 0.0, 0.0),
		col4f(0.0, 0.5, 0.0, 0.0),
		col4f(0.0, 0.0, 0.5, 0.0),
		col4f(0.5, 0.5, 0.5, 1.0)
	);
	mat4f depthBiasMVP = biasMatrix * depthMVP;

	if (m_shadowShader == nullptr)
	{
		const char* vertShader = ""
			"#version 330 core\n"
			"// Input vertex data, different for all executions of this shader.\n"
			"layout(location = 0) in vec3 vertexPosition_modelspace;\n"
			"// Values that stay constant for the whole mesh.\n"
			"uniform mat4 u_depthMVP;\n"
			"void main() {\n"
			"	gl_Position = u_depthMVP * vec4(vertexPosition_modelspace, 1);\n"
			"}\n";
		const char* fragShader = ""
			"#version 330 core\n"
			"// Ouput data\n"
			"layout(location = 0) out float fragmentdepth;\n"
			"void main() {\n"
			"	// Not really needed, OpenGL does it anyway\n"
			"	gl_FragDepth = gl_FragCoord.z;\n"
			"}\n";
		std::vector<Attributes> attributes;
		ShaderID vert = Shader::compile(vertShader, ShaderType::Vertex);
		ShaderID frag = Shader::compile(fragShader, ShaderType::Fragment);
		m_shadowShader = Shader::create(vert, frag, ShaderID(0), attributes);
		m_shadowMaterial = ShaderMaterial::create(m_shadowShader);
	}
	static RenderPass shadowPass;
	m_shadowMaterial->set<mat4f>("u_depthMVP", depthMVP);
	m_shadowFramebuffer->clear(0.f, 0.f, 0.f, 1.f);
	for (size_t i = 0; i < m_model->meshes.size(); i++)
	{
		shadowPass.framebuffer = m_shadowFramebuffer;
		shadowPass.mesh = m_model->meshes[i];
		shadowPass.indexCount = m_model->meshes[i]->getIndexCount(); // TODO set zero means all ?
		shadowPass.indexOffset = 0;
		shadowPass.material = m_shadowMaterial;
		shadowPass.blend = Blending::none();
		shadowPass.cull = m_model->materials[i].doubleSided ? doubleSide : singleSide;
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
	backbuffer->clear(0.f, 0.f, 0.f, 1.f);
	if (input::pressed(input::Key::Space))
	{
		static Batch b;
		b.draw(mat3f::scale(vec2f(backbuffer->width(), backbuffer->height())), Batch::Rect(vec2f(0.f), vec2f(1.f), m_shadowFramebuffer->attachment(Framebuffer::AttachmentType::Depth), 0));
		b.render();
		b.clear();
	}
	else
	{
		aka::mat4f view = aka::mat4f::inverse(m_camera.transform);
		aka::mat4f perspective = aka::mat4f::perspective(aka::degreef(90.f), (float)backbuffer->width() / (float)backbuffer->height(), 0.01f, 10000.f);
		for (size_t i = 0; i < m_model->meshes.size(); i++)
		{
			aka::mat4f model = m_model->transforms[i];
			aka::mat3f normal = aka::mat3f::transpose(aka::mat3f::inverse(mat3f(model)));
			aka::color4f color = m_model->materials[i].color;
			m_material->set<mat4f>("u_projection", perspective);
			m_material->set<mat4f>("u_model", model);
			m_material->set<mat4f>("u_view", view);
			m_material->set<mat4f>("u_depthMVP", depthBiasMVP); // TODO transform by object
			m_material->set<mat3f>("u_normalMatrix", normal);
			m_material->set<vec3f>("u_lightDir", m_lightDir);
			m_material->set<color4f>("u_color", color);
			m_material->set<Texture::Ptr>("u_colorTexture", m_model->materials[i].colorTexture);
			m_material->set<Texture::Ptr>("u_normalTexture", m_model->materials[i].normalTexture);
			m_material->set<Texture::Ptr>("u_shadowTexture", m_shadowFramebuffer->attachment(Framebuffer::AttachmentType::Depth));
			renderPass.framebuffer = backbuffer;
			renderPass.mesh = m_model->meshes[i];
			renderPass.indexCount = m_model->meshes[i]->getIndexCount(); // TODO set zero means all ?
			renderPass.indexOffset = 0;
			renderPass.material = m_material;
			renderPass.blend = blending;
			renderPass.cull = m_model->materials[i].doubleSided ? doubleSide : singleSide;
			renderPass.depth = depth;
			renderPass.stencil = stencil;
			renderPass.viewport = aka::Rect{ 0 };
			renderPass.scissor = aka::Rect{ 0 };

			renderPass.execute();
		}
	}
}

};
