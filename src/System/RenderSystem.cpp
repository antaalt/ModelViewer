#include "RenderSystem.h"

#include "../Model/Model.h"

namespace viewer {

using namespace aka;

void RenderSystem::onCreate(aka::World& world)
{
	Framebuffer::Ptr backbuffer = GraphicBackend::backbuffer();

	createShaders();
	createRenderTargets(backbuffer->width(), backbuffer->height());

	// --- Lighting pass
	float quadVertices[] = {
		-1, -1, // bottom left corner
		 1, -1, // bottom right corner
		 1,  1, // top right corner
		-1,  1, // top left corner
	};
	uint16_t quadIndices[] = { 0,1,2,0,2,3 };
	m_quad = Mesh::create();
	std::vector<VertexAccessor> quadVertexInfo = {{
		VertexAccessor{
			VertexAttribute{ VertexSemantic::Position, VertexFormat::Float, VertexType::Vec2 },
			VertexBufferView{
				Buffer::create(BufferType::VertexBuffer, sizeof(quadVertices), BufferUsage::Immutable, BufferCPUAccess::None, quadVertices),
				0, // offset
				sizeof(quadVertices), // size
				sizeof(float) * 2 // stride
			},
			0,
			8
		}
	} };
	IndexAccessor quadIndexInfo{};
	quadIndexInfo.format = IndexFormat::UnsignedShort;
	quadIndexInfo.count = 6;
	quadIndexInfo.bufferView.buffer = Buffer::create(BufferType::IndexBuffer, sizeof(quadIndices), BufferUsage::Immutable, BufferCPUAccess::None, quadIndices);
	quadIndexInfo.bufferView.offset = 0;
	quadIndexInfo.bufferView.size = (uint32_t)quadIndexInfo.bufferView.buffer->size();
	m_quad->upload(quadVertexInfo.data(), quadVertexInfo.size(), quadIndexInfo);

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
	// TODO cubemap as component somehow ? or simply parameter
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
		TextureFormat::RGBA8, TextureFlag::None,
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

	VertexAccessor skyboxVertexInfo = {
		VertexAttribute{ VertexSemantic::Position, VertexFormat::Float, VertexType::Vec3 },
		VertexBufferView{
			Buffer::create(BufferType::VertexBuffer, sizeof(skyboxVertices), BufferUsage::Immutable, BufferCPUAccess::None, skyboxVertices),
			0, // offset
			sizeof(skyboxVertices), // size
			sizeof(float) * 3 // stride
		},
		0, // offset
		sizeof(skyboxVertices) / sizeof(float) // count
	};
	m_cube->upload(&skyboxVertexInfo, 1);
}

void RenderSystem::onDestroy(aka::World& world)
{
	// Gbuffer pass
	m_position.reset();
	m_albedo.reset();
	m_normal.reset();
	m_depth.reset();
	m_material.reset();
	m_gbuffer.reset();
	m_gbufferMaterial.reset();

	// Lighing pass
	m_quad.reset();
	m_sphere.reset();
	m_ambientMaterial.reset();
	m_pointMaterial.reset();
	m_dirMaterial.reset();

	// Skybox
	m_cube.reset();
	m_skybox.reset();
	m_skyboxMaterial.reset();

	// Post process pass
	m_storageDepth.reset();
	m_storage.reset();
	m_storageFramebuffer.reset();
	m_postprocessMaterial.reset();
}

void RenderSystem::onRender(aka::World& world)
{
	EventDispatcher<ShaderHotReloadEvent>::dispatch();

	Framebuffer::Ptr backbuffer = GraphicBackend::backbuffer();

	Entity cameraEntity = Scene::getMainCamera(world);
	Camera3DComponent& camera = cameraEntity.get<Camera3DComponent>();
	mat4f view = camera.view;
	mat4f projection = camera.projection->projection();

	auto renderableView = world.registry().view<Transform3DComponent, MeshComponent, MaterialComponent>();

	// --- G-Buffer pass
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

	m_gbufferMaterial->set<mat4f>("u_view", view);
	m_gbufferMaterial->set<mat4f>("u_projection", projection);

	m_gbuffer->clear(color4f(0.f), 1.f, 0, ClearMask::All);

	renderableView.each([&](const Transform3DComponent& transform, const MeshComponent& mesh, const MaterialComponent& material) {
		frustum<>::planes p = frustum<>::extract(projection * view);
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
		m_gbufferMaterial->set<Texture::Ptr>("u_materialTexture", material.materialTexture);
		m_gbufferMaterial->set<Texture::Ptr>("u_colorTexture", material.colorTexture);
		m_gbufferMaterial->set<Texture::Ptr>("u_normalTexture", material.normalTexture);
		gbufferPass.submesh = mesh.submesh;
		gbufferPass.cull = Culling{ CullMode::BackFace, CullOrder::CounterClockWise };

		gbufferPass.execute();
	});

	// --- Lighting pass
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
	lightingPass.material->set<Texture::Ptr>("u_positionTexture", m_position);
	lightingPass.material->set<Texture::Ptr>("u_albedoTexture", m_albedo);
	lightingPass.material->set<Texture::Ptr>("u_normalTexture", m_normal);
	lightingPass.material->set<Texture::Ptr>("u_skyboxTexture", m_skybox);
	//lightingPass.material->set<Texture::Ptr>("u_materialTexture", m_material);
	lightingPass.material->set<vec3f>("u_cameraPos", vec3f(cameraEntity.get<Transform3DComponent>().transform[3]));
	lightingPass.execute();

	// --- Directional lights
	lightingPass.material = m_dirMaterial;
	lightingPass.material->set<Texture::Ptr>("u_positionTexture", m_position);
	lightingPass.material->set<Texture::Ptr>("u_albedoTexture", m_albedo);
	lightingPass.material->set<Texture::Ptr>("u_normalTexture", m_normal);
	lightingPass.material->set<Texture::Ptr>("u_depthTexture", m_depth);
	lightingPass.material->set<Texture::Ptr>("u_materialTexture", m_material);
	lightingPass.material->set<vec3f>("u_cameraPos", vec3f(cameraEntity.get<Transform3DComponent>().transform[3]));
	auto directionalShadows = world.registry().view<Transform3DComponent, DirectionalLightComponent>();
	directionalShadows.each([&](const Transform3DComponent& transform, DirectionalLightComponent& light) {
		mat4f worldToLightTextureSpaceMatrix[DirectionalLightComponent::cascadeCount];
		for (size_t i = 0; i < DirectionalLightComponent::cascadeCount; i++)
			worldToLightTextureSpaceMatrix[i] = projectionToTextureCoordinateMatrix * light.worldToLightSpaceMatrix[i];
		lightingPass.material->set<vec3f>("u_lightDirection", light.direction);
		lightingPass.material->set<float>("u_lightIntensity", light.intensity);
		lightingPass.material->set<color3f>("u_lightColor", light.color);
		lightingPass.material->set<mat4f>("u_worldToLightTextureSpace", worldToLightTextureSpaceMatrix, DirectionalLightComponent::cascadeCount);
		lightingPass.material->set<float>("u_cascadeEndClipSpace", light.cascadeEndClipSpace, DirectionalLightComponent::cascadeCount);
		lightingPass.material->set<Texture::Ptr>("u_shadowMap", light.shadowMap, DirectionalLightComponent::cascadeCount);
		lightingPass.execute();
	});

	// --- Point lights
	// Using light volumes
	lightingPass.submesh = SubMesh{ m_sphere, PrimitiveType::Triangles, m_sphere->getIndexCount(), 0 };
	lightingPass.cull = Culling{ CullMode::FrontFace, CullOrder::CounterClockWise }; // Important to avoid rendering 2 times or clipping
	lightingPass.material = m_pointMaterial;
	lightingPass.material->set<Texture::Ptr>("u_positionTexture", m_position);
	lightingPass.material->set<Texture::Ptr>("u_albedoTexture", m_albedo);
	lightingPass.material->set<Texture::Ptr>("u_normalTexture", m_normal);
	lightingPass.material->set<Texture::Ptr>("u_materialTexture", m_material);
	lightingPass.material->set<vec2f>("u_screen", vec2f(m_storage->width(), m_storage->height()));
	lightingPass.material->set<vec3f>("u_cameraPos", vec3f(cameraEntity.get<Transform3DComponent>().transform[3]));
	auto pointShadows = world.registry().view<Transform3DComponent, PointLightComponent>();
	pointShadows.each([&](const Transform3DComponent& transform, PointLightComponent& light) {
		point3f position(transform.transform.cols[3]);
		aabbox<> bounds(position + vec3f(-light.radius), position + vec3f(light.radius));
		frustum<>::planes p = frustum<>::extract(projection * view);
		if (!p.intersect(transform.transform * bounds))
			return;
		mat4f model = transform.transform * mat4f::scale(vec3f(light.radius));
		lightingPass.material->set<mat4f>("u_mvp", projection* view * model);
		lightingPass.material->set<vec3f>("u_lightPosition", vec3f(position));
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
	skyboxPass.submesh.count = m_cube->getVertexCount(0);
	skyboxPass.submesh.mesh = m_cube;
	skyboxPass.material = m_skyboxMaterial;
	skyboxPass.clear = Clear{ ClearMask::None, color4f(0.f), 1.f, 0 };
	skyboxPass.blend = Blending::none();
	skyboxPass.depth = Depth{ DepthCompare::LessOrEqual, false };
	skyboxPass.stencil = Stencil::none();
	skyboxPass.viewport = aka::Rect{ 0 };
	skyboxPass.scissor = aka::Rect{ 0 };
	skyboxPass.cull = Culling{ CullMode::BackFace, CullOrder::CounterClockWise };

	skyboxPass.material->set<Texture::Ptr>("u_skyboxTexture", m_skybox);
	skyboxPass.material->set<mat4f>("u_view", mat4f(mat3f(view))); // TODO mvp
	skyboxPass.material->set<mat4f>("u_projection", projection);

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

	postProcessPass.material->set<Texture::Ptr>("u_inputTexture", m_storage);
	postProcessPass.material->set<uint32_t>("u_width", backbuffer->width());
	postProcessPass.material->set<uint32_t>("u_height", backbuffer->height());

	postProcessPass.execute();

	// Set depth for UI elements
	backbuffer->blit(m_storageFramebuffer, FramebufferAttachmentType::DepthStencil, Sampler::Filter::Nearest);
}

void RenderSystem::onReceive(const aka::BackbufferResizeEvent& e)
{
	createRenderTargets(e.width, e.height);
}

void RenderSystem::onReceive(const ShaderHotReloadEvent& e)
{
	createShaders();
}

void RenderSystem::createShaders()
{
	// TODO cache shader and do not delete them during program creation.
	// TODO use custom file for handling multiple API (ASSET REWORK, JSON based ?)
	// - a file indexing shader path depending on the api & the type (frag, vert...) (JSON)
	// - a single file containing all the shaders from all the api delimited by # or json 
	//	-> problem of linting.
	std::vector<VertexAttribute> defaultAttributes = {
		VertexAttribute{ VertexSemantic::Position, VertexFormat::Float, VertexType::Vec3 },
		VertexAttribute{ VertexSemantic::Normal, VertexFormat::Float, VertexType::Vec3 },
		VertexAttribute{ VertexSemantic::TexCoord0, VertexFormat::Float, VertexType::Vec2 },
		VertexAttribute{ VertexSemantic::Color0, VertexFormat::Float, VertexType::Vec4 }
	};
	std::vector<VertexAttribute> quadAttribute = {
		VertexAttribute{ VertexSemantic::Position, VertexFormat::Float, VertexType::Vec2 }
	};
	std::vector<VertexAttribute> cubeAttribute = {
		VertexAttribute{ VertexSemantic::Position, VertexFormat::Float, VertexType::Vec3 }
	};
	{
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
			aka::Shader::Ptr shader = aka::Shader::create(vert, frag, defaultAttributes.data(), defaultAttributes.size());
			if (shader->valid())
				m_gbufferMaterial = aka::ShaderMaterial::create(shader);
		}
	}
	{
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
			aka::Shader::Ptr shader = aka::Shader::create(vert, frag, defaultAttributes.data(), defaultAttributes.size());
			if (shader->valid())
				m_pointMaterial = aka::ShaderMaterial::create(shader);
		}
	}
	{
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
			aka::Shader::Ptr shader = aka::Shader::create(vert, frag, quadAttribute.data(), quadAttribute.size());
			if (shader->valid())
				m_dirMaterial = aka::ShaderMaterial::create(shader);
		}
	}
	{
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
			aka::Shader::Ptr shader = aka::Shader::create(vert, frag, quadAttribute.data(), quadAttribute.size());
			if (shader->valid())
				m_ambientMaterial = aka::ShaderMaterial::create(shader);
		}
	}
	{
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
			aka::Shader::Ptr shader = aka::Shader::create(vert, frag, cubeAttribute.data(), cubeAttribute.size());
			if (shader->valid())
				m_skyboxMaterial = aka::ShaderMaterial::create(shader);
		}
	}
	{
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
			aka::Shader::Ptr shader = aka::Shader::create(vert, frag, quadAttribute.data(), quadAttribute.size());
			if (shader->valid())
				m_postprocessMaterial = aka::ShaderMaterial::create(shader);
		}
	}
}

void RenderSystem::createRenderTargets(uint32_t width, uint32_t height)
{
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
	m_depth = Texture::create2D(width, height, TextureFormat::DepthStencil, TextureFlag::RenderTarget, gbufferSampler);
	m_position = Texture::create2D(width, height, TextureFormat::RGBA16F, TextureFlag::RenderTarget, gbufferSampler);
	m_albedo = Texture::create2D(width, height, TextureFormat::RGBA8, TextureFlag::RenderTarget, gbufferSampler);
	m_normal = Texture::create2D(width, height, TextureFormat::RGBA16F, TextureFlag::RenderTarget, gbufferSampler);
	m_material = Texture::create2D(width, height, TextureFormat::RGBA16F, TextureFlag::RenderTarget, gbufferSampler);
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
			m_material
		}
	};
	m_gbuffer = Framebuffer::create(gbufferAttachments, sizeof(gbufferAttachments) / sizeof(FramebufferAttachment));

	// --- Post process
	m_storageDepth = Texture::create2D(width, height, TextureFormat::DepthStencil, TextureFlag::RenderTarget, gbufferSampler);
	m_storage = Texture::create2D(width, height, TextureFormat::RGBA16F, TextureFlag::RenderTarget, gbufferSampler);
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
}

};