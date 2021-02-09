#include "ModelViewer.h"

#include "Model/GLTF/GLTFLoader.h"

namespace viewer {

void Viewer::loadShader()
{
#if defined(AKA_USE_OPENGL)
	aka::ShaderID vert = aka::Shader::compile(readString("../../../src/data/shaders/GL/gltf.vert"), aka::ShaderType::Vertex);
	aka::ShaderID frag = aka::Shader::compile(readString("../../../src/data/shaders/GL/gltf.frag"), aka::ShaderType::Fragment);
	std::vector<aka::Attributes> attributes;
#else
	std::string str = readString("./data/shaders/D3D/shader.hlsl");
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

void Viewer::initialize()
{
	GLTFLoader loader;
	// TODO use args
	//m_model = loader.load("./glTF-Sample-Models-master/2.0/Sponza/glTF/Sponza.gltf");
	//m_model = loader.load("./glTF-Sample-Models-master/2.0/AlphaBlendModeTest/glTF/AlphaBlendModeTest.gltf");
	//m_model = loader.load("./glTF-Sample-Models-master/2.0/CesiumMilkTruck/glTF/CesiumMilkTruck.gltf");
	m_model = loader.load("../../../../glTF-Sample-Models-master/2.0/Lantern/glTF/Lantern.gltf");
	if (m_model == nullptr)
		throw std::runtime_error("Could not load model.");
	aka::Logger::info("Scene Bounding box : ", m_model->bbox.min, " - ", m_model->bbox.max);

	loadShader();
	
	m_camera.set(m_model->bbox);
	//aka::Logger::info("Camera : ", m_camera.transform);
}

void Viewer::destroy()
{
}

void Viewer::update(aka::Time::Unit deltaTime)
{
	using namespace aka;

	// Arcball
	{
		// https://gamedev.stackexchange.com/questions/53333/how-to-implement-a-basic-arcball-camera-in-opengl-with-glm
		if (input::pressed(input::Button::Button1))
		{
			input::Position& delta = input::delta();
			radianf pitch = radianf(-delta.y * deltaTime.seconds());
			radianf yaw = radianf(-delta.x * deltaTime.seconds());
			m_camera.position = mat4f::rotate(vec3f(1, 0, 0), pitch).multiplyPoint(m_camera.position - m_camera.target) + m_camera.target;
			m_camera.position = mat4f::rotate(vec3f(0, 1, 0), yaw).multiplyPoint(m_camera.position - m_camera.target) + m_camera.target;
		}
		input::Position& scroll = input::scroll();
		if (scroll.y != 0.f)
		{
			vec3f dir = vec3f::normalize(vec3f(m_camera.target - m_camera.position));
			m_camera.position = m_camera.position + point3f(dir * (scroll.y * m_camera.speed * deltaTime.seconds()));
		}
		m_camera.transform = mat4f::lookAt(m_camera.position, m_camera.target, m_camera.up);
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
		quit();
	}
}

void Viewer::render()
{
	static aka::RenderPass renderPass{};
	static Culling doubleSide = Culling{ CullMode::None, CullOrder::CounterClockWise };
	static Culling singleSide = Culling{ CullMode::BackFace, CullOrder::CounterClockWise };
	static Blending blending = Blending::normal();
	static Depth depth = Depth{ DepthCompare::LessOrEqual, true };
	static Stencil stencil = Stencil::none();
	aka::Framebuffer::Ptr backbuffer = aka::GraphicBackend::backbuffer();
	backbuffer->clear(0.f, 0.f, 0.f, 1.f);
	aka::mat4f perspective = aka::mat4f::perspective(aka::degreef(90.f), (float)backbuffer->width() / (float)backbuffer->height(), 0.01f, 1000.f);
	aka::mat4f view = aka::mat4f::inverse(m_camera.transform);
	for (size_t i = 0; i < m_model->meshes.size(); i++)
	{
		aka::mat4f model = m_model->transforms[i];
		aka::mat3f normal = aka::mat3f::transpose(aka::mat3f::inverse(mat3f(model)));
		aka::color4f color = m_model->materials[i].color;
		m_material->set<mat4f>("u_projection", perspective);
		m_material->set<mat4f>("u_model", model);
		m_material->set<mat4f>("u_view", view);
		m_material->set<mat3f>("u_normalMatrix", normal);
		m_material->set<color4f>("u_color", color);
		m_material->set<Texture::Ptr>("u_colorTexture", m_model->materials[i].colorTexture);
		m_material->set<Texture::Ptr>("u_normalTexture", m_model->materials[i].normalTexture);
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

};
