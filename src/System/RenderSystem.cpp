#include "RenderSystem.h"

#include "../Model/Model.h"

namespace viewer {

using namespace aka;

// Array of type are aligned as 16 in std140 (???) 
struct alignas(16) Aligned16Float {
	float data;
};

struct alignas(16) DirectionalLightUniformBuffer {
	alignas(16) vec3f direction;
	alignas(4) float intensity;
	alignas(16) vec3f color;
	alignas(16) mat4f worldToLightTextureSpace[DirectionalLightComponent::cascadeCount];
	alignas(16) Aligned16Float cascadeEndClipSpace[DirectionalLightComponent::cascadeCount];
};

struct alignas(16) PointLightUniformBuffer {
	alignas(16) vec3f lightPosition;
	alignas(4) float lightIntensity;
	alignas(16) color3f lightColor;
	alignas(4) float farPointLight;
};

struct alignas(16) CameraUniformBuffer {
	alignas(16) mat4f view;
	alignas(16) mat4f projection;
	alignas(16) mat4f viewInverse;
	alignas(16) mat4f projectionInverse;
};
struct alignas(16) ViewportUniformBuffer {
	alignas(8) vec2f viewport;
	alignas(8) vec2f rcp;
};
struct alignas(16) ModelUniformBuffer {
	alignas(16) mat4f model;
	alignas(16) vec3f normalMatrix0;
	alignas(16) vec3f normalMatrix1;
	alignas(16) vec3f normalMatrix2;
	alignas(16) color4f color;
};

void RenderSystem::onCreate(aka::World& world)
{
	GraphicDevice* device = GraphicBackend::device();
	Backbuffer::Ptr backbuffer = device->backbuffer();

	m_gbufferMaterial = Material::create(ProgramManager::get("gbuffer"));
	m_pointMaterial = Material::create(ProgramManager::get("point"));
	m_dirMaterial = Material::create(ProgramManager::get("directional"));
	m_ambientMaterial = Material::create(ProgramManager::get("ambient"));
	m_skyboxMaterial = Material::create(ProgramManager::get("skybox"));
	m_postprocessMaterial = Material::create(ProgramManager::get("postProcess"));
	m_textMaterial = Material::create(ProgramManager::get("text"));

	createRenderTargets(backbuffer->width(), backbuffer->height());

	// --- Uniforms
	m_cameraUniformBuffer = Buffer::create(BufferType::Uniform, sizeof(CameraUniformBuffer), BufferUsage::Default, BufferCPUAccess::None);
	m_viewportUniformBuffer = Buffer::create(BufferType::Uniform, sizeof(ViewportUniformBuffer), BufferUsage::Default, BufferCPUAccess::None);
	m_modelUniformBuffer = Buffer::create(BufferType::Uniform, sizeof(ModelUniformBuffer), BufferUsage::Default, BufferCPUAccess::None); // This one change a lot. use dynamic
	m_pointLightUniformBuffer = Buffer::create(BufferType::Uniform, sizeof(PointLightUniformBuffer), BufferUsage::Default, BufferCPUAccess::None); // This one change a lot.
	m_directionalLightUniformBuffer = Buffer::create(BufferType::Uniform, sizeof(DirectionalLightUniformBuffer), BufferUsage::Default, BufferCPUAccess::None); // This one change a lot.

	// --- Lighting pass
	m_shadowSampler.filterMag = TextureFilter::Nearest;
	m_shadowSampler.filterMin = TextureFilter::Nearest;
	m_shadowSampler.wrapU = TextureWrap::ClampToEdge;
	m_shadowSampler.wrapV = TextureWrap::ClampToEdge;
	m_shadowSampler.wrapW = TextureWrap::ClampToEdge;
	m_shadowSampler.anisotropy = 1.f;

	float quadVertices[] = {
		-1.f, -1.f, // bottom left corner
		 1.f, -1.f, // bottom right corner
		 1.f,  1.f, // top right corner
		-1.f,  1.f, // top left corner
	};
	uint16_t quadIndices[] = { 0,1,2,0,2,3 };
	m_quad = Mesh::create();
	VertexAttribute quadAttributes = VertexAttribute{ VertexSemantic::Position, VertexFormat::Float, VertexType::Vec2 };
	m_quad->uploadInterleaved(&quadAttributes, 1, quadVertices, 4, IndexFormat::UnsignedShort, quadIndices, 6);

	m_sphere = Scene::createSphereMesh(point3f(0.f), 1.f, 32, 16);

	// --- Skybox pass
	uint8_t data[] = { 
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 
		200, 200, 200, 255, 200, 200, 200, 255, 200, 200, 200, 255, 200, 200, 200, 255,
		170, 170, 170, 255, 170, 170, 170, 255, 170, 170, 170, 255, 170, 170, 170, 255,
		140, 140, 140, 255, 140, 140, 140, 255, 140, 140, 140, 255, 140, 140, 140, 255,
		110, 110, 110, 255, 110, 110, 110, 255, 110, 110, 110, 255, 110, 110, 110, 255,
		 90,  90,  90, 255,  90,  90,  90, 255,  90,  90,  90, 255,  90,  90,  90, 255,
		 60,  60,  60, 255,  60,  60,  60, 255,  60,  60,  60, 255,  60,  60,  60, 255,
		 30,  30,  30, 255,  30,  30,  30, 255,  30,  30,  30, 255,  30,  30,  30, 255,
	};
	Texture2D::Ptr equirectangularMap = Texture2D::create(4, 8, TextureFormat::RGBA8, TextureFlag::ShaderResource, data);
	m_skybox = TextureCubeMap::generate(512, 512, TextureFormat::RGBA8, TextureFlag::ShaderResource, equirectangularMap, TextureFilter::Linear);
	m_skyboxSampler.filterMag = TextureFilter::Linear;
	m_skyboxSampler.filterMin = TextureFilter::Linear;
	m_skyboxSampler.wrapU = TextureWrap::ClampToEdge;
	m_skyboxSampler.wrapV = TextureWrap::ClampToEdge;
	m_skyboxSampler.wrapW = TextureWrap::ClampToEdge;
	m_skyboxSampler.anisotropy = 1.f;

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
	VertexAttribute cubeAttributes = VertexAttribute{ VertexSemantic::Position, VertexFormat::Float, VertexType::Vec3 };
	m_cube->uploadInterleaved(&cubeAttributes, 1, &skyboxVertices, 36);
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
	GraphicDevice* device = GraphicBackend::device();
	Backbuffer::Ptr backbuffer = device->backbuffer();

	Entity cameraEntity = Scene::getMainCamera(world);
	Camera3DComponent& camera = cameraEntity.get<Camera3DComponent>();
	mat4f view = camera.view;
	mat4f projection = camera.projection->projection();

	// --- Update Uniforms
	m_gbufferMaterial->set("ModelUniformBuffer", m_modelUniformBuffer);
	m_gbufferMaterial->set("CameraUniformBuffer", m_cameraUniformBuffer);
	m_ambientMaterial->set("CameraUniformBuffer", m_cameraUniformBuffer);
	m_dirMaterial->set("CameraUniformBuffer", m_cameraUniformBuffer);
	m_dirMaterial->set("DirectionalLightUniformBuffer", m_directionalLightUniformBuffer);
	m_pointMaterial->set("PointLightUniformBuffer", m_pointLightUniformBuffer);
	m_pointMaterial->set("CameraUniformBuffer", m_cameraUniformBuffer);
	m_pointMaterial->set("ViewportUniformBuffer", m_viewportUniformBuffer);
	m_pointMaterial->set("ModelUniformBuffer", m_modelUniformBuffer);
	m_pointMaterial->set("CameraUniformBuffer", m_cameraUniformBuffer);
	m_skyboxMaterial->set("CameraUniformBuffer", m_cameraUniformBuffer);
	m_postprocessMaterial->set("ViewportUniformBuffer", m_viewportUniformBuffer);
	// TODO only update on camera move / update
	CameraUniformBuffer cameraUBO;
	cameraUBO.view = view;
	cameraUBO.projection = projection;
	cameraUBO.viewInverse = cameraEntity.get<Transform3DComponent>().transform;
	cameraUBO.projectionInverse = mat4f::inverse(projection);
	m_cameraUniformBuffer->upload(&cameraUBO);
	ViewportUniformBuffer viewportUBO;
	viewportUBO.viewport = vec2f(backbuffer->width(), backbuffer->height());
	m_viewportUniformBuffer->upload(&viewportUBO);

	// Samplers
	TextureSampler samplers[] = { m_shadowSampler , m_shadowSampler , m_shadowSampler };
	m_dirMaterial->set("u_shadowMap", samplers, DirectionalLightComponent::cascadeCount);
	m_pointMaterial->set("u_shadowMap", m_shadowSampler);

	auto renderableView = world.registry().view<Transform3DComponent, MeshComponent, MaterialComponent>();

	// --- G-Buffer pass
	// TODO depth prepass
	RenderPass gbufferPass;
	gbufferPass.framebuffer = m_gbuffer;
	gbufferPass.material = m_gbufferMaterial;
	gbufferPass.clear = Clear::none;
	gbufferPass.blend = Blending::none;
	gbufferPass.depth = Depth{ DepthCompare::Less, true };
	gbufferPass.cull = Culling{ CullMode::BackFace, CullOrder::CounterClockWise };
	gbufferPass.stencil = Stencil::none;
	gbufferPass.viewport = aka::Rect{ 0 };
	gbufferPass.scissor = aka::Rect{ 0 };

	m_gbuffer->clear(color4f(0.f), 1.f, 0, ClearMask::All);

	renderableView.each([&](const Transform3DComponent& transform, const MeshComponent& mesh, const MaterialComponent& material) {
		frustum<>::planes p = frustum<>::extract(projection * view);
		// Check intersection in camera space
		// https://www.gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf
		if (!p.intersect(transform.transform * mesh.bounds))
			return;

		ModelUniformBuffer modelUBO;
		modelUBO.model = transform.transform;
		mat3f normalMatrix = mat3f::transpose(mat3f::inverse(mat3f(transform.transform)));
		modelUBO.normalMatrix0 = vec3f(normalMatrix[0]);
		modelUBO.normalMatrix1 = vec3f(normalMatrix[1]);
		modelUBO.normalMatrix2 = vec3f(normalMatrix[2]);
		modelUBO.color = material.color;
		m_modelUniformBuffer->upload(&modelUBO);

		gbufferPass.material->set("u_materialTexture", material.material.sampler);
		gbufferPass.material->set("u_materialTexture", material.material.texture);
		gbufferPass.material->set("u_colorTexture", material.albedo.sampler);
		gbufferPass.material->set("u_colorTexture", material.albedo.texture);
		gbufferPass.material->set("u_normalTexture", material.normal.sampler);
		gbufferPass.material->set("u_normalTexture", material.normal.texture);

		gbufferPass.submesh = mesh.submesh;

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
	lightingPass.submesh.count = m_quad->getIndexCount();
	lightingPass.clear = Clear::none;
	lightingPass.blend.colorModeSrc = BlendMode::One;
	lightingPass.blend.colorModeDst = BlendMode::One;
	lightingPass.blend.colorOp = BlendOp::Add;
	lightingPass.blend.alphaModeSrc = BlendMode::One;
	lightingPass.blend.alphaModeDst = BlendMode::Zero;
	lightingPass.blend.alphaOp = BlendOp::Add;
	lightingPass.blend.mask = BlendMask::Rgb;
	lightingPass.blend.blendColor = color32(255);
	lightingPass.depth = Depth::none;
	lightingPass.stencil = Stencil::none;
	lightingPass.viewport = aka::Rect{ 0 };
	lightingPass.scissor = aka::Rect{ 0 };
	lightingPass.cull = Culling{ CullMode::BackFace, CullOrder::CounterClockWise };

	// --- Ambient light
	lightingPass.material = m_ambientMaterial;
	lightingPass.material->set("u_positionTexture", m_position);
	lightingPass.material->set("u_albedoTexture", m_albedo);
	lightingPass.material->set("u_normalTexture", m_normal);
	//lightingPass.material->set<Texture::Ptr>("u_materialTexture", m_material);
	lightingPass.material->set("u_skyboxTexture", m_skybox);

	lightingPass.execute();

	// --- Directional lights
	lightingPass.material = m_dirMaterial;
	lightingPass.material->set("u_positionTexture", m_position);
	lightingPass.material->set("u_albedoTexture", m_albedo);
	lightingPass.material->set("u_normalTexture", m_normal);
	lightingPass.material->set("u_depthTexture", m_depth);
	lightingPass.material->set("u_materialTexture", m_material);
	
	auto directionalShadows = world.registry().view<Transform3DComponent, DirectionalLightComponent>();
	directionalShadows.each([&](const Transform3DComponent& transform, DirectionalLightComponent& light) {
		mat4f worldToLightTextureSpaceMatrix[DirectionalLightComponent::cascadeCount];
		for (size_t i = 0; i < DirectionalLightComponent::cascadeCount; i++)
			worldToLightTextureSpaceMatrix[i] = projectionToTextureCoordinateMatrix * light.worldToLightSpaceMatrix[i];

		DirectionalLightUniformBuffer directionalUBO;
		directionalUBO.direction = light.direction;
		directionalUBO.intensity = light.intensity;
		directionalUBO.color = vec3f(light.color.r, light.color.g, light.color.b);
		memcpy(directionalUBO.worldToLightTextureSpace, worldToLightTextureSpaceMatrix, sizeof(worldToLightTextureSpaceMatrix));
		for (size_t i = 0; i < DirectionalLightComponent::cascadeCount; i++)
			directionalUBO.cascadeEndClipSpace[i].data = light.cascadeEndClipSpace[i];
		m_directionalLightUniformBuffer->upload(&directionalUBO);

		for (size_t i = 0; i < DirectionalLightComponent::cascadeCount; i++)
			lightingPass.material->set("u_shadowMap", light.shadowMap[i], (uint32_t)i);
		lightingPass.execute();
	});

	// --- Point lights
	// Using light volumes
	lightingPass.submesh = SubMesh{ m_sphere, PrimitiveType::Triangles, m_sphere->getIndexCount(), 0 };
	lightingPass.cull = Culling{ CullMode::FrontFace, CullOrder::CounterClockWise }; // Important to avoid rendering 2 times or clipping
	lightingPass.material = m_pointMaterial;
	lightingPass.material->set("u_positionTexture", m_position);
	lightingPass.material->set("u_albedoTexture", m_albedo);
	lightingPass.material->set("u_normalTexture", m_normal);
	lightingPass.material->set("u_materialTexture", m_material);

	ModelUniformBuffer modelUBO;
	modelUBO.color = color4f(1.f);
	modelUBO.normalMatrix0 = vec3f(1, 0, 0);
	modelUBO.normalMatrix1 = vec3f(0, 1, 0);
	modelUBO.normalMatrix2 = vec3f(0, 0, 1);

	auto pointShadows = world.registry().view<Transform3DComponent, PointLightComponent>();
	pointShadows.each([&](const Transform3DComponent& transform, PointLightComponent& light) {
		point3f position(transform.transform.cols[3]);
		aabbox<> bounds(position + vec3f(-light.radius), position + vec3f(light.radius));
		frustum<>::planes p = frustum<>::extract(projection * view);
		if (!p.intersect(transform.transform * bounds))
			return;

		PointLightUniformBuffer pointUBO;
		pointUBO.lightPosition = vec3f(position);
		pointUBO.lightIntensity = light.intensity;
		pointUBO.lightColor = light.color;
		pointUBO.farPointLight = light.radius;
		m_pointLightUniformBuffer->upload(&pointUBO);


		ModelUniformBuffer modelUBO;
		modelUBO.model = transform.transform * mat4f::scale(vec3f(light.radius));
		m_modelUniformBuffer->upload(&modelUBO);

		lightingPass.material->set("u_shadowMap", light.shadowMap);

		lightingPass.execute();
	});

	// Copy depth to storage depth
	Texture::copy(m_depth, m_storageDepth);

	// --- Skybox pass
	RenderPass skyboxPass;
	skyboxPass.framebuffer = m_storageFramebuffer;
	skyboxPass.submesh.type = PrimitiveType::Triangles;
	skyboxPass.submesh.offset = 0;
	skyboxPass.submesh.count = m_cube->getVertexCount(0);
	skyboxPass.submesh.mesh = m_cube;
	skyboxPass.material = m_skyboxMaterial;
	skyboxPass.clear = Clear::none;
	skyboxPass.blend = Blending::none;
	skyboxPass.depth = Depth{ DepthCompare::LessOrEqual, false };
	skyboxPass.stencil = Stencil::none;
	skyboxPass.viewport = aka::Rect{ 0 };
	skyboxPass.scissor = aka::Rect{ 0 };
	skyboxPass.cull = Culling{ CullMode::BackFace, CullOrder::CounterClockWise };

	skyboxPass.material->set("u_skyboxTexture", m_skyboxSampler);
	skyboxPass.material->set("u_skyboxTexture", m_skybox);

	skyboxPass.execute();

	// --- Text pass
	// Text should generate a mesh with its UV & co. This way, it will be easier to handle it.
	// Dirty rendering for now, close your eyes.
	RenderPass textPass;
	textPass.framebuffer = m_storageFramebuffer;
	textPass.material = m_textMaterial;
	textPass.clear = Clear::none;
	textPass.blend = Blending::none;
	textPass.depth = Depth{ DepthCompare::LessOrEqual, false };
	textPass.stencil = Stencil::none;
	textPass.viewport = aka::Rect{ 0 };
	textPass.scissor = aka::Rect{ 0 };
	textPass.cull = Culling::none;

	// Setup text mesh
	uint16_t quadIndices[] = { 0,1,2,0,2,3 };
	float vertices[4 * 2 * 2] = {
		// pos
		0.f, 0.f,
		1.f, 0.f,
		1.f, 1.f,
		0.f, 1.f,
		// uv
		0.f, 0.f,
		1.f, 0.f,
		1.f, 1.f,
		0.f, 1.f,
	};
	Buffer::Ptr textVertices = Buffer::create(BufferType::Vertex, 4 * sizeof(float) * 2 * 2, BufferUsage::Dynamic, BufferCPUAccess::Write, vertices);
	Buffer::Ptr textIndices  = Buffer::create(BufferType::Index, sizeof(quadIndices), BufferUsage::Default, BufferCPUAccess::None, quadIndices);
	VertexAccessor accessors[2] = {
		VertexAccessor{
			VertexAttribute{ VertexSemantic::Position, VertexFormat::Float, VertexType::Vec2 },
			VertexBufferView{ textVertices, 0, 4 * sizeof(float) * 2, 0 },
			0, 4
		},
		VertexAccessor{
			VertexAttribute{ VertexSemantic::TexCoord0, VertexFormat::Float, VertexType::Vec2 },
			VertexBufferView{ textVertices, 0, 4 * sizeof(float) * 2, 0 },
			4 * sizeof(float) * 2, // offset
			4 // count
		}
	};
	IndexAccessor indexAccessor{};
	indexAccessor.bufferView = IndexBufferView{ textIndices, 0, sizeof(quadIndices) };
	indexAccessor.format = IndexFormat::UnsignedShort;
	indexAccessor.count = 6;
	Mesh::Ptr textMesh = Mesh::create();
	textMesh->upload(accessors, 2, indexAccessor);
	textPass.submesh.type = PrimitiveType::Triangles;
	textPass.submesh.offset = 0;
	textPass.submesh.count = textMesh->getIndexCount();
	textPass.submesh.mesh = textMesh;

	textPass.material->set("ModelUniformBuffer", m_modelUniformBuffer);
	textPass.material->set("CameraUniformBuffer", m_cameraUniformBuffer);

	auto textView = world.registry().view<Transform3DComponent, TextComponent>();
	textView.each([&](const Transform3DComponent& transform, TextComponent& text) {
		textPass.material->set("u_texture", text.font->atlas());
		textPass.material->set("u_texture", text.sampler);
		// TODO draw instanced for performance
		float scale = 1.f; // TODO add scale ?
		float advance = 0.f;
		const char* start = text.text.begin();
		const char* end = text.text.end();
		while (start < end)
		{
			uint32_t c = encoding::next(start, end);
			// TODO check if rendering text out of screen for culling ?
			const Character& ch = text.font->getCharacter(c);
			vec2f position = vec2f(advance + ch.bearing.x, (float)-(ch.size.y - ch.bearing.y)) * scale;
			vec2f size = vec2f((float)ch.size.x, (float)ch.size.y) * scale;

			ModelUniformBuffer modelUBO;
			modelUBO.model = transform.transform * mat4f::translate(vec3f(position, 0.f)) * mat4f::scale(vec3f(size, 1.f));
			modelUBO.color = text.color;
			modelUBO.normalMatrix0 = vec3f(1, 0, 0);
			modelUBO.normalMatrix1 = vec3f(0, 1, 0);
			modelUBO.normalMatrix2 = vec3f(0, 0, 1);
			m_modelUniformBuffer->upload(&modelUBO);

			uv2f begin = ch.texture.get(0);
			uv2f end = ch.texture.get(1);
			// TODO fix uv here. Depend on API and flipUV directive.
			begin.v = 1.f - begin.v;
			end.v = 1.f - end.v;

			// Update mesh UV. Use uv transform matrix instead ?
			float* verts = (float*)textVertices->map(BufferMap::Write);
			memcpy(verts, vertices, sizeof(vertices));
			verts[8] = begin.u; verts[9] = begin.v;
			verts[10] = end.u; verts[11] = begin.v;
			verts[12] = end.u; verts[13] = end.v;
			verts[14] = begin.u; verts[15] = end.v;
			textVertices->unmap();

			textPass.execute();
			// now advance cursors for next glyph (note that advance is number of 1/64 pixels)
			advance += ch.advance * scale;
		}
	});


	// --- Post process pass
	RenderPass postProcessPass;
	postProcessPass.framebuffer = backbuffer;
	postProcessPass.submesh.type = PrimitiveType::Triangles;
	postProcessPass.submesh.offset = 0;
	postProcessPass.submesh.count = m_quad->getIndexCount();
	postProcessPass.submesh.mesh = m_quad;
	postProcessPass.material = m_postprocessMaterial;
	postProcessPass.clear = Clear::none;
	postProcessPass.blend = Blending::none;
	postProcessPass.depth = Depth::none;
	postProcessPass.stencil = Stencil::none;
	postProcessPass.viewport = aka::Rect{ 0 };
	postProcessPass.scissor = aka::Rect{ 0 };
	postProcessPass.cull = Culling{ CullMode::BackFace, CullOrder::CounterClockWise };

	postProcessPass.material->set("u_inputTexture", m_storage);

	postProcessPass.execute();

	// Set depth for UI elements
	backbuffer->blit(m_storageDepth, TextureFilter::Nearest);
}

void RenderSystem::onReceive(const aka::BackbufferResizeEvent& e)
{
	createRenderTargets(e.width, e.height);
}

void RenderSystem::onReceive(const aka::ProgramReloadedEvent& e)
{
	if (e.name == "gbuffer")
		m_gbufferMaterial = Material::create(e.program);
	else if (e.name == "point")
		m_pointMaterial = Material::create(e.program);
	else if (e.name == "directional")
		m_dirMaterial = Material::create(e.program);
	else if (e.name == "ambient")
		m_ambientMaterial = Material::create(e.program);
	else if (e.name == "skybox")
		m_skyboxMaterial = Material::create(e.program);
	else if (e.name == "postProcess")
		m_postprocessMaterial = Material::create(e.program);
	else if (e.name == "text")
		m_textMaterial = Material::create(e.program);
}

void RenderSystem::createRenderTargets(uint32_t width, uint32_t height)
{
	// --- G-Buffer pass
	//
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
	m_depth = Texture2D::create(width, height, TextureFormat::DepthStencil, TextureFlag::RenderTarget | TextureFlag::ShaderResource);
	m_position = Texture2D::create(width, height, TextureFormat::RGBA16F, TextureFlag::RenderTarget | TextureFlag::ShaderResource);
	m_albedo = Texture2D::create(width, height, TextureFormat::RGBA8, TextureFlag::RenderTarget | TextureFlag::ShaderResource);
	m_normal = Texture2D::create(width, height, TextureFormat::RGBA16F, TextureFlag::RenderTarget | TextureFlag::ShaderResource);
	m_material = Texture2D::create(width, height, TextureFormat::RGBA16F, TextureFlag::RenderTarget | TextureFlag::ShaderResource);
	Attachment gbufferAttachments[] = {
		Attachment{ AttachmentType::DepthStencil, m_depth, AttachmentFlag::None, 0, 0 },
		Attachment{ AttachmentType::Color0, m_position, AttachmentFlag::None, 0, 0 },
		Attachment{ AttachmentType::Color1, m_albedo, AttachmentFlag::None, 0, 0 },
		Attachment{ AttachmentType::Color2, m_normal, AttachmentFlag::None, 0, 0 },
		Attachment{ AttachmentType::Color3, m_material, AttachmentFlag::None, 0, 0 }
	};
	m_gbuffer = Framebuffer::create(gbufferAttachments, sizeof(gbufferAttachments) / sizeof(Attachment));

	// --- Post process
	m_storageDepth = Texture2D::create(width, height, TextureFormat::DepthStencil, TextureFlag::RenderTarget | TextureFlag::ShaderResource);
	m_storage = Texture2D::create(width, height, TextureFormat::RGBA16F, TextureFlag::RenderTarget | TextureFlag::ShaderResource);
	Attachment storageAttachment[] = {
		Attachment{ AttachmentType::DepthStencil, m_storageDepth, AttachmentFlag::None, 0, 0 },
		Attachment{ AttachmentType::Color0, m_storage, AttachmentFlag::None, 0, 0 }
	};
	m_storageFramebuffer = Framebuffer::create(storageAttachment, 2);
}

};