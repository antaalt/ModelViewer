#include "ModelViewer.h"

#include <imgui.h>
#include <imguizmo.h>
#include <Aka/Layer/ImGuiLayer.h>

#include "Editor/SceneEditor.h"
#include "Editor/InfoEditor.h"
#include "Editor/AssetEditor.h"

namespace viewer {

void Viewer::loadShader()
{
	// TODO cache shader and do not delete them during program creation.
	// TODO use custom file for handling multiple API (ASSET REWORK, JSON based ?)
	// - a file indexing shader path depending on the api & the type (frag, vert...) (JSON)
	// - a single file containing all the shaders from all the api delimited by # or json 
	//	-> problem of linting.
	{
		std::vector<Attributes> attributes = { // HLSL only
			Attributes{ AttributeID(0), "POS" },
			Attributes{ AttributeID(0), "NORM" },
			Attributes{ AttributeID(0), "TEX" },
			Attributes{ AttributeID(0), "COL" }
		};
#if defined(AKA_USE_OPENGL)
		aka::ShaderID vert = aka::Shader::compile(File::readString(Asset::path("shaders/GL/gbuffer.vert")), aka::ShaderType::Vertex);
		aka::ShaderID frag = aka::Shader::compile(File::readString(Asset::path("shaders/GL/gbuffer.frag")), aka::ShaderType::Fragment);
#else
		std::string str = File::readString(Asset::path("shaders/D3D/gbuffer.hlsl"));
		aka::ShaderID vert = aka::Shader::compile(str, aka::ShaderType::Vertex);
		aka::ShaderID frag = aka::Shader::compile(str, aka::ShaderType::Fragment);
#endif
		if (vert == ShaderID(0) || frag == ShaderID(0))
		{
			aka::Logger::error("Failed to compile gbuffer shader");
		}
		else
		{
			aka::Shader::Ptr shader = aka::Shader::create(vert, frag, attributes);
			if (shader->valid())
				m_gbufferMaterial = aka::ShaderMaterial::create(shader);
		}
	}
	{
		std::vector<Attributes> attributes = { // HLSL only
			Attributes{ AttributeID(0), "POS" }
		};
#if defined(AKA_USE_OPENGL)
		aka::ShaderID vert = aka::Shader::compile(File::readString(Asset::path("shaders/GL/point.vert")), aka::ShaderType::Vertex);
		aka::ShaderID frag = aka::Shader::compile(File::readString(Asset::path("shaders/GL/point.frag")), aka::ShaderType::Fragment);
#else
		std::string str = File::readString(Asset::path("shaders/D3D/point.hlsl"));
		aka::ShaderID vert = aka::Shader::compile(str, aka::ShaderType::Vertex);
		aka::ShaderID frag = aka::Shader::compile(str, aka::ShaderType::Fragment);
#endif
		if (vert == ShaderID(0) || frag == ShaderID(0))
		{
			aka::Logger::error("Failed to compile point light shader");
		}
		else
		{
			aka::Shader::Ptr shader = aka::Shader::create(vert, frag, attributes);
			if (shader->valid())
				m_pointMaterial = aka::ShaderMaterial::create(shader);
		}
	}
	{
		std::vector<Attributes> attributes = { // HLSL only
			Attributes{ AttributeID(0), "POS" }
		};
#if defined(AKA_USE_OPENGL)
		aka::ShaderID vert = aka::Shader::compile(File::readString(Asset::path("shaders/GL/quad.vert")), aka::ShaderType::Vertex);
		aka::ShaderID frag = aka::Shader::compile(File::readString(Asset::path("shaders/GL/directional.frag")), aka::ShaderType::Fragment);
#else
		std::string str = File::readString(Asset::path("shaders/D3D/directional.hlsl"));
		aka::ShaderID vert = aka::Shader::compile(str, aka::ShaderType::Vertex);
		aka::ShaderID frag = aka::Shader::compile(str, aka::ShaderType::Fragment);
#endif
		if (vert == ShaderID(0) || frag == ShaderID(0))
		{
			aka::Logger::error("Failed to compile directional light shader");
		}
		else
		{
			aka::Shader::Ptr shader = aka::Shader::create(vert, frag, attributes);
			if (shader->valid())
				m_dirMaterial = aka::ShaderMaterial::create(shader);
		}
	}
	{
		std::vector<Attributes> attributes = { // HLSL only
			Attributes{ AttributeID(0), "POS" }
		};
#if defined(AKA_USE_OPENGL)
		aka::ShaderID vert = aka::Shader::compile(File::readString(Asset::path("shaders/GL/quad.vert")), aka::ShaderType::Vertex);
		aka::ShaderID frag = aka::Shader::compile(File::readString(Asset::path("shaders/GL/ambient.frag")), aka::ShaderType::Fragment);
#else
		std::string str = File::readString(Asset::path("shaders/D3D/ambient.hlsl"));
		aka::ShaderID vert = aka::Shader::compile(str, aka::ShaderType::Vertex);
		aka::ShaderID frag = aka::Shader::compile(str, aka::ShaderType::Fragment);
#endif
		if (vert == ShaderID(0) || frag == ShaderID(0))
		{
			aka::Logger::error("Failed to compile ambient shader");
		}
		else
		{
			aka::Shader::Ptr shader = aka::Shader::create(vert, frag, attributes);
			if (shader->valid())
				m_ambientMaterial = aka::ShaderMaterial::create(shader);
		}
	}
	{
		std::vector<Attributes> attributes = { // HLSL only
			Attributes{ AttributeID(0), "POS" },
			Attributes{ AttributeID(0), "NORM" },
			Attributes{ AttributeID(0), "TEX" },
			Attributes{ AttributeID(0), "COL" }
		};
#if defined(AKA_USE_OPENGL)
		ShaderID vert = Shader::compile(File::readString(Asset::path("shaders/GL/shadow.vert")), ShaderType::Vertex);
		ShaderID frag = Shader::compile(File::readString(Asset::path("shaders/GL/shadow.frag")), ShaderType::Fragment);
#else
		std::string str = File::readString(Asset::path("shaders/D3D/shadow.hlsl"));
		ShaderID vert = Shader::compile(str, ShaderType::Vertex);
		ShaderID frag = Shader::compile(str, ShaderType::Fragment);
#endif
		if (vert == ShaderID(0) || frag == ShaderID(0))
		{
			aka::Logger::error("Failed to compile shadow shader");
		}
		else
		{
			aka::Shader::Ptr shader = aka::Shader::create(vert, frag, attributes);
			if (shader->valid())
				m_shadowMaterial = aka::ShaderMaterial::create(shader);
		}
	}
	{
		std::vector<Attributes> attributes = { // HLSL only
			Attributes{ AttributeID(0), "POS" },
			Attributes{ AttributeID(0), "NORM" },
			Attributes{ AttributeID(0), "TEX" },
			Attributes{ AttributeID(0), "COL" }
		};
#if defined(AKA_USE_OPENGL)
		ShaderID vert = Shader::compile(File::readString(Asset::path("shaders/GL/shadowPoint.vert")), ShaderType::Vertex);
		ShaderID geo = Shader::compile(File::readString(Asset::path("shaders/GL/shadowPoint.geo")), ShaderType::Geometry);
		ShaderID frag = Shader::compile(File::readString(Asset::path("shaders/GL/shadowPoint.frag")), ShaderType::Fragment);
#else
		std::string str = File::readString(Asset::path("shaders/D3D/shadowPoint.hlsl"));
		ShaderID vert = Shader::compile(str, ShaderType::Vertex);
		ShaderID geo = Shader::compile(str, ShaderType::Geometry);
		ShaderID frag = Shader::compile(str, ShaderType::Fragment);
#endif
		if (vert == ShaderID(0) || frag == ShaderID(0) || geo == ShaderID(0))
		{
			aka::Logger::error("Failed to compile shadow point shader");
		}
		else
		{
			aka::Shader::Ptr shader = aka::Shader::createGeometry(vert, frag, geo, attributes);
			if (shader->valid())
				m_shadowPointMaterial = aka::ShaderMaterial::create(shader);
		}
	}
	{
		std::vector<Attributes> attributes = { // HLSL only
			Attributes{ AttributeID(0), "POS" },
		};
#if defined(AKA_USE_OPENGL)
		ShaderID vert = Shader::compile(File::readString(Asset::path("shaders/GL/skybox.vert")), ShaderType::Vertex);
		ShaderID frag = Shader::compile(File::readString(Asset::path("shaders/GL/skybox.frag")), ShaderType::Fragment);
#else
		std::string str = File::readString(Asset::path("shaders/D3D/skybox.hlsl"));
		ShaderID vert = Shader::compile(str, ShaderType::Vertex);
		ShaderID frag = Shader::compile(str, ShaderType::Fragment);
#endif
		if (vert == ShaderID(0) || frag == ShaderID(0))
		{
			aka::Logger::error("Failed to compile skybox shader");
		}
		else
		{
			aka::Shader::Ptr shader = aka::Shader::create(vert, frag, attributes);
			if (shader->valid())
				m_skyboxMaterial = aka::ShaderMaterial::create(shader);
		}
	}
	{
		std::vector<Attributes> attributes = { // HLSL only
			Attributes{ AttributeID(0), "POS" }
		};
#if defined(AKA_USE_OPENGL)
		aka::ShaderID vert = aka::Shader::compile(File::readString(Asset::path("shaders/GL/quad.vert")), aka::ShaderType::Vertex);
		aka::ShaderID frag = aka::Shader::compile(File::readString(Asset::path("shaders/GL/postProcess.frag")), aka::ShaderType::Fragment);
#else
		std::string str = File::readString(Asset::path("shaders/D3D/postProcess.hlsl"));
		aka::ShaderID vert = aka::Shader::compile(str, aka::ShaderType::Vertex);
		aka::ShaderID frag = aka::Shader::compile(str, aka::ShaderType::Fragment);
#endif
		if (vert == ShaderID(0) || frag == ShaderID(0))
		{
			aka::Logger::error("Failed to compile post process shader");
		}
		else
		{
			aka::Shader::Ptr shader = aka::Shader::create(vert, frag, attributes);
			if (shader->valid())
				m_postprocessMaterial = aka::ShaderMaterial::create(shader);
		}
	}
}

void Viewer::onCreate()
{
	m_world.attach<ArcballCameraSystem>();
	m_world.attach<Scene::GraphSystem>();
	m_world.create();

	m_editors.push_back(new SceneEditor);
	m_editors.push_back(new InfoEditor);
	m_editors.push_back(new AssetEditor);
	for (EditorWindow* editor : m_editors)
		editor->onCreate(m_world);

	StopWatch<> stopWatch;
	// TODO use args & worker
	bool loaded = false;
	loaded = ModelLoader::load(Asset::path("glTF-Sample-Models/2.0/Sponza/glTF/Sponza.gltf"), m_world);
	//loaded = ModelLoader::load(Asset::path("glTF-Sample-Models/2.0/AlphaBlendModeTest/glTF/AlphaBlendModeTest.gltf"), m_world);
	//loaded = ModelLoader::load(Asset::path("glTF-Sample-Models/2.0/CesiumMilkTruck/glTF/CesiumMilkTruck.gltf"), m_world);
	//loaded = ModelLoader::load(Asset::path("glTF-Sample-Models/2.0/Lantern/glTF/Lantern.gltf"), m_world);
	//loaded = ModelLoader::load(Asset::path("glTF-Sample-Models/2.0/EnvironmentTest/glTF/EnvironmentTest.gltf"), m_world);
	if (!loaded)
		throw std::runtime_error("Could not load model.");
	// Compute scene bounds.
	auto view = m_world.registry().view<Transform3DComponent, MeshComponent>();
	view.each([this](Transform3DComponent& t, MeshComponent& mesh) {
		m_bounds.include(t.transform * mesh.bounds);
	});
	aka::Logger::info("Model loaded : ", stopWatch.elapsed(), "ms");
	aka::Logger::info("Scene Bounding box : ", m_bounds.min, " - ", m_bounds.max);

	loadShader();

	// --- G-Buffer pass
	Sampler gbufferSampler{};
	gbufferSampler.filterMag = Sampler::Filter::Nearest;
	gbufferSampler.filterMin = Sampler::Filter::Nearest;
	gbufferSampler.wrapU = Sampler::Wrap::ClampToEdge;
	gbufferSampler.wrapV = Sampler::Wrap::ClampToEdge;
	gbufferSampler.wrapW = Sampler::Wrap::ClampToEdge;

	// Depth | Stencil
	// D     | S  
	// 
	// position | _
	// R G B    | A
	// 
	// albedo | opacity
	// R G B  | A
	// 
	// normal | _
	// R G B  | A
	// 
	// ao | roughness | metalness | _
	// R  | G         | B         | A
	m_depth = Texture::create2D(width(), height(), TextureFormat::UnsignedInt248, TextureComponent::Depth24Stencil8, TextureFlag::RenderTarget, gbufferSampler);
	m_position = Texture::create2D(width(), height(), TextureFormat::Float, TextureComponent::RGBA16F, TextureFlag::RenderTarget, gbufferSampler);
	m_albedo = Texture::create2D(width(), height(), TextureFormat::UnsignedByte, TextureComponent::RGBA, TextureFlag::RenderTarget, gbufferSampler);
	m_normal = Texture::create2D(width(), height(), TextureFormat::Float, TextureComponent::RGBA16F, TextureFlag::RenderTarget, gbufferSampler);
	m_roughness = Texture::create2D(width(), height(), TextureFormat::Float, TextureComponent::RGBA16F, TextureFlag::RenderTarget, gbufferSampler);
	FramebufferAttachment gbufferAttachments[] = {
		FramebufferAttachment{
			FramebufferAttachmentType::DepthStencil,
			m_depth
		},
		FramebufferAttachment{
			FramebufferAttachmentType::Color0,
			m_position
		},
		FramebufferAttachment{
			FramebufferAttachmentType::Color1,
			m_albedo
		},
		FramebufferAttachment{
			FramebufferAttachmentType::Color2,
			m_normal
		},
		FramebufferAttachment{
			FramebufferAttachmentType::Color3,
			m_roughness
		}
	};
	m_gbuffer = Framebuffer::create(gbufferAttachments, sizeof(gbufferAttachments) / sizeof(FramebufferAttachment));

	// --- Lighting pass
	float quadVertices[] = {
		-1, -1, // bottom left corner
		 1, -1, // bottom right corner
		 1,  1, // top right corner
		-1,  1, // top left corner
	};
	uint8_t quadIndices[] = { 0,1,2,0,2,3 };
	m_quad = Mesh::create();
	VertexInfo quadVertexInfo{ {
		VertexAttributeData {
			VertexAttribute{ VertexFormat::Float, VertexType::Vec2 },
			SubBuffer{
				Buffer::create(BufferType::VertexBuffer, sizeof(quadVertices), BufferUsage::Static, BufferAccess::ReadOnly, quadVertices),
				0,
				sizeof(quadVertices)
			}, 
			sizeof(float) * 2,
			0
		}
	} };
	IndexInfo quadIndexInfo{};
	quadIndexInfo.format = IndexFormat::UnsignedByte;
	quadIndexInfo.subBuffer.buffer = Buffer::create(BufferType::IndexBuffer, sizeof(quadIndices), BufferUsage::Static, BufferAccess::ReadOnly, quadIndices);
	quadIndexInfo.subBuffer.offset = 0;
	quadIndexInfo.subBuffer.size = (uint32_t)quadIndexInfo.subBuffer.buffer->size();
	m_quad->upload(quadVertexInfo, quadIndexInfo);
	
	m_sphere = Scene::createSphereMesh(point3f(0.f), 1.f, 32, 16);

	// --- Skybox pass
	String cubemapPath[6] = {
		"skybox/skybox_px.jpg",
		"skybox/skybox_nx.jpg",
		"skybox/skybox_py.jpg",
		"skybox/skybox_ny.jpg",
		"skybox/skybox_pz.jpg",
		"skybox/skybox_nz.jpg",
	};
	Image cubemap[6];
	for (size_t i = 0; i < 6; i++)
	{
		cubemap[i] = Image::load(Asset::path(cubemapPath[i]), false);
		AKA_ASSERT(cubemap[i].width == cubemap[0].width && cubemap[i].height == cubemap[0].height, "Width & height not matching");
	}
	Sampler cubeSampler{};
	cubeSampler.filterMag = Sampler::Filter::Linear;
	cubeSampler.filterMin = Sampler::Filter::Linear;
	cubeSampler.wrapU = Sampler::Wrap::ClampToEdge;
	cubeSampler.wrapV = Sampler::Wrap::ClampToEdge;
	cubeSampler.wrapW = Sampler::Wrap::ClampToEdge;
	m_skybox = Texture::createCubemap(
		cubemap[0].width, cubemap[0].height,
		TextureFormat::UnsignedByte, TextureComponent::RGBA, TextureFlag::None, 
		cubeSampler, 
		cubemap[0].bytes.data(),
		cubemap[1].bytes.data(),
		cubemap[2].bytes.data(),
		cubemap[3].bytes.data(),
		cubemap[4].bytes.data(),
		cubemap[5].bytes.data()
	);

	float skyboxVertices[] = {
		-1.0f,  1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		-1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f
	};
	m_cube = Mesh::create();

	VertexInfo skyboxVertexInfo{ {
		VertexAttributeData {
			VertexAttribute{ VertexFormat::Float, VertexType::Vec3 },
			SubBuffer{
				Buffer::create(BufferType::VertexBuffer, sizeof(skyboxVertices), BufferUsage::Static, BufferAccess::ReadOnly, skyboxVertices),
				0,
				sizeof(skyboxVertices)
			},
			sizeof(float) * 3,
			0
		}
	} };
	m_cube->upload(skyboxVertexInfo);

	// --- Shadows
	Sampler shadowSampler{};
	shadowSampler.filterMag = Sampler::Filter::Nearest;
	shadowSampler.filterMin = Sampler::Filter::Nearest;
	shadowSampler.wrapU = Sampler::Wrap::ClampToEdge;
	shadowSampler.wrapV = Sampler::Wrap::ClampToEdge;
	shadowSampler.wrapW = Sampler::Wrap::ClampToEdge;

	m_sun = m_world.createEntity("Sun");
	m_sun.add<Transform3DComponent>();
	m_sun.add<Hierarchy3DComponent>();
	m_sun.add<DirectionalLightComponent>();
	m_sun.add<DirtyLightComponent>();
	Transform3DComponent& sunTransform = m_sun.get<Transform3DComponent>();
	Hierarchy3DComponent& h = m_sun.get<Hierarchy3DComponent>();
	DirectionalLightComponent& sun = m_sun.get<DirectionalLightComponent>();
	sun.direction = vec3f::normalize(vec3f(0.1f, 1.f, 0.1f));
	sun.color = color3f(1.f);
	sun.intensity = 10.f;
	sunTransform.transform = mat4f::identity();
	sun.shadowMap[0] = Texture::create2D(2048, 2048, TextureFormat::Float, TextureComponent::Depth, TextureFlag::RenderTarget, shadowSampler);
	sun.shadowMap[1] = Texture::create2D(2048, 2048, TextureFormat::Float, TextureComponent::Depth, TextureFlag::RenderTarget, shadowSampler);
	sun.shadowMap[2] = Texture::create2D(4096, 4096, TextureFormat::Float, TextureComponent::Depth, TextureFlag::RenderTarget, shadowSampler);
	// TODO texture atlas & single shader execution
	for (size_t i = 0; i < 3; i++)
		sun.worldToLightSpaceMatrix[i] = mat4f::identity();
	// TODO empty attachment as default ?
	FramebufferAttachment shadowAttachments[] = {
		FramebufferAttachment{
			FramebufferAttachmentType::Depth,
			sun.shadowMap[0]
		}
	};
	m_shadowFramebuffer = Framebuffer::create(shadowAttachments, 1);

	{
		// Second dir light
		Entity e = m_world.createEntity("Sun2");
		e.add<Transform3DComponent>();
		e.add<Hierarchy3DComponent>();
		e.add<DirectionalLightComponent>();
		e.add<DirtyLightComponent>();
		Transform3DComponent& t = e.get<Transform3DComponent>();
		Hierarchy3DComponent& h = e.get<Hierarchy3DComponent>();
		DirectionalLightComponent& l = e.get<DirectionalLightComponent>();
		l.direction = vec3f::normalize(vec3f(0.2f, 1.f, 0.2f));
		l.color = color3f(1.f, 0.1f, 0.2f);
		l.intensity = 1.f;
		t.transform = mat4f::identity();
		l.shadowMap[0] = Texture::create2D(2048, 2048, TextureFormat::Float, TextureComponent::Depth, TextureFlag::RenderTarget, shadowSampler);
		l.shadowMap[1] = Texture::create2D(2048, 2048, TextureFormat::Float, TextureComponent::Depth, TextureFlag::RenderTarget, shadowSampler);
		l.shadowMap[2] = Texture::create2D(4096, 4096, TextureFormat::Float, TextureComponent::Depth, TextureFlag::RenderTarget, shadowSampler);
		// TODO texture atlas & single shader execution
		for (size_t i = 0; i < 3; i++)
			l.worldToLightSpaceMatrix[i] = mat4f::identity();
	}
	{
		// Point light
		Entity e = m_world.createEntity("Light");
		e.add<Transform3DComponent>();
		e.add<Hierarchy3DComponent>();
		e.add<PointLightComponent>();
		e.add<DirtyLightComponent>();
		Transform3DComponent& t = e.get<Transform3DComponent>();
		Hierarchy3DComponent& h = e.get<Hierarchy3DComponent>();
		PointLightComponent& l = e.get<PointLightComponent>();
		l.color = color3f(0.2f, 1.f, 0.2f);
		l.intensity = 10.f;
		t.transform = mat4f::translate(vec3f(0, 1, 0));
		l.shadowMap = Texture::createCubemap(1024, 1024, TextureFormat::Float, TextureComponent::Depth, TextureFlag::RenderTarget, shadowSampler);
		for (size_t i = 0; i < 6; i++)
			l.worldToLightSpaceMatrix[i] = mat4f::identity();
	}

	// --- FXAA pass
	m_storageDepth = Texture::create2D(width(), height(), TextureFormat::UnsignedInt248, TextureComponent::Depth24Stencil8, TextureFlag::RenderTarget, gbufferSampler);
	m_storage = Texture::create2D(width(), height(), TextureFormat::Float, TextureComponent::RGBA16F, TextureFlag::RenderTarget, gbufferSampler);
	FramebufferAttachment storageAttachment[] = {
		FramebufferAttachment{
			FramebufferAttachmentType::DepthStencil,
			m_storageDepth
		},
		FramebufferAttachment{
			FramebufferAttachmentType::Color0,
			m_storage
		}
	};
	m_storageFramebuffer = Framebuffer::create(storageAttachment, 2);

	// --- Camera
	{
		m_camera = m_world.createEntity("Camera");
		m_camera.add<Transform3DComponent>();
		m_camera.add<Hierarchy3DComponent>();
		m_camera.add<Camera3DComponent>();
		m_camera.add<ArcballCameraComponent>();
		m_camera.add<DirtyCameraComponent>();
		ArcballCameraComponent& controller = m_camera.get<ArcballCameraComponent>();
		Transform3DComponent& transform = m_camera.get<Transform3DComponent>();
		Camera3DComponent& camera = m_camera.get<Camera3DComponent>();
		controller.set(m_bounds);
		transform.transform = mat4f::lookAt(controller.position, controller.target, controller.up);
		camera.projection = &m_projection;
		camera.view = mat4f::inverse(transform.transform);
	}
	
	m_projection.nearZ = 0.01f;
	m_projection.farZ = 100.f;
	m_projection.hFov = anglef::degree(90.f);
	m_projection.ratio = width() / (float)height();

	// --- UI
	m_debug = true;
	attach<ImGuiLayer>();
}

void Viewer::onDestroy()
{
	for (EditorWindow* editor : m_editors)
	{
		editor->onDestroy(m_world);
		delete editor;
	}
	m_editors.clear();
	m_world.destroy();
}

void Viewer::onUpdate(aka::Time::Unit deltaTime)
{
	// Arcball status
	m_camera.get<ArcballCameraComponent>().active = !ImGui::GetIO().WantCaptureKeyboard && !ImGui::GetIO().WantCaptureMouse && !ImGuizmo::IsUsing();

	// TOD
	if (Mouse::pressed(MouseButton::ButtonMiddle))
	{
		const Position& pos = Mouse::position();
		float x = pos.x / (float)GraphicBackend::backbuffer()->width();
		m_sun.get<DirectionalLightComponent>().direction = vec3f::normalize(lerp(vec3f(1, 1, 1), vec3f(-1, 1, -1), x));
		if (!m_sun.has<DirtyLightComponent>())
			m_sun.add<DirtyLightComponent>();
	}

	// Reset
	if (Keyboard::down(KeyboardKey::R) && !ImGui::GetIO().WantCaptureKeyboard)
	{
		m_camera.get<ArcballCameraComponent>().set(m_bounds);
		auto dirLightUpdate = m_world.registry().view<DirectionalLightComponent>();
		for (entt::entity e : dirLightUpdate)
			if (!m_world.registry().has<DirtyLightComponent>(e))
				m_world.registry().emplace<DirtyLightComponent>(e);
	}
	if (Keyboard::down(KeyboardKey::D) && !ImGui::GetIO().WantCaptureKeyboard)
	{
		m_debug = !m_debug;
	}
	if (Keyboard::down(KeyboardKey::PrintScreen) && !ImGui::GetIO().WantCaptureKeyboard)
	{
		GraphicBackend::screenshot("screen.png");
	}
	// Hot reload
	if (Keyboard::down(KeyboardKey::Space) && !ImGui::GetIO().WantCaptureKeyboard)
	{
		Logger::info("Reloading shaders...");
		loadShader();
	}
	// Quit the app if requested
	if (Keyboard::pressed(KeyboardKey::Escape) && !ImGui::GetIO().WantCaptureKeyboard)
	{
		EventDispatcher<QuitEvent>::emit();
	}
	m_world.update(deltaTime);
	// Editor
	for (EditorWindow* editor : m_editors)
		editor->onUpdate(m_world);
}

void Viewer::onResize(uint32_t width, uint32_t height)
{
	m_projection.ratio = width / (float)height;
}

// --- Shadows
// Compute the shadow projection around the view projection.
mat4f computeShadowViewProjectionMatrix(const mat4f& view, const mat4f& projection, uint32_t resolution, float near, float far, const vec3f& lightDirWorld)
{
	frustum<> f = frustum<>::fromProjection(projection * view);
	point3f centerWorld = f.center();
	// http://alextardif.com/shadowmapping.html
	vec3f extentDepth = f.corners[0] - f.corners[7];
	vec3f extentWidth = f.corners[1] - f.corners[7];
	vec3f e = vec3f(
		max(abs(extentDepth.x), abs(extentWidth.x)),
		max(abs(extentDepth.y), abs(extentWidth.y)),
		max(abs(extentDepth.z), abs(extentWidth.z))
	);
	float radius = e.norm() / 2.f;
	float texelsPerUnit = (float)resolution / (radius * 2.f);

	mat4f lookAt = mat4f::inverse(mat4f::lookAt(point3f(0.f), point3f(-lightDirWorld), norm3f(0, 1, 0)));
	mat4f scalar = mat4f::scale(vec3f(texelsPerUnit));
	lookAt *= scalar;
	mat4f lookAtInverse = mat4f::inverse(lookAt);

	centerWorld = lookAt * centerWorld;
	centerWorld.x = floor(centerWorld.x);
	centerWorld.y = floor(centerWorld.y);
	centerWorld = lookAtInverse * centerWorld;

	point3f eye = centerWorld + (lightDirWorld * radius * 2.f);

	mat4f lightViewMatrix = mat4f::inverse(mat4f::lookAt(eye, centerWorld, norm3f(0, 1, 0)));

	float scale = 6.f; // scalar to improve depth so that we don't miss shadow of tall objects
	mat4 lightProjectionMatrix = mat4f::orthographic(
		-radius, radius,
		-radius, radius,
		-radius * scale, radius * scale
	);
	return lightProjectionMatrix * lightViewMatrix;
}

void Viewer::onRender()
{
	Framebuffer::Ptr backbuffer = GraphicBackend::backbuffer();
	mat4f debugView = mat4f::inverse(mat4f::lookAt(m_bounds.center() + m_bounds.extent(), point3f(0.f)));
	mat4f view = m_camera.get<Camera3DComponent>().view;
	// TODO get main camera
	mat4f debugPerspective = mat4f::perspective(m_projection.hFov, (float)backbuffer->width() / (float)backbuffer->height(), 0.01f, 1000.f);
	mat4f perspective = m_camera.get<Camera3DComponent>().projection->projection();

	// --- Shadow map system
	auto pointLightUpdate = m_world.registry().view<DirtyLightComponent, PointLightComponent>();
	auto dirLightUpdate = m_world.registry().view<DirtyLightComponent, DirectionalLightComponent>();
	for (entt::entity e : pointLightUpdate)
	{
		Transform3DComponent& transform = m_world.registry().get<Transform3DComponent>(e);
		PointLightComponent& light = m_world.registry().get<PointLightComponent>(e);

		// Generate shadow cascades
		mat4f shadowProjection = mat4f::perspective(anglef::degree(90.f), 1.f, 0.1f, light.radius);
		point3f lightPos = point3f(transform.transform.cols[3]);
		light.worldToLightSpaceMatrix[0] = shadowProjection * mat4f::inverse(mat4f::lookAt(lightPos, lightPos + vec3f(1.0, 0.0, 0.0), norm3f(0.0, -1.0, 0.0)));
		light.worldToLightSpaceMatrix[1] = shadowProjection * mat4f::inverse(mat4f::lookAt(lightPos, lightPos + vec3f(-1.0, 0.0, 0.0), norm3f(0.0, -1.0, 0.0)));
		light.worldToLightSpaceMatrix[2] = shadowProjection * mat4f::inverse(mat4f::lookAt(lightPos, lightPos + vec3f(0.0, 1.0, 0.0), norm3f(0.0, 0.0, 1.0)));
		light.worldToLightSpaceMatrix[3] = shadowProjection * mat4f::inverse(mat4f::lookAt(lightPos, lightPos + vec3f(0.0, -1.0, 0.0), norm3f(0.0, 0.0, -1.0)));
		light.worldToLightSpaceMatrix[4] = shadowProjection * mat4f::inverse(mat4f::lookAt(lightPos, lightPos + vec3f(0.0, 0.0, 1.0), norm3f(0.0, -1.0, 0.0)));
		light.worldToLightSpaceMatrix[5] = shadowProjection * mat4f::inverse(mat4f::lookAt(lightPos, lightPos + vec3f(0.0, 0.0, -1.0), norm3f(0.0, -1.0, 0.0)));

		RenderPass shadowPass;
		shadowPass.framebuffer = m_shadowFramebuffer;
		shadowPass.material = m_shadowPointMaterial;
		shadowPass.clear = Clear{ ClearMask::None, color4f(1.f), 1.f, 0 };
		shadowPass.blend = Blending::none();
		shadowPass.depth = Depth{ DepthCompare::Less, true };
		shadowPass.cull = Culling{ CullMode::BackFace, CullOrder::CounterClockWise };
		shadowPass.stencil = Stencil::none();
		shadowPass.viewport = aka::Rect{ 0 };
		shadowPass.scissor = aka::Rect{ 0 };

		m_shadowFramebuffer->attachment(FramebufferAttachmentType::Depth, light.shadowMap);
		m_shadowFramebuffer->clear(color4f(1.f), 1.f, 0, ClearMask::Depth);
		m_shadowPointMaterial->set<mat4f>("u_lights[0]", light.worldToLightSpaceMatrix, 6);
		m_shadowPointMaterial->set<vec3f>("u_lightPos", vec3f(transform.transform.cols[3]));
		m_shadowPointMaterial->set<float>("u_far", light.radius);
		auto view = m_world.registry().view<Transform3DComponent, MeshComponent>();
		view.each([&](const Transform3DComponent& transform, const MeshComponent& mesh) {
			m_shadowPointMaterial->set<mat4f>("u_model", transform.transform);
			shadowPass.submesh = mesh.submesh;
			shadowPass.execute();
		});
		m_world.registry().remove<DirtyLightComponent>(e);
	}

	for (entt::entity e : dirLightUpdate)
	{
		DirectionalLightComponent& light = m_world.registry().get<DirectionalLightComponent>(e);

		const float offset[DirectionalLightComponent::cascadeCount + 1] = { m_projection.nearZ, m_projection.farZ / 20.f, m_projection.farZ / 5.f, m_projection.farZ };
		// Generate shadow cascades
		for (size_t i = 0; i < DirectionalLightComponent::cascadeCount; i++)
		{
			float w = (float)width();
			float h = (float)height();
			float n = offset[i];
			float f = offset[i + 1];
			mat4f p = mat4f::perspective(m_projection.hFov, w / h, n, f);
			light.worldToLightSpaceMatrix[i] = computeShadowViewProjectionMatrix(view, p, light.shadowMap[i]->width(), n, f, light.direction);
			vec4f clipSpace = perspective * vec4f(0.f, 0.f, -offset[i + 1], 1.f);
			light.cascadeEndClipSpace[i] = clipSpace.z / clipSpace.w;
		}

		RenderPass shadowPass;
		shadowPass.framebuffer = m_shadowFramebuffer;
		shadowPass.material = m_shadowMaterial;
		shadowPass.clear = Clear{ ClearMask::None, color4f(1.f), 1.f, 0 };
		shadowPass.blend = Blending::none();
		shadowPass.depth = Depth{ DepthCompare::Less, true };
		shadowPass.cull = Culling{ CullMode::BackFace, CullOrder::CounterClockWise };
		shadowPass.stencil = Stencil::none();
		shadowPass.viewport = aka::Rect{ 0 };
		shadowPass.scissor = aka::Rect{ 0 };
		for (size_t i = 0; i < DirectionalLightComponent::cascadeCount; i++)
		{
			m_shadowFramebuffer->attachment(FramebufferAttachmentType::Depth, light.shadowMap[i]);
			m_shadowFramebuffer->clear(color4f(1.f), 1.f, 0, ClearMask::Depth);
			m_shadowMaterial->set<mat4f>("u_light", light.worldToLightSpaceMatrix[i]);
			auto view = m_world.registry().view<Transform3DComponent, MeshComponent>();
			view.each([&](const Transform3DComponent& transform, const MeshComponent& mesh) {
				// TODO separate light view matrix to be able to perform culling here
				m_shadowMaterial->set<mat4f>("u_model", transform.transform);
				shadowPass.submesh = mesh.submesh;
				shadowPass.execute();
			});
		}
		m_world.registry().remove<DirtyLightComponent>(e);
	}

	// --- Shadow pass
	mat4f renderView = view;
	mat4f renderPerspective = perspective;
	auto renderableView = m_world.registry().view<Transform3DComponent, MeshComponent, MaterialComponent>();

	// --- G-Buffer pass
	if (Keyboard::pressed(KeyboardKey::ControlLeft))
	{
		renderView = debugView;
		renderPerspective = debugPerspective;
	}
	// TODO depth prepass
	RenderPass gbufferPass;
	gbufferPass.framebuffer = m_gbuffer;
	gbufferPass.material = m_gbufferMaterial;
	gbufferPass.clear = Clear{ ClearMask::None, color4f(1.f), 1.f, 0 };
	gbufferPass.blend = Blending::none();
	gbufferPass.depth = Depth{ DepthCompare::Less, true };
	gbufferPass.stencil = Stencil::none();
	gbufferPass.viewport = aka::Rect{ 0 };
	gbufferPass.scissor = aka::Rect{ 0 };

	m_gbufferMaterial->set<mat4f>("u_view", renderView);
	m_gbufferMaterial->set<mat4f>("u_projection", renderPerspective);

	m_gbuffer->clear(color4f(0.f), 1.f, 0, ClearMask::All);
		
	renderableView.each([&](const Transform3DComponent& transform, const MeshComponent& mesh, const MaterialComponent& material) {
		frustum<>::planes p = frustum<>::extract(perspective * view);
		// Check intersection in camera space
		// https://www.gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf
		if (!p.intersect(transform.transform * mesh.bounds))
			return;

		aka::mat4f model = transform.transform;
		aka::mat3f normal = aka::mat3f::transpose(aka::mat3f::inverse(mat3f(model)));
		aka::color4f color = material.color;
		m_gbufferMaterial->set<mat4f>("u_model", model);
		m_gbufferMaterial->set<mat3f>("u_normalMatrix", normal);
		m_gbufferMaterial->set<color4f>("u_color", color);
		m_gbufferMaterial->set<Texture::Ptr>("u_roughnessTexture", material.roughnessTexture);
		m_gbufferMaterial->set<Texture::Ptr>("u_colorTexture", material.colorTexture);
		m_gbufferMaterial->set<Texture::Ptr>("u_normalTexture", material.normalTexture);

		m_gbufferMaterial->set<mat4f>("u_model", model);
		gbufferPass.submesh = mesh.submesh;
		gbufferPass.cull = Culling{ CullMode::BackFace, CullOrder::CounterClockWise };

		gbufferPass.execute();
	});

	// --- Lighting pass
	// We need :
	// A directional light pass into storage buffer (using quad)
	// A point light pass into storage buffer (using volumetric sphere & culling)
	// A tonemapping pass into storage buffer
	// This way, we can handle an infinite amount of light
	static const mat4f projectionToTextureCoordinateMatrix(
		col4f(0.5, 0.0, 0.0, 0.0),
		col4f(0.0, 0.5, 0.0, 0.0),
		col4f(0.0, 0.0, 0.5, 0.0),
		col4f(0.5, 0.5, 0.5, 1.0)
	);

	m_storageFramebuffer->clear(color4f(0.f, 0.f, 0.f, 1.f), 1.f, 1, ClearMask::Color);

	RenderPass lightingPass;
	lightingPass.framebuffer = m_storageFramebuffer;
	lightingPass.submesh.mesh = m_quad;
	lightingPass.submesh.type = PrimitiveType::Triangles;
	lightingPass.submesh.offset = 0;
	lightingPass.submesh.count = 6;
	lightingPass.clear = Clear{ ClearMask::None, color4f(0.f), 1.f, 0 };
	lightingPass.blend.colorModeSrc = BlendMode::One;
	lightingPass.blend.colorModeDst = BlendMode::One;
	lightingPass.blend.colorOp = BlendOp::Add;
	lightingPass.blend.alphaModeSrc = BlendMode::One;
	lightingPass.blend.alphaModeDst = BlendMode::Zero;
	lightingPass.blend.alphaOp = BlendOp::Add;
	lightingPass.blend.mask = BlendMask::Rgb;
	lightingPass.blend.blendColor = color32(255);
	lightingPass.depth = Depth{ DepthCompare::None, false };
	lightingPass.stencil = Stencil::none();
	lightingPass.viewport = aka::Rect{ 0 };
	lightingPass.scissor = aka::Rect{ 0 };
	lightingPass.cull = Culling{ CullMode::BackFace, CullOrder::CounterClockWise };

	// --- Ambient light
	lightingPass.material = m_ambientMaterial;
	lightingPass.material->set<Texture::Ptr>("u_position", m_position);
	lightingPass.material->set<Texture::Ptr>("u_albedo", m_albedo);
	lightingPass.material->set<Texture::Ptr>("u_normal", m_normal);
	lightingPass.material->set<Texture::Ptr>("u_skybox", m_skybox);
	lightingPass.material->set<vec3f>("u_cameraPos", vec3f(m_camera.get<Transform3DComponent>().transform[3]));
	lightingPass.execute();
	
	// --- Directional lights
	lightingPass.material = m_dirMaterial;
	lightingPass.material->set<Texture::Ptr>("u_position", m_position);
	lightingPass.material->set<Texture::Ptr>("u_albedo", m_albedo);
	lightingPass.material->set<Texture::Ptr>("u_normal", m_normal);
	lightingPass.material->set<Texture::Ptr>("u_depth", m_depth);
	lightingPass.material->set<Texture::Ptr>("u_roughness", m_roughness);
	lightingPass.material->set<vec3f>("u_cameraPos", vec3f(m_camera.get<Transform3DComponent>().transform[3]));
	auto directionalShadows = m_world.registry().view<Transform3DComponent, DirectionalLightComponent>();
	directionalShadows.each([&](const Transform3DComponent& transform, DirectionalLightComponent& light) {
		mat4f worldToLightTextureSpaceMatrix[DirectionalLightComponent::cascadeCount];
		for (size_t i = 0; i < DirectionalLightComponent::cascadeCount; i++)
			worldToLightTextureSpaceMatrix[i] = projectionToTextureCoordinateMatrix * light.worldToLightSpaceMatrix[i];
		lightingPass.material->set<vec3f>("u_lightDirection", light.direction);
		lightingPass.material->set<float>("u_lightIntensity", light.intensity);
		lightingPass.material->set<color3f>("u_lightColor", light.color);
		lightingPass.material->set<mat4f>("u_worldToLightTextureSpace[0]", worldToLightTextureSpaceMatrix, DirectionalLightComponent::cascadeCount);
		lightingPass.material->set<float>("u_cascadeEndClipSpace[0]", light.cascadeEndClipSpace, DirectionalLightComponent::cascadeCount);
		lightingPass.material->set<Texture::Ptr>("u_shadowMap[0]", light.shadowMap, DirectionalLightComponent::cascadeCount);
		lightingPass.execute();
	});

	// --- Point lights
	// Using light volumes
	lightingPass.submesh = SubMesh{ m_sphere, PrimitiveType::Triangles, m_sphere->getIndexCount(), 0 };
	lightingPass.cull = Culling{ CullMode::FrontFace, CullOrder::CounterClockWise}; // Important to avoid rendering 2 times or clipping
	lightingPass.material = m_pointMaterial;
	lightingPass.material->set<Texture::Ptr>("u_position", m_position);
	lightingPass.material->set<Texture::Ptr>("u_albedo", m_albedo);
	lightingPass.material->set<Texture::Ptr>("u_normal", m_normal);
	lightingPass.material->set<Texture::Ptr>("u_roughness", m_roughness);
	lightingPass.material->set<vec2f>("u_screen", vec2f(m_storage->width(), m_storage->height()));
	lightingPass.material->set<vec3f>("u_cameraPos", vec3f(m_camera.get<Transform3DComponent>().transform[3]));
	auto pointShadows = m_world.registry().view<Transform3DComponent, PointLightComponent>();
	pointShadows.each([&](const Transform3DComponent& transform, PointLightComponent& light) {
		// TODO use frustum culling;
		mat4f model = transform.transform * mat4f::scale(vec3f(light.radius));
		lightingPass.material->set<mat4f>("u_mvp", perspective * view * model);
		lightingPass.material->set<vec3f>("u_lightPosition", vec3f(transform.transform.cols[3]));
		lightingPass.material->set<float>("u_lightIntensity", light.intensity);
		lightingPass.material->set<color3f>("u_lightColor", light.color);
		lightingPass.material->set<Texture::Ptr>("u_shadowMap", light.shadowMap);
		lightingPass.material->set<float>("u_farPointLight", light.radius);
		lightingPass.execute();
	});

	m_storageFramebuffer->blit(m_gbuffer, FramebufferAttachmentType::DepthStencil, Sampler::Filter::Nearest);
	
	// --- Skybox pass
	RenderPass skyboxPass;
	skyboxPass.framebuffer = m_storageFramebuffer;
	skyboxPass.submesh.type = PrimitiveType::Triangles;
	skyboxPass.submesh.offset = 0;
	skyboxPass.submesh.count = m_cube->getVertexCount();
	skyboxPass.submesh.mesh = m_cube;
	skyboxPass.material = m_skyboxMaterial;
	skyboxPass.clear = Clear{ ClearMask::None, color4f(0.f), 1.f, 0 };
	skyboxPass.blend = Blending::none();
	skyboxPass.depth = Depth{ DepthCompare::LessOrEqual, false };
	skyboxPass.stencil = Stencil::none();
	skyboxPass.viewport = aka::Rect{ 0 };
	skyboxPass.scissor = aka::Rect{ 0 };
	skyboxPass.cull = Culling{ CullMode::BackFace, CullOrder::CounterClockWise };

	skyboxPass.material->set<Texture::Ptr>("u_skybox", m_skybox);
	skyboxPass.material->set<mat4f>("u_view", mat4f(mat3f(renderView)));
	skyboxPass.material->set<mat4f>("u_projection", renderPerspective);

	skyboxPass.execute();

	// --- Post process pass
	RenderPass postProcessPass;
	postProcessPass.framebuffer = backbuffer;
	postProcessPass.submesh.type = PrimitiveType::Triangles;
	postProcessPass.submesh.offset = 0;
	postProcessPass.submesh.count = m_quad->getIndexCount();
	postProcessPass.submesh.mesh = m_quad;
	postProcessPass.material = m_postprocessMaterial;
	postProcessPass.clear = Clear{ ClearMask::None, color4f(0.f), 1.f, 0 };
	postProcessPass.blend = Blending::none();
	postProcessPass.depth = Depth{ DepthCompare::None, false };
	postProcessPass.stencil = Stencil::none();
	postProcessPass.viewport = aka::Rect{ 0 };
	postProcessPass.scissor = aka::Rect{ 0 };
	postProcessPass.cull = Culling{ CullMode::BackFace, CullOrder::CounterClockWise };

	postProcessPass.material->set<Texture::Ptr>("u_input", m_storage);
	postProcessPass.material->set<uint32_t>("u_width", width());
	postProcessPass.material->set<uint32_t>("u_height", height());

	postProcessPass.execute();

	if (m_debug)
	{
		// Blit depth to be used by debug pass
		backbuffer->blit(m_storageFramebuffer, FramebufferAttachmentType::DepthStencil, Sampler::Filter::Nearest);

		// --- Editor pass
		for (EditorWindow* editor : m_editors)
			editor->onRender(m_world);
	}
}

};
