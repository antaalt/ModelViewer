#include "ModelViewer.h"

#include <imgui.h>
#include <imguizmo.h>
#include <Aka/Layer/ImGuiLayer.h>

namespace viewer {

void Viewer::loadShader()
{
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
		aka::ShaderID vert = aka::Shader::compile(File::readString(Asset::path("shaders/GL/quad.vert")), aka::ShaderType::Vertex);
		aka::ShaderID frag = aka::Shader::compile(File::readString(Asset::path("shaders/GL/shading.frag")), aka::ShaderType::Fragment);
#else
		std::string str = File::readString(Asset::path("shaders/D3D/shading.hlsl"));
		aka::ShaderID vert = aka::Shader::compile(str, aka::ShaderType::Vertex);
		aka::ShaderID frag = aka::Shader::compile(str, aka::ShaderType::Fragment);
#endif
		if (vert == ShaderID(0) || frag == ShaderID(0))
		{
			aka::Logger::error("Failed to compile lighting shader");
		}
		else
		{
			aka::Shader::Ptr shader = aka::Shader::create(vert, frag, attributes);
			if (shader->valid())
				m_lightingMaterial = aka::ShaderMaterial::create(shader);
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
		aka::ShaderID vert = aka::Shader::compile(File::readString(Asset::path("shaders/GL/gltf.vert")), aka::ShaderType::Vertex);
		aka::ShaderID frag = aka::Shader::compile(File::readString(Asset::path("shaders/GL/gltf.frag")), aka::ShaderType::Fragment);
#else
		std::string str = File::readString(Asset::path("shaders/D3D/shader.hlsl"));
		aka::ShaderID vert = aka::Shader::compile(str, aka::ShaderType::Vertex);
		aka::ShaderID frag = aka::Shader::compile(str, aka::ShaderType::Fragment);
#endif
		if (vert == ShaderID(0) || frag == ShaderID(0))
		{
			aka::Logger::error("Failed to compile default shader");
		}
		else
		{
			aka::Shader::Ptr shader = aka::Shader::create(vert, frag, attributes);
			if (shader->valid())
				m_material = aka::ShaderMaterial::create(shader);
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
		aka::ShaderID frag = aka::Shader::compile(File::readString(Asset::path("shaders/GL/fxaa.frag")), aka::ShaderType::Fragment);
#else
		std::string str = File::readString(Asset::path("shaders/D3D/fxaa.hlsl"));
		aka::ShaderID vert = aka::Shader::compile(str, aka::ShaderType::Vertex);
		aka::ShaderID frag = aka::Shader::compile(str, aka::ShaderType::Fragment);
#endif
		if (vert == ShaderID(0) || frag == ShaderID(0))
		{
			aka::Logger::error("Failed to compile fxaa shader");
		}
		else
		{
			aka::Shader::Ptr shader = aka::Shader::create(vert, frag, attributes);
			if (shader->valid())
				m_fxaaMaterial = aka::ShaderMaterial::create(shader);
		}
	}
}

void Viewer::onCreate()
{
	m_world.attach<ArcballCameraSystem>();
	m_world.attach<Scene::GraphSystem>();
	m_world.create();
	StopWatch<> stopWatch;
	// TODO use args
	bool loaded = false;
	//loaded = ModelLoader::load(Asset::path("glTF-Sample-Models/2.0/Sponza/glTF/Sponza.gltf"), m_world);
	//loaded = ModelLoader::load(Asset::path("glTF-Sample-Models/2.0/AlphaBlendModeTest/glTF/AlphaBlendModeTest.gltf"), m_world);
	//loaded = ModelLoader::load(Asset::path("glTF-Sample-Models/2.0/CesiumMilkTruck/glTF/CesiumMilkTruck.gltf"), m_world);
	loaded = ModelLoader::load(Asset::path("glTF-Sample-Models/2.0/Lantern/glTF/Lantern.gltf"), m_world);
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
	m_quad = Mesh::create();
	VertexData dataQuad;
	dataQuad.attributes.push_back(VertexData::Attribute{ 0, VertexFormat::Float, VertexType::Vec2 });
	float quadVertices[] = {
		-1, -1, // bottom left corner
		 1, -1,  // bottom right corner
		 1,  1, // top right corner
		-1,  1, // top left corner
	};
	uint8_t indices[] = { 0,1,2,0,2,3 };
	m_quad->vertices(dataQuad, quadVertices, 4);
	m_quad->indices(IndexFormat::UnsignedByte, indices, 6);

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

	m_cube = Mesh::create();
	VertexData dataSkybox;
	dataSkybox.attributes.push_back(VertexData::Attribute{ 0, VertexFormat::Float, VertexType::Vec3 });
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
	m_cube->vertices(dataSkybox, skyboxVertices, 36);

	// --- Forward pass 

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
	if (Keyboard::down(KeyboardKey::R))
	{
		m_camera.get<ArcballCameraComponent>().set(m_bounds);
		auto dirLightUpdate = m_world.registry().view<DirectionalLightComponent>();
		for (entt::entity e : dirLightUpdate)
			if (!m_world.registry().has<DirtyLightComponent>(e))
				m_world.registry().emplace<DirtyLightComponent>(e);
	}
	if (Keyboard::down(KeyboardKey::D))
	{
		m_debug = !m_debug;
	}
	if (Keyboard::down(KeyboardKey::PrintScreen))
	{
		GraphicBackend::screenshot("screen.png");
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
	m_world.update(deltaTime);
	// check if main camera was updated and set dirty flag on dir lights
	if (m_camera.has<DirtyCameraComponent>())
	{
		// TODO check if main camera moved only, add to all dir lights
		auto dirLightUpdate = m_world.registry().view<DirectionalLightComponent>();
		for (entt::entity e : dirLightUpdate)
			if (!m_world.registry().has<DirtyLightComponent>(e))
				m_world.registry().emplace<DirtyLightComponent>(e);
		m_camera.remove<DirtyCameraComponent>();
	}
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
	// TODO use camera (Arcball inherit camera ?)
	mat4f debugPerspective = mat4f::perspective(m_projection.hFov, (float)backbuffer->width() / (float)backbuffer->height(), 0.01f, 1000.f);
	mat4f perspective = mat4f::perspective(m_projection.hFov, (float)backbuffer->width() / (float)backbuffer->height(), m_projection.nearZ, m_projection.farZ);

	// --- Shadow map system
	auto pointLightUpdate = m_world.registry().view<DirtyLightComponent, PointLightComponent>();
	auto dirLightUpdate = m_world.registry().view<DirtyLightComponent, DirectionalLightComponent>();
	for (entt::entity e : pointLightUpdate)
	{
		Transform3DComponent& transform = m_world.registry().get<Transform3DComponent>(e);
		PointLightComponent& light = m_world.registry().get<PointLightComponent>(e);

		// Generate shadow cascades
		mat4f shadowProjection = mat4f::perspective(anglef::degree(90.f), 1.f, 1.f, PointLightComponent::far);
		point3f lightPos = point3f(transform.transform.cols[3]);
		light.worldToLightSpaceMatrix[0] = shadowProjection * mat4f::inverse(mat4f::lookAt(lightPos, lightPos + vec3f(1.0, 0.0, 0.0), norm3f(0.0, -1.0, 0.0)));
		light.worldToLightSpaceMatrix[1] = shadowProjection * mat4f::inverse(mat4f::lookAt(lightPos, lightPos + vec3f(-1.0, 0.0, 0.0), norm3f(0.0, -1.0, 0.0)));
		light.worldToLightSpaceMatrix[2] = shadowProjection * mat4f::inverse(mat4f::lookAt(lightPos, lightPos + vec3f(0.0, 1.0, 0.0), norm3f(0.0, 0.0, 1.0)));
		light.worldToLightSpaceMatrix[3] = shadowProjection * mat4f::inverse(mat4f::lookAt(lightPos, lightPos + vec3f(0.0, -1.0, 0.0), norm3f(0.0, 0.0, -1.0)));
		light.worldToLightSpaceMatrix[4] = shadowProjection * mat4f::inverse(mat4f::lookAt(lightPos, lightPos + vec3f(0.0, 0.0, 1.0), norm3f(0.0, -1.0, 0.0)));
		light.worldToLightSpaceMatrix[5] = shadowProjection * mat4f::inverse(mat4f::lookAt(lightPos, lightPos + vec3f(0.0, 0.0, -1.0), norm3f(0.0, -1.0, 0.0)));

		RenderPass shadowPass;
		shadowPass.framebuffer = m_shadowFramebuffer;
		shadowPass.submesh.type = PrimitiveType::Triangles;
		shadowPass.submesh.indexOffset = 0;
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
		m_shadowPointMaterial->set<float>("u_far", PointLightComponent::far);
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
		shadowPass.submesh.type = PrimitiveType::Triangles;
		shadowPass.submesh.indexOffset = 0;
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
	const mat4f projectionToTextureCoordinateMatrix(
		col4f(0.5, 0.0, 0.0, 0.0),
		col4f(0.0, 0.5, 0.0, 0.0),
		col4f(0.0, 0.0, 0.5, 0.0),
		col4f(0.5, 0.5, 0.5, 1.0)
	);
	mat4f renderView = view;
	mat4f renderPerspective = perspective;
	auto renderableView = m_world.registry().view<Transform3DComponent, MeshComponent, MaterialComponent>();

	if (!Keyboard::pressed(KeyboardKey::AltLeft))
	{
		// --- G-Buffer pass
		if (Keyboard::pressed(KeyboardKey::ControlLeft))
		{
			renderView = debugView;
			renderPerspective = debugPerspective;
		}
		// TODO depth prepass
		RenderPass gbufferPass;
		gbufferPass.framebuffer = m_gbuffer;
		gbufferPass.submesh.type = PrimitiveType::Triangles;
		gbufferPass.submesh.indexOffset = 0;
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
		// TODO We can use compute here
		RenderPass lightingPass;
		lightingPass.framebuffer = m_storageFramebuffer;
		lightingPass.submesh.type = PrimitiveType::Triangles;
		lightingPass.submesh.indexOffset = 0;
		lightingPass.material = m_lightingMaterial;
		lightingPass.clear = Clear{ ClearMask::All, color4f(0.f), 1.f, 0 };
		lightingPass.blend = Blending::none();
		lightingPass.depth = Depth{ DepthCompare::None, false };
		lightingPass.stencil = Stencil::none();
		lightingPass.viewport = aka::Rect{ 0 };
		lightingPass.scissor = aka::Rect{ 0 };
		lightingPass.submesh.mesh = m_quad;
		lightingPass.submesh.indexCount = m_quad->getIndexCount(); // TODO set zero means all ?
		lightingPass.cull = Culling{ CullMode::BackFace, CullOrder::CounterClockWise };

		m_lightingMaterial->set<Texture::Ptr>("u_position", m_position);
		m_lightingMaterial->set<Texture::Ptr>("u_albedo", m_albedo);
		m_lightingMaterial->set<Texture::Ptr>("u_normal", m_normal);
		m_lightingMaterial->set<Texture::Ptr>("u_depth", m_depth);
		m_lightingMaterial->set<Texture::Ptr>("u_roughness", m_roughness);
		m_lightingMaterial->set<Texture::Ptr>("u_skybox", m_skybox);
		m_lightingMaterial->set<vec3f>("u_cameraPos", vec3f(m_camera.get<Transform3DComponent>().transform[3]));
		m_lightingMaterial->set<float>("u_farPointLight", PointLightComponent::far);
		int count = 0;
		auto directionalShadows = m_world.registry().view<Transform3DComponent, DirectionalLightComponent>();
		directionalShadows.each([&](const Transform3DComponent& transform, DirectionalLightComponent& light) {
			mat4f worldToLightTextureSpaceMatrix[DirectionalLightComponent::cascadeCount];
			for (size_t i = 0; i < DirectionalLightComponent::cascadeCount; i++)
				worldToLightTextureSpaceMatrix[i] = projectionToTextureCoordinateMatrix * light.worldToLightSpaceMatrix[i];
			std::string index = std::to_string(count);
			m_lightingMaterial->set<vec3f>(("u_dirLights[" + index + "].direction").c_str(), light.direction);
			m_lightingMaterial->set<float>(("u_dirLights[" + index + "].intensity").c_str(), light.intensity);
			m_lightingMaterial->set<color3f>(("u_dirLights[" + index + "].color").c_str(), light.color);
			m_lightingMaterial->set<mat4f>(("u_dirLights[" + index + "].worldToLightTextureSpace[0]").c_str(), worldToLightTextureSpaceMatrix, DirectionalLightComponent::cascadeCount);
			m_lightingMaterial->set<float>(("u_dirLights[" + index + "].cascadeEndClipSpace[0]").c_str(), light.cascadeEndClipSpace, DirectionalLightComponent::cascadeCount);
			m_lightingMaterial->set<Texture::Ptr>(("u_dirLights[" + index + "].shadowMap[0]").c_str(), light.shadowMap, DirectionalLightComponent::cascadeCount);
			count++;
		});
		m_lightingMaterial->set<int>("u_dirLightCount", count);
		count = 0;
		auto pointShadows = m_world.registry().view<Transform3DComponent, PointLightComponent>();
		pointShadows.each([&](const Transform3DComponent& transform, PointLightComponent& light) {
			mat4f worldToLightTextureSpaceMatrix[6];
			for (size_t i = 0; i < 6; i++)
				worldToLightTextureSpaceMatrix[i] = projectionToTextureCoordinateMatrix * light.worldToLightSpaceMatrix[i];
			std::string index = std::to_string(count);
			m_lightingMaterial->set<vec3f>(("u_pointLights[" + index + "].position").c_str(), vec3f(transform.transform.cols[3]));
			m_lightingMaterial->set<float>(("u_pointLights[" + index + "].intensity").c_str(), light.intensity);
			m_lightingMaterial->set<color3f>(("u_pointLights[" + index + "].color").c_str(), light.color);
			//m_lightingMaterial->set<mat4f>(("u_pointLights[" + index + "].worldToLightTextureSpace[0]").c_str(), worldToLightTextureSpaceMatrix, 6);
			m_lightingMaterial->set<Texture::Ptr>(("u_pointLights[" + index + "].shadowMap").c_str(), light.shadowMap);
			count++;
		});
		m_lightingMaterial->set<int>("u_pointLightCount", count);

		lightingPass.execute();

		m_storageFramebuffer->blit(m_gbuffer, FramebufferAttachmentType::DepthStencil, Sampler::Filter::Nearest);
	}
	else
	{
		// --- Forward pass
		m_storageFramebuffer->clear(color4f(0.f, 0.f, 0.f, 1.f), 1.f, 0, ClearMask::All);

		if (Keyboard::pressed(KeyboardKey::ControlLeft))
		{
			renderView = debugView;
			renderPerspective = debugPerspective;
		}
		mat4f worldToLightTextureSpaceMatrix[DirectionalLightComponent::cascadeCount];
		for (size_t i = 0; i < DirectionalLightComponent::cascadeCount; i++)
			worldToLightTextureSpaceMatrix[i] = projectionToTextureCoordinateMatrix * m_sun.get<DirectionalLightComponent>().worldToLightSpaceMatrix[i];
		m_material->set<mat4f>("u_view", renderView);
		m_material->set<mat4f>("u_projection", renderPerspective);
		m_material->set<mat4f>("u_light[0]", worldToLightTextureSpaceMatrix, DirectionalLightComponent::cascadeCount);
		m_material->set<vec3f>("u_lightDir", m_sun.get<DirectionalLightComponent>().direction);
		m_material->set<Texture::Ptr>("u_shadowTexture[0]", m_sun.get<DirectionalLightComponent>().shadowMap, DirectionalLightComponent::cascadeCount);
		m_material->set<float>("u_cascadeEndClipSpace[0]", m_sun.get<DirectionalLightComponent>().cascadeEndClipSpace, DirectionalLightComponent::cascadeCount);

		RenderPass renderPass{};
		renderPass.framebuffer = m_storageFramebuffer;
		renderPass.submesh.type = PrimitiveType::Triangles;
		renderPass.submesh.indexOffset = 0;
		renderPass.material = m_material;
		renderPass.clear = Clear{ ClearMask::None, color4f(1.f), 1.f, 0 };
		renderPass.blend = Blending::nonPremultiplied();
		renderPass.depth = Depth{ DepthCompare::Less, true };
		renderPass.stencil = Stencil::none();
		renderPass.viewport = aka::Rect{ 0 };
		renderPass.scissor = aka::Rect{ 0 };

		renderableView.each([&](const Transform3DComponent& transform, const MeshComponent& mesh, const MaterialComponent& material) {
			frustum<>::planes p = frustum<>::extract(perspective * view);
			if (!p.intersect(transform.transform * mesh.bounds))
				return;
			aka::mat4f model = transform.transform;
			aka::mat3f normal = aka::mat3f::transpose(aka::mat3f::inverse(mat3f(model)));
			aka::color4f color = material.color;
			m_material->set<mat4f>("u_model", model);
			m_material->set<mat3f>("u_normalMatrix", normal);
			m_material->set<color4f>("u_color", color);
			m_material->set<Texture::Ptr>("u_colorTexture", material.colorTexture);
			m_material->set<Texture::Ptr>("u_normalTexture", material.normalTexture);
			renderPass.submesh = mesh.submesh;
			renderPass.cull = material.doubleSided ? Culling{ CullMode::None, CullOrder::CounterClockWise } : Culling{ CullMode::BackFace, CullOrder::CounterClockWise };

			renderPass.execute();
		});
	}
	// --- Skybox pass
	RenderPass skyboxPass;
	skyboxPass.framebuffer = m_storageFramebuffer;
	skyboxPass.submesh.type = PrimitiveType::Triangles;
	skyboxPass.submesh.indexOffset = 0;
	skyboxPass.submesh.indexCount = m_cube->getIndexCount();
	skyboxPass.submesh.mesh = m_cube;
	skyboxPass.material = m_skyboxMaterial;
	skyboxPass.clear = Clear{ ClearMask::None, color4f(0.f), 1.f, 0 };
	skyboxPass.blend = Blending::none();
	skyboxPass.depth = Depth{ DepthCompare::LessOrEqual, false };
	skyboxPass.stencil = Stencil::none();
	skyboxPass.viewport = aka::Rect{ 0 };
	skyboxPass.scissor = aka::Rect{ 0 };
	skyboxPass.cull = Culling{ CullMode::BackFace, CullOrder::CounterClockWise };

	m_skyboxMaterial->set<Texture::Ptr>("u_skybox", m_skybox);
	m_skyboxMaterial->set<mat4f>("u_view", mat4f(mat3f(renderView)));
	m_skyboxMaterial->set<mat4f>("u_projection", renderPerspective);

	skyboxPass.execute();

	// --- FXAA pass
	RenderPass fxaaPass;
	fxaaPass.framebuffer = backbuffer;
	fxaaPass.submesh.type = PrimitiveType::Triangles;
	fxaaPass.submesh.indexOffset = 0;
	fxaaPass.submesh.indexCount = m_quad->getIndexCount();
	fxaaPass.submesh.mesh = m_quad;
	fxaaPass.material = m_fxaaMaterial;
	fxaaPass.clear = Clear{ ClearMask::None, color4f(0.f), 1.f, 0 };
	fxaaPass.blend = Blending::none();
	fxaaPass.depth = Depth{ DepthCompare::None, false };
	fxaaPass.stencil = Stencil::none();
	fxaaPass.viewport = aka::Rect{ 0 };
	fxaaPass.scissor = aka::Rect{ 0 };
	fxaaPass.cull = Culling{ CullMode::BackFace, CullOrder::CounterClockWise };

	m_fxaaMaterial->set<Texture::Ptr>("u_input", m_storage);
	m_fxaaMaterial->set<uint32_t>("u_width", width());
	m_fxaaMaterial->set<uint32_t>("u_height", height());

	fxaaPass.execute();

	// --- Debug pass
	if (m_debug)
	{
		ImGuizmo::BeginFrame();
		if (ImGui::Begin("Info"))
		{
			ImGuiIO& io = ImGui::GetIO();
			ImGui::Text("Resolution : %ux%u", width(), height());
			ImGui::Text("%.3f ms/frame", 1000.0f / ImGui::GetIO().Framerate);
			ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
			// TODO toggle fullscreen, vsync
		}
		ImGui::End();

		if (ImGui::Begin("Camera"))
		{
			float fov = m_projection.hFov.radian();
			if (ImGui::SliderAngle("Fov", &fov, 10.f, 160.f))
				m_projection.hFov = anglef::radian(fov);
			ImGui::SliderFloat("Near", &m_projection.nearZ, 0.001f, 10.f);
			ImGui::SliderFloat("Far", &m_projection.farZ, 10.f, 1000.f);
			//imguizmo
		}
		ImGui::End();

		if (ImGui::Begin("Scene"))
		{
			auto graphView = m_world.registry().view<Transform3DComponent, Hierarchy3DComponent>();
			// TODO do not compute child map every cycle
			std::map<entt::entity, std::vector<entt::entity>> childrens;
			std::vector<entt::entity> roots;
			for (entt::entity entity : graphView)
			{
				const Hierarchy3DComponent& h = m_world.registry().get<Hierarchy3DComponent>(entity);
				if (h.parent == Entity::null())
					roots.push_back(entity);
				else
					childrens[h.parent.handle()].push_back(entity);
			}
			// --- Graph
			static entt::entity selected = entt::null;
			std::function<void(entt::entity)> recurse = [&](entt::entity entity)
			{
				const TagComponent& tag = m_world.registry().get<TagComponent>(entity);

				auto it = childrens.find(entity);
				if (it != childrens.end())
				{
					char buffer[256];
					int err = snprintf(buffer, 256, "%s##%p", tag.name.cstr(), &tag);
					ImGuiTreeNodeFlags flags = 0;
					if (entity == selected)
						flags |= ImGuiTreeNodeFlags_Selected;
					if (ImGui::TreeNodeEx(buffer, flags))
					{
						if (ImGui::IsItemClicked())
							selected = entity;
						for (entt::entity e : it->second)
							recurse(e);
						ImGui::TreePop();
					}
				}
				else
				{
					ImGui::Bullet();
					bool isSelected = selected == entity;
					if (ImGui::Selectable(tag.name.cstr(), &isSelected))
						selected = entity;
				}
			};

			ImGui::TextColored(ImVec4(0.93f, 0.04f, 0.26f, 1.f), "Graph");
			if (ImGui::BeginChild("", ImVec2(0, 200), true))
			{
				for (entt::entity e : roots)
					recurse(e);
			}
			ImGui::EndChild();

			// --- Item info
			if (selected != entt::null)
			{
				bool updated = false;
				const TagComponent& tag = m_world.registry().get<TagComponent>(selected);
				Transform3DComponent& transform = m_world.registry().get<Transform3DComponent>(selected);
				const Hierarchy3DComponent& hierarchy = m_world.registry().get<Hierarchy3DComponent>(selected);
				ImGui::TextColored(ImVec4(0.93f, 0.04f, 0.26f, 1.f), tag.name.cstr());
				ImGui::Text("Transform");
				updated |= ImGui::InputFloat4("##col0", transform.transform.cols[0].data);
				updated |= ImGui::InputFloat4("##col1", transform.transform.cols[1].data);
				updated |= ImGui::InputFloat4("##col2", transform.transform.cols[2].data);
				updated |= ImGui::InputFloat4("##col3", transform.transform.cols[3].data);
				ImGuizmo::OPERATION operation = ImGuizmo::OPERATION::TRANSLATE;
				ImGuizmo::MODE mode = ImGuizmo::MODE::WORLD;
				ImGuiIO& io = ImGui::GetIO();
				ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
				updated |= ImGuizmo::Manipulate(view[0].data, perspective[0].data, operation, mode, transform.transform[0].data);

				aabbox<> bounds;				
				if (m_world.registry().has<MeshComponent>(selected) && m_world.registry().has<MaterialComponent>(selected))
				{
					const MeshComponent& mesh = m_world.registry().get<MeshComponent>(selected);
					MaterialComponent& material = m_world.registry().get<MaterialComponent>(selected);
					ImGui::Text("Mesh");
					ImGui::Text("Vertices : %d", mesh.submesh.mesh->getVertexCount());
					ImGui::Text("Indices : %d", mesh.submesh.mesh->getIndexCount());
					// Material
					ImGui::ColorEdit4("Color", material.color.data);
					ImGui::Checkbox("Double sided", &material.doubleSided);
					ImGui::Image((ImTextureID)(uintptr_t)material.colorTexture->handle(), ImVec2(100, 100));
					ImGui::Image((ImTextureID)(uintptr_t)material.normalTexture->handle(), ImVec2(100, 100));
					ImGui::Image((ImTextureID)(uintptr_t)material.roughnessTexture->handle(), ImVec2(100, 100));
					bounds.include(transform.transform* mesh.bounds);
				}
				if (m_world.registry().has<DirectionalLightComponent>(selected))
				{
					DirectionalLightComponent& light = m_world.registry().get<DirectionalLightComponent>(selected);
					ImGui::Text("Directional light");
					if (ImGui::InputFloat3("Direction", light.direction.data))
					{
						updated = true;
						light.direction = vec3f::normalize(light.direction);
					}
					ImGui::ColorEdit3("Color", light.color.data);
					ImGui::SliderFloat("Intensity", &light.intensity, 0.1f, 100.f);
					bounds.include(transform.transform * point3f(1));
					bounds.include(transform.transform * point3f(-1));
					if (updated && !m_world.registry().has<DirtyLightComponent>(selected))
						m_world.registry().emplace<DirtyLightComponent>(selected);
				}
				if (m_world.registry().has<PointLightComponent>(selected))
				{
					PointLightComponent& light = m_world.registry().get<PointLightComponent>(selected);
					ImGui::Text("Point light");
					ImGui::ColorEdit3("Color", light.color.data);
					ImGui::SliderFloat("Intensity", &light.intensity, 0.1f, 100.f);
					bounds.include(transform.transform * point3f(1));
					bounds.include(transform.transform * point3f(-1));
					if (updated && !m_world.registry().has<DirtyLightComponent>(selected))
						m_world.registry().emplace<DirtyLightComponent>(selected);
				}
				if (m_world.registry().has<Camera3DComponent>(selected))
				{
					Camera3DComponent& camera = m_world.registry().get<Camera3DComponent>(selected);
					ImGui::Text("Camera");
					CameraPerspective* p = dynamic_cast<CameraPerspective*>(camera.projection);
					if (p != nullptr)
					{
						float fov = p->hFov.radian();
						if (ImGui::SliderAngle("Fov", &fov, 10.f, 160.f))
						{
							p->hFov = anglef::radian(fov);
							updated = true;
						}
						updated |= ImGui::SliderFloat("Near", &p->nearZ, 0.001f, 10.f);
						updated |= ImGui::SliderFloat("Far", &p->farZ, 10.f, 1000.f);
					}
					
					bounds.include(transform.transform * point3f(1));
					bounds.include(transform.transform * point3f(-1));
					if (updated && !m_world.registry().has<DirtyLightComponent>(selected))
						m_world.registry().emplace<DirtyCameraComponent>(selected);
				}
				Renderer3D::drawAxis(transform.transform);
				Renderer3D::drawTransform(mat4f::translate(vec3f(bounds.center()))* mat4f::scale(bounds.extent() / 2.f));
				Renderer3D::render(GraphicBackend::backbuffer(), renderView, renderPerspective);
				Renderer3D::clear();
			}
		}
		ImGui::End();
	}
}

};
