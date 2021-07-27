#include "ModelViewer.h"

#include <imgui.h>
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
			aka::Shader::Ptr shader = aka::Shader::create(vert, frag, aka::ShaderID(0), attributes);
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
			aka::Shader::Ptr shader = aka::Shader::create(vert, frag, aka::ShaderID(0), attributes);
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
			aka::Shader::Ptr shader = aka::Shader::create(vert, frag, aka::ShaderID(0), attributes);
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
			aka::Shader::Ptr shader = aka::Shader::create(vert, frag, aka::ShaderID(0), attributes);
			if (shader->valid())
				m_shadowMaterial = aka::ShaderMaterial::create(shader);
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
			aka::Shader::Ptr shader = aka::Shader::create(vert, frag, aka::ShaderID(0), attributes);
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
			aka::Shader::Ptr shader = aka::Shader::create(vert, frag, aka::ShaderID(0), attributes);
			if (shader->valid())
				m_fxaaMaterial = aka::ShaderMaterial::create(shader);
		}
	}
}

void Viewer::onCreate()
{
	m_world.attach<SceneGraph>();
	m_world.create();
	StopWatch<> stopWatch;
	// TODO use args
	bool loaded = false;
	//loaded = ModelLoader::load(Asset::path("glTF-Sample-Models/2.0/Sponza/glTF/Sponza.gltf"), m_world);
	//loaded = ModelLoader::load(Asset::path("glTF-Sample-Models/2.0/AlphaBlendModeTest/glTF/AlphaBlendModeTest.gltf"), m_world);
	loaded = ModelLoader::load(Asset::path("glTF-Sample-Models/2.0/CesiumMilkTruck/glTF/CesiumMilkTruck.gltf"), m_world);
	//loaded = ModelLoader::load(Asset::path("glTF-Sample-Models/2.0/Lantern/glTF/Lantern.gltf"), m_world);
	//loaded = ModelLoader::load(Asset::path("glTF-Sample-Models/2.0/EnvironmentTest/glTF/EnvironmentTest.gltf"), m_world);
	if (!loaded)
		throw std::runtime_error("Could not load model.");
	// Compute scene bounds.
	auto view = m_world.registry().view<Transform3DComponent, MeshComponent>();
	m_bounds;
	view.each([this](Transform3DComponent& t, MeshComponent& mesh) {
		// TODO node inheritance.
		m_bounds.include(t.transform.multiplyPoint(mesh.bounds.min));
		m_bounds.include(t.transform.multiplyPoint(mesh.bounds.max));
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
	shadowSampler.filterMag = Sampler::Filter::Linear;
	shadowSampler.filterMin = Sampler::Filter::Linear;
	shadowSampler.wrapU = Sampler::Wrap::ClampToEdge;
	shadowSampler.wrapV = Sampler::Wrap::ClampToEdge;
	shadowSampler.wrapW = Sampler::Wrap::ClampToEdge;
	// TODO texture atlas & single shader execution
	m_shadowCascadeTexture[0] = Texture::create2D(2048, 2048, TextureFormat::Float, TextureComponent::Depth, TextureFlag::RenderTarget, shadowSampler);
	m_shadowCascadeTexture[1] = Texture::create2D(2048, 2048, TextureFormat::Float, TextureComponent::Depth, TextureFlag::RenderTarget, shadowSampler);
	m_shadowCascadeTexture[2] = Texture::create2D(4096, 4096, TextureFormat::Float, TextureComponent::Depth, TextureFlag::RenderTarget, shadowSampler);
	FramebufferAttachment shadowAttachments[] = {
		FramebufferAttachment{
			FramebufferAttachmentType::Depth,
			m_shadowCascadeTexture[0]
		}
	}; 
	m_shadowFramebuffer = Framebuffer::create(shadowAttachments, 1);

	m_sun = m_world.createEntity("Sun");
	m_sun.add<Transform3DComponent>();
	m_sun.add<DirectionnalLightComponent>();
	Transform3DComponent& sunTransform = m_sun.get<Transform3DComponent>();
	DirectionnalLightComponent& sun = m_sun.get<DirectionnalLightComponent>();
	sun.direction = vec3f(0.1f, 1.f, 0.1f);
	sun.color = color3f(1.f);
	sun.intensity = 1.f;
	sunTransform.transform = mat4f::identity();

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

	m_camera.set(m_bounds);

	m_near = 0.01f;
	m_far = 100.f;
	m_hFov = anglef::degree(90.f);
	m_debug = true;

	attach<ImGuiLayer>();
}

void Viewer::onDestroy()
{
	m_world.destroy();
}

void Viewer::onUpdate(aka::Time::Unit deltaTime)
{
	// Arcball
	if(!ImGui::GetIO().WantCaptureKeyboard)
		m_camera.update(deltaTime);

	// TOD
	if (Mouse::pressed(MouseButton::ButtonMiddle))
	{
		const Position& pos = Mouse::position();
		float x = pos.x / (float)GraphicBackend::backbuffer()->width();
		m_sun.get<DirectionnalLightComponent>().direction = vec3f::normalize(lerp(vec3f(1, 1, 1), vec3f(-1, 1, -1), x));
	}

	// Reset
	if (Keyboard::down(KeyboardKey::R))
	{
		m_camera.set(m_bounds);
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
	mat4f view = mat4f::inverse(m_camera.transform());
	// TODO use camera (Arcball inherit camera ?)
	mat4f debugPerspective = mat4f::perspective(m_hFov, (float)backbuffer->width() / (float)backbuffer->height(), 0.01f, 1000.f);
	mat4f perspective = mat4f::perspective(m_hFov, (float)backbuffer->width() / (float)backbuffer->height(), m_near, m_far);

	// --- Shadow pass
	auto directShadows = m_world.registry().view<Transform3DComponent, DirectionnalLightComponent>();
	mat4f worldToLightSpaceMatrix[cascadeCount];
	const float offset[cascadeCount + 1] = { m_near, m_far / 20.f, m_far / 5.f, m_far };
	float cascadeEndClipSpace[cascadeCount];
	directShadows.each([&](const Transform3DComponent& transform, const DirectionnalLightComponent& lights) {
		// Generate shadow cascades
		for (size_t i = 0; i < cascadeCount; i++)
		{
			float w = (float)width();
			float h = (float)height();
			float n = offset[i];
			float f = offset[i + 1];
			mat4f p = mat4f::perspective(m_hFov, w / h, n, f);
			worldToLightSpaceMatrix[i] = computeShadowViewProjectionMatrix(view, p, m_shadowCascadeTexture[i]->width(), n, f, lights.direction);
			vec4f clipSpace = perspective * vec4f(0.f, 0.f, -offset[i + 1], 1.f);
			cascadeEndClipSpace[i] = clipSpace.z / clipSpace.w;
		}
		mat4f projectionToTextureCoordinateMatrix(
			col4f(0.5, 0.0, 0.0, 0.0),
			col4f(0.0, 0.5, 0.0, 0.0),
			col4f(0.0, 0.0, 0.5, 0.0),
			col4f(0.5, 0.5, 0.5, 1.0)
		);
		for (size_t i = 0; i < cascadeCount; i++)
			m_worldToLightTextureSpaceMatrix[i] = projectionToTextureCoordinateMatrix * worldToLightSpaceMatrix[i];

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
		for (size_t i = 0; i < cascadeCount; i++)
		{
			m_shadowFramebuffer->attachment(FramebufferAttachmentType::Depth, m_shadowCascadeTexture[i]); // Store in DirectionalLightComponent
			m_shadowFramebuffer->clear(color4f(1.f), 1.f, 0, ClearMask::Depth);
			m_shadowMaterial->set<mat4f>("u_light", worldToLightSpaceMatrix[i]); // Store in DirectionalLightComponent
			auto view = m_world.registry().view<Transform3DComponent, MeshComponent>();
			view.each([&](const Transform3DComponent& transform, const MeshComponent& mesh) {
				m_shadowMaterial->set<mat4f>("u_model", transform.transform);
				shadowPass.submesh = mesh.submesh;
				shadowPass.execute();
			});
		}
	});

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
		m_lightingMaterial->set<Texture::Ptr>("u_shadowTexture[0]", m_shadowCascadeTexture, cascadeCount);
		m_lightingMaterial->set<mat4f>("u_worldToLightTextureSpace[0]", m_worldToLightTextureSpaceMatrix, cascadeCount);
		m_lightingMaterial->set<vec3f>("u_lightDir", m_sun.get<DirectionnalLightComponent>().direction);
		m_lightingMaterial->set<vec3f>("u_cameraPos", vec3f(m_camera.transform()[3]));
		m_lightingMaterial->set<float>("u_cascadeEndClipSpace[0]", cascadeEndClipSpace, cascadeCount);

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
		m_material->set<mat4f>("u_view", renderView);
		m_material->set<mat4f>("u_projection", renderPerspective);
		m_material->set<mat4f>("u_light[0]", m_worldToLightTextureSpaceMatrix, cascadeCount);
		m_material->set<vec3f>("u_lightDir", m_sun.get<DirectionnalLightComponent>().direction);
		m_material->set<Texture::Ptr>("u_shadowTexture[0]", m_shadowCascadeTexture, cascadeCount);
		m_material->set<float>("u_cascadeEndClipSpace[0]", cascadeEndClipSpace, cascadeCount);
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
		// TODO add pipeline (default pipeline, debug pipeline...)
		// Debug pipeline : create origin mesh. for every mesh in scene, draw bbox & origin as line.
		vec2f windowSize = vec2f((float)backbuffer->width(), (float)backbuffer->height());
		vec2f pos[3];
		vec2f size = vec2f(128.f);
		for (size_t i = 0; i < cascadeCount; i++)
			pos[i] = vec2f(size.x + 10.f * (i + 1), windowSize.y - size.y - 10.f);
		
		int hovered = -1;
		const Position& p = Mouse::position();
		for (size_t i = 0; i < cascadeCount; i++)
		{
			if (p.x > pos[i].x && p.x < pos[i].x + size.x && p.y > pos[i].y && p.y < pos[i].y + size.y)
			{
				hovered = (int)i;
				break;
			}
		}

		Renderer3D::drawAxis(mat4f::identity());
		Renderer3D::drawAxis(mat4f::inverse(view));
		Renderer3D::drawAxis(mat4f::translate(vec3f(m_bounds.center())));
		if (hovered < 0)
		{
			Renderer3D::drawFrustum(perspective * view);
		}
		else
		{
			Renderer3D::drawFrustum(mat4f::perspective(m_hFov, (float)backbuffer->width() / (float)backbuffer->height(), offset[hovered], offset[hovered + 1]) * view);
			Renderer3D::drawFrustum(worldToLightSpaceMatrix[hovered]);
		}
		Renderer3D::drawTransform(mat4f::translate(vec3f(m_bounds.center())) * mat4f::scale(m_bounds.extent() / 2.f));
		Renderer3D::render(GraphicBackend::backbuffer(), renderView, renderPerspective);
		Renderer3D::clear();
		
		for (size_t i = 0; i < cascadeCount; i++)
			Renderer2D::drawRect(
				mat3f::identity(),
				vec2f(size.x * i + 10.f * (i + 1), windowSize.y - size.y - 10.f),
				size,
				m_shadowCascadeTexture[i],
				color4f(1.f),
				10
			);
		Renderer2D::render();
		Renderer2D::clear();
	}

	if (m_debug)
	{
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
			float fov = m_hFov.radian();
			if (ImGui::SliderAngle("Fov", &fov, 10.f, 160.f))
				m_hFov = anglef::radian(fov);
			ImGui::SliderFloat("Near", &m_near, 0.001f, 10.f);
			ImGui::SliderFloat("Far", &m_far, 10.f, 1000.f);
			//imgui gizmo
		}
		ImGui::End();

		if (ImGui::Begin("Scene"))
		{
			uint32_t id = 0;
			uint32_t vertexCount = 0;
			uint32_t indexCount = 0;
			renderableView.each([&](Transform3DComponent& transform, const MeshComponent& mesh, const MaterialComponent& material) {
				char buffer[256];
				vertexCount += mesh.submesh.mesh->getVertexCount();
				indexCount += mesh.submesh.mesh->getIndexCount();
				snprintf(buffer, 256, "Node %u", id++);
				if (ImGui::TreeNode(buffer))
				{
					// TODO use ImGuiGizmo
					ImGui::Text("Transform");
					ImGui::InputFloat4("##col0", transform.transform.cols[0].data);
					ImGui::InputFloat4("##col1", transform.transform.cols[1].data);
					ImGui::InputFloat4("##col2", transform.transform.cols[2].data);
					ImGui::InputFloat4("##col3", transform.transform.cols[3].data);
					ImGui::Text("Mesh");
					ImGui::Text("Vertices : %d", mesh.submesh.mesh->getVertexCount());
					ImGui::Text("Indices : %d", mesh.submesh.mesh->getIndexCount());
					material;

					ImGui::TreePop();
				}
			});
			ImGui::Text("Vertices : %d", vertexCount);
			ImGui::Text("Indices : %d", indexCount);
		}
		ImGui::End();
	}
}

};
