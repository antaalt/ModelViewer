#include "RenderSystem.h"

#include "../Model/Model.h"

namespace app {

using namespace aka;

// Array of type are aligned as 16 in std140 (???) 
struct alignas(16) Aligned16Float {
	float data;
};

// --- Lights
struct alignas(16) DirectionalLightUniformBuffer {
	alignas(16) vec3f direction;
	alignas(4) float intensity;
	alignas(16) vec3f color;
	alignas(16) mat4f worldToLightTextureSpace[DirectionalLightComponent::cascadeCount];
	alignas(16) Aligned16Float cascadeEndClipSpace[DirectionalLightComponent::cascadeCount];
};
struct alignas(16) PointLightUniformBuffer {
	alignas(16) mat4f model;
	alignas(16) vec3f lightPosition;
	alignas(4) float lightIntensity;
	alignas(16) color3f lightColor;
	alignas(4) float farPointLight;
};

// --- View
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
// --- Object
struct alignas(16) MaterialUniformBuffer {
	alignas(16) color4f color;
};
// --- Instance
struct alignas(16) MatricesUniformBuffer {
	alignas(16) mat4f model;
	alignas(16) vec3f normalMatrix0;
	alignas(16) vec3f normalMatrix1;
	alignas(16) vec3f normalMatrix2;
};

void RenderSystem::onCreate(aka::World& world)
{
	Application* app = Application::app();
	GraphicDevice* device = app->graphic();

	ProgramManager* program = app->program();
	m_gbufferProgram = program->get("gbuffer");
	m_pointProgram = program->get("point"); m_pointDescriptorSet = device->createDescriptorSet(m_pointProgram->bindings[0]);
	m_dirProgram = program->get("directional"); m_dirDescriptorSet = device->createDescriptorSet(m_dirProgram->bindings[0]);
	m_ambientProgram = program->get("ambient"); m_ambientDescriptorSet = device->createDescriptorSet(m_ambientProgram->bindings[0]);
	m_skyboxProgram = program->get("skybox"); m_skyboxDescriptorSet = device->createDescriptorSet(m_skyboxProgram->bindings[0]);
	m_postprocessProgram = program->get("postProcess"); m_postprocessDescriptorSet = device->createDescriptorSet(m_postprocessProgram->bindings[0]);
	//m_textDescriptorSet = device->createDescriptorSet(0, program->get("text"));
	m_viewDescriptorSet = device->createDescriptorSet(m_gbufferProgram->bindings[0]);

	// --- Uniforms
	m_cameraUniformBuffer = device->createBuffer(BufferType::Uniform, sizeof(CameraUniformBuffer), BufferUsage::Default, BufferCPUAccess::None);
	m_viewportUniformBuffer = device->createBuffer(BufferType::Uniform, sizeof(ViewportUniformBuffer), BufferUsage::Default, BufferCPUAccess::None);

	// --- Lighting pass
	m_defaultSampler = device->createSampler(
		Filter::Nearest,
		Filter::Nearest,
		SamplerMipMapMode::None,
		1,
		SamplerAddressMode::ClampToEdge,
		SamplerAddressMode::ClampToEdge,
		SamplerAddressMode::ClampToEdge,
		1.f
	);
	m_shadowSampler = device->createSampler(
		Filter::Nearest,
		Filter::Nearest,
		SamplerMipMapMode::None,
		1,
		SamplerAddressMode::ClampToEdge,
		SamplerAddressMode::ClampToEdge,
		SamplerAddressMode::ClampToEdge,
		1.f
	);

	float quadVertices[] = {
		-1.f, -1.f, // bottom left corner
		 1.f, -1.f, // bottom right corner
		 1.f,  1.f, // top right corner
		-1.f,  1.f, // top left corner
	};
	uint16_t quadIndices[] = { 0,1,2,0,2,3 };
	m_quad = Mesh::create();
	m_quad->bindings.attributes[0] = VertexAttribute{ VertexSemantic::Position, VertexFormat::Float, VertexType::Vec2 };
	m_quad->bindings.offsets[0] = 0;
	m_quad->bindings.count = 1;
	m_quad->vertices[0] = device->createBuffer(BufferType::Vertex, sizeof(quadVertices), BufferUsage::Default, BufferCPUAccess::None, quadVertices);
	m_quad->indices = device->createBuffer(BufferType::Index, sizeof(quadIndices), BufferUsage::Default, BufferCPUAccess::None, quadIndices);
	m_quad->format = IndexFormat::UnsignedShort;
	m_quad->count = 6;

	m_sphere = Scene::createSphereMesh(point3f(0.f), 1.f, 8, 8);

	// --- Skybox pass
	uint8_t data[] = { 
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		200, 200, 200, 255, 200, 200, 200, 255, 200, 200, 200, 255, 200, 200, 200, 255, 200, 200, 200, 255, 200, 200, 200, 255, 200, 200, 200, 255, 200, 200, 200, 255,
		170, 170, 170, 255, 170, 170, 170, 255, 170, 170, 170, 255, 170, 170, 170, 255, 170, 170, 170, 255, 170, 170, 170, 255, 170, 170, 170, 255, 170, 170, 170, 255,
		140, 140, 140, 255, 140, 140, 140, 255, 140, 140, 140, 255, 140, 140, 140, 255, 140, 140, 140, 255, 140, 140, 140, 255, 140, 140, 140, 255, 140, 140, 140, 255,
		110, 110, 110, 255, 110, 110, 110, 255, 110, 110, 110, 255, 110, 110, 110, 255, 110, 110, 110, 255, 110, 110, 110, 255, 110, 110, 110, 255, 110, 110, 110, 255,
		 90,  90,  90, 255,  90,  90,  90, 255,  90,  90,  90, 255,  90,  90,  90, 255,  90,  90,  90, 255,  90,  90,  90, 255,  90,  90,  90, 255,  90,  90,  90, 255,
		 60,  60,  60, 255,  60,  60,  60, 255,  60,  60,  60, 255,  60,  60,  60, 255,  60,  60,  60, 255,  60,  60,  60, 255,  60,  60,  60, 255,  60,  60,  60, 255,
		 30,  30,  30, 255,  30,  30,  30, 255,  30,  30,  30, 255,  30,  30,  30, 255,  30,  30,  30, 255,  30,  30,  30, 255,  30,  30,  30, 255,  30,  30,  30, 255,
	};
	void* datas[6];
	for (int i = 0; i < 6; i++)
		datas[i] = data;
	m_skybox = Texture::createCubemap(8, 8, TextureFormat::RGBA8, TextureFlag::ShaderResource, datas);
	//Texture* equirectangularMap = Texture::create2D(4, 8, TextureFormat::RGBA8, TextureFlag::ShaderResource, data);
	//m_skybox = Texture::generate(512, 512, TextureFormat::RGBA8, TextureFlag::ShaderResource, equirectangularMap, Filter::Linear);
	m_skyboxSampler = device->createSampler(
		Filter::Linear,
		Filter::Linear,
		SamplerMipMapMode::None,
		1,
		SamplerAddressMode::ClampToEdge,
		SamplerAddressMode::ClampToEdge,
		SamplerAddressMode::ClampToEdge,
		1.f
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
	m_cube->bindings.attributes[0] = VertexAttribute{ VertexSemantic::Position, VertexFormat::Float, VertexType::Vec3 };
	m_cube->bindings.offsets[0] = 0;
	m_cube->bindings.count = 1;
	m_cube->vertices[0] = device->createBuffer(BufferType::Vertex, sizeof(skyboxVertices), BufferUsage::Default, BufferCPUAccess::None, skyboxVertices);
	m_cube->indices = nullptr;
	m_cube->format = IndexFormat::UnsignedShort;
	m_cube->count = 36;

	createRenderTargets(app->width(), app->height());
}

void RenderSystem::onDestroy(aka::World& world)
{
	Application* app = Application::app();
	GraphicDevice* device = app->graphic();

	auto renderableView = world.registry().view<RenderComponent>();
	for (entt::entity e : renderableView)
	{
		RenderComponent& r = world.registry().get<RenderComponent>(e);
		device->destroy(r.ubo[0]);
		device->destroy(r.ubo[1]);
		device->destroy(r.material);
		device->destroy(r.matrices);
	}
	// Gbuffer pass
	device->destroy(m_position);
	device->destroy(m_albedo);
	device->destroy(m_normal);
	device->destroy(m_depth);
	device->destroy(m_material);
	device->destroy(m_gbuffer);
	device->destroy(m_viewDescriptorSet);
	device->destroy(m_defaultSampler);
	device->destroy(m_shadowSampler);

	// Lighing pass
	device->destroy(m_quad->indices);
	device->destroy(m_quad->vertices[0]);
	device->destroy(m_sphere->indices);
	device->destroy(m_sphere->vertices[0]);
	device->destroy(m_ambientPipeline);
	device->destroy(m_ambientDescriptorSet);
	device->destroy(m_pointPipeline);
	device->destroy(m_pointDescriptorSet);
	device->destroy(m_dirDescriptorSet);
	device->destroy(m_dirPipeline);

	// Skybox
	device->destroy(m_cube->indices);
	device->destroy(m_cube->vertices[0]);
	device->destroy(m_skybox);
	device->destroy(m_skyboxDescriptorSet);
	device->destroy(m_skyboxSampler);

	// Post process pass
	device->destroy(m_storage);
	device->destroy(m_storageFramebuffer);
	device->destroy(m_storageDepthFramebuffer);
	device->destroy(m_postprocessDescriptorSet);

	// Uniforms
	device->destroy(m_cameraUniformBuffer);
	device->destroy(m_viewportUniformBuffer);

	// program and shaders self destroyed
}

void RenderSystem::onRender(aka::World& world, aka::Frame* frame)
{
	Application* app = Application::app();
	GraphicDevice* device = app->graphic();

	Framebuffer* backbuffer = device->backbuffer(frame); // TODO retrieve frame (check if is in render thread)

	Entity cameraEntity = Scene::getMainCamera(world);
	Camera3DComponent& camera = cameraEntity.get<Camera3DComponent>();
	mat4f view = camera.view;
	mat4f projection = camera.projection->projection();

	// --- Update Uniforms
	m_postprocessDescriptorSet->setSampledImage(0, m_storage, m_shadowSampler);
	m_postprocessDescriptorSet->setUniformBuffer(1, m_viewportUniformBuffer);
	device->update(m_postprocessDescriptorSet);

	// TODO only update on camera move / update
	CameraUniformBuffer cameraUBO;
	cameraUBO.view = view;
	cameraUBO.projection = projection;
	cameraUBO.viewInverse = cameraEntity.get<Transform3DComponent>().transform;
	cameraUBO.projectionInverse = mat4f::inverse(projection);
	device->upload(m_cameraUniformBuffer, &cameraUBO, 0, sizeof(CameraUniformBuffer));
	ViewportUniformBuffer viewportUBO;
	viewportUBO.viewport = vec2f(app->width(), app->height());
	device->upload(m_viewportUniformBuffer , &viewportUBO, 0, sizeof(ViewportUniformBuffer));

	// Samplers
	Sampler* samplers[] = { m_shadowSampler , m_shadowSampler , m_shadowSampler };

	// Add render data to every component
	// View
	m_viewDescriptorSet->setUniformBuffer(0, m_cameraUniformBuffer);
	m_viewDescriptorSet->setUniformBuffer(1, m_viewportUniformBuffer);
	device->update(m_viewDescriptorSet);

	auto notRenderSetView = world.registry().view<Transform3DComponent, MeshComponent, OpaqueMaterialComponent>(entt::exclude<RenderComponent>);
	for (entt::entity e : notRenderSetView)
	{
		// TODO move to rendercomponent creation
		RenderComponent& r = world.registry().emplace<RenderComponent>(e);
		Transform3DComponent& transform = world.registry().get<Transform3DComponent>(e);
		OpaqueMaterialComponent& material = world.registry().get<OpaqueMaterialComponent>(e);
		//MeshComponent& mesh = world.registry().get<MeshComponent>(e);

		// Matrices
		MatricesUniformBuffer matricesUBO;
		matricesUBO.model = transform.transform;
		mat3f normalMatrix = mat3f::transpose(mat3f::inverse(mat3f(transform.transform)));
		matricesUBO.normalMatrix0 = vec3f(normalMatrix[0]);
		matricesUBO.normalMatrix1 = vec3f(normalMatrix[1]);
		matricesUBO.normalMatrix2 = vec3f(normalMatrix[2]);
		r.matrices = device->createDescriptorSet(m_gbufferProgram->bindings[2]);
		r.ubo[0] = device->createBuffer(BufferType::Uniform, sizeof(MatricesUniformBuffer), BufferUsage::Default, BufferCPUAccess::None, &matricesUBO);
		r.matrices->setUniformBuffer(0, r.ubo[0]);
		device->update(r.matrices);


		// DescriptorSet
		MaterialUniformBuffer materialUBO;
		materialUBO.color = material.color;
		r.ubo[1] = device->createBuffer(BufferType::Uniform, sizeof(MaterialUniformBuffer), BufferUsage::Default, BufferCPUAccess::None, &materialUBO);
		r.material = device->createDescriptorSet(m_gbufferProgram->bindings[1]);
		r.material->setUniformBuffer(0, r.ubo[1]);
		r.material->setSampledImage(1, material.albedo.texture, material.albedo.sampler);
		r.material->setSampledImage(2, material.normal.texture, material.normal.sampler);
		r.material->setSampledImage(3, material.material.texture, material.material.sampler);
		device->update(r.material);
	}

	auto renderableView = world.registry().view<Transform3DComponent, MeshComponent, OpaqueMaterialComponent, RenderComponent>();

	// --- G-Buffer pass
	// TODO depth prepass
	CommandList* cmd = frame->commandList;
	cmd->bindPipeline(m_gbufferPipeline);
	cmd->beginRenderPass(m_gbuffer, ClearState{ ClearMask::None, {0.f}, 1.f, 0 });
	renderableView.each([&](const Transform3DComponent& transform, const MeshComponent& mesh, const OpaqueMaterialComponent& material, const RenderComponent& rendering) {
		frustum<>::planes p = frustum<>::extract(projection * view);
		// Check intersection in camera space
		// https://www.gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf
		if (!p.intersect(transform.transform * mesh.bounds))
			return;

		DescriptorSet* materials[3] = { m_viewDescriptorSet, rendering.material, rendering.matrices };
		cmd->bindIndexBuffer(mesh.mesh->indices, mesh.mesh->format, 0);
		cmd->bindVertexBuffer(mesh.mesh->vertices, 0, 1, mesh.mesh->bindings.offsets);
		cmd->bindDescriptorSets(materials, 3);

		cmd->drawIndexed(mesh.mesh->count, 0, 0, 1);
	});
	cmd->endRenderPass();

	// --- Lighting pass
	static const mat4f projectionToTextureCoordinateMatrix(
#if defined(GEOMETRY_CLIP_SPACE_NEGATIVE)
		col4f(0.5, 0.0, 0.0, 0.0),
		col4f(0.0, 0.5, 0.0, 0.0),
		col4f(0.0, 0.0, 0.5, 0.0),
		col4f(0.5, 0.5, 0.5, 1.0)
#elif defined(GEOMETRY_CLIP_SPACE_POSITIVE)
		col4f(0.5, 0.0, 0.0, 0.0),
		col4f(0.0, 0.5, 0.0, 0.0),
		col4f(0.0, 0.0, 1.0, 0.0),
		col4f(0.5, 0.5, 0.0, 1.0)
#endif
	);

	// --- Ambient light
	m_ambientDescriptorSet->setSampledImage(0, m_position, m_defaultSampler);
	m_ambientDescriptorSet->setSampledImage(1, m_albedo, m_defaultSampler);
	m_ambientDescriptorSet->setSampledImage(2, m_normal, m_defaultSampler);
	m_ambientDescriptorSet->setSampledImage(3, m_skybox, m_defaultSampler);
	m_ambientDescriptorSet->setUniformBuffer(4, m_cameraUniformBuffer);
	device->update(m_ambientDescriptorSet);

	cmd->bindPipeline(m_ambientPipeline);
	cmd->bindIndexBuffer(m_quad->indices, m_quad->format, 0);
	cmd->bindVertexBuffer(m_quad->vertices, 0, 1, m_quad->bindings.offsets);
	cmd->bindDescriptorSet(0, m_ambientDescriptorSet);

	cmd->beginRenderPass(m_storageFramebuffer, ClearState{ ClearMask::None, {0.f, 0.f, 0.f, 1.f}, 1.f, 1 });

	cmd->drawIndexed(m_quad->count, 0, 0, 1);


	// --- Directional lights
	m_dirDescriptorSet->setSampledImage(0, m_position, m_defaultSampler);
	m_dirDescriptorSet->setSampledImage(1, m_albedo, m_defaultSampler);
	m_dirDescriptorSet->setSampledImage(2, m_normal, m_defaultSampler);
	m_dirDescriptorSet->setSampledImage(3, m_depth, m_defaultSampler);
	m_dirDescriptorSet->setSampledImage(4, m_material, m_defaultSampler);
	m_dirDescriptorSet->setUniformBuffer(5, m_cameraUniformBuffer);
	device->update(m_dirDescriptorSet);

	cmd->bindPipeline(m_dirPipeline);
	
	auto directionalShadows = world.registry().view<Transform3DComponent, DirectionalLightComponent>();
	directionalShadows.each([&](const Transform3DComponent& transform, DirectionalLightComponent& light){

		if (light.renderDescriptorSet == nullptr)
		{
			// Init is here because we dont have access to DirectionalLightUniformBuffer in shadowMapSystem
			light.renderDescriptorSet = device->createDescriptorSet(m_dirProgram->bindings[1]);
			light.renderUBO = device->createBuffer(BufferType::Uniform, sizeof(DirectionalLightUniformBuffer), BufferUsage::Default, BufferCPUAccess::None);
			light.renderDescriptorSet->setUniformBuffer(0, light.renderUBO);
			light.renderDescriptorSet->setSampledImage(1, light.shadowMap, m_defaultSampler);
			device->update(light.renderDescriptorSet);
		}
		//if (dirty)
		{
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
			device->upload(light.renderUBO, &directionalUBO, 0, sizeof(DirectionalLightUniformBuffer));
		}

		DescriptorSet* materials[2] = { m_dirDescriptorSet, light.renderDescriptorSet };
		cmd->bindIndexBuffer(m_quad->indices, m_quad->format, 0);
		cmd->bindVertexBuffer(m_quad->vertices, 0, 1, m_quad->bindings.offsets);
		cmd->bindDescriptorSets(materials, 2);

		cmd->drawIndexed(m_quad->count, 0, 0, 1);
	});

	// --- Point lights
	// Using light volumes
	m_pointDescriptorSet->setSampledImage(0, m_position, m_defaultSampler);
	m_pointDescriptorSet->setSampledImage(1, m_albedo, m_defaultSampler);
	m_pointDescriptorSet->setSampledImage(2, m_normal, m_defaultSampler);
	m_pointDescriptorSet->setSampledImage(3, m_depth, m_defaultSampler);
	m_pointDescriptorSet->setSampledImage(4, m_material, m_defaultSampler);
	m_pointDescriptorSet->setUniformBuffer(5, m_cameraUniformBuffer);
	m_pointDescriptorSet->setUniformBuffer(6, m_viewportUniformBuffer);
	device->update(m_pointDescriptorSet);

	cmd->bindPipeline(m_pointPipeline);

	auto pointShadows = world.registry().view<Transform3DComponent, PointLightComponent>();
	pointShadows.each([&](const Transform3DComponent& transform, PointLightComponent& light) {
		point3f position(transform.transform.cols[3]);
		aabbox<> bounds(position + vec3f(-light.radius), position + vec3f(light.radius));
		frustum<>::planes p = frustum<>::extract(projection * view);
		if (!p.intersect(bounds))
			return;

		if (light.renderDescriptorSet == nullptr)
		{
			// Init is here because we dont have access to PointLightUniformBuffer in shadowMapSystem
			light.renderDescriptorSet = device->createDescriptorSet(m_pointProgram->bindings[1]);
			light.renderUBO = device->createBuffer(BufferType::Uniform, sizeof(PointLightUniformBuffer), BufferUsage::Default, BufferCPUAccess::None);
			light.renderDescriptorSet->setUniformBuffer(0, light.renderUBO);
			light.renderDescriptorSet->setSampledImage(1, light.shadowMap, m_defaultSampler);
			device->update(light.renderDescriptorSet);
		}
		//if (dirty)
		{
			PointLightUniformBuffer pointUBO;
			pointUBO.model = transform.transform * mat4f::scale(vec3f(light.radius));
			pointUBO.lightPosition = vec3f(position);
			pointUBO.lightIntensity = light.intensity;
			pointUBO.lightColor = light.color;
			pointUBO.farPointLight = light.radius;
			device->upload(light.renderUBO, &pointUBO, 0, sizeof(PointLightUniformBuffer));
		}

		DescriptorSet* materials[2] = { m_pointDescriptorSet, light.renderDescriptorSet };
		cmd->bindIndexBuffer(m_sphere->indices, m_sphere->format, 0);
		cmd->bindVertexBuffer(m_sphere->vertices, 0, 1, m_sphere->bindings.offsets);
		cmd->bindDescriptorSets(materials, 2);

		cmd->drawIndexed(m_sphere->count, 0, 0, 1);
	});
	cmd->endRenderPass();

	// --- Skybox pass
	m_skyboxDescriptorSet->setUniformBuffer(0, m_cameraUniformBuffer);
	m_skyboxDescriptorSet->setSampledImage(1, m_skybox, m_skyboxSampler);
	device->update(m_skyboxDescriptorSet);

	cmd->beginRenderPass(m_storageDepthFramebuffer, ClearState{});
	cmd->bindPipeline(m_skyboxPipeline);
	cmd->bindVertexBuffer(m_cube->vertices, 0, 1, m_cube->bindings.offsets);
	cmd->bindDescriptorSet(0, m_skyboxDescriptorSet);

	cmd->draw(m_cube->count, 0, 1);
	cmd->endRenderPass();


	/*// --- Text pass
	// Text should generate a mesh with its UV & co. This way, it will be easier to handle it.
	// Dirty rendering for now, close your eyes.
	RenderPass textPass;
	textPass.framebuffer = m_storageFramebuffer;
	textPass.material = m_textDescriptorSet;
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
	});*/


	// --- Post process pass
	cmd->bindPipeline(m_postPipeline);
	cmd->bindDescriptorSet(0, m_postprocessDescriptorSet);
	cmd->bindVertexBuffer(m_quad->vertices, 0, 1, m_quad->bindings.offsets);
	cmd->bindIndexBuffer(m_quad->indices, m_quad->format, 0);

	cmd->beginRenderPass(backbuffer, ClearState{});
	cmd->drawIndexed(m_quad->count, 0, 0, 1);
	cmd->endRenderPass();

	// Set depth for UI elements
	//backbuffer->blit(m_storageDepth, TextureFilter::Nearest);
}

void RenderSystem::onReceive(const aka::BackbufferResizeEvent& e)
{
	createRenderTargets(e.width, e.height);
}

void RenderSystem::onReceive(const aka::ProgramReloadedEvent& e)
{
	Application* app = Application::app();
	GraphicDevice* device = app->graphic();

	// TODO fix hot reload
	if (e.name == "gbuffer")
		m_gbufferProgram = e.program;
	else if (e.name == "point")
		m_pointProgram = e.program;
	else if (e.name == "directional")
		m_dirProgram = e.program;
	else if (e.name == "ambient")
		m_ambientProgram = e.program;
	else if (e.name == "skybox")
		m_skyboxProgram = e.program;
	else if (e.name == "postProcess")
		m_postprocessProgram = e.program;
	//else if (e.name == "text")
	//	m_textDescriptorSet = e.program;
}

void RenderSystem::createRenderTargets(uint32_t width, uint32_t height)
{
	Application* app = Application::app();
	GraphicDevice* device = app->graphic();

	ViewportState viewport{ Rect{0, 0, app->width(), app->height()}, Rect{0, 0, app->width(), app->height()} };

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
		FramebufferState fbDesc;
		fbDesc.colors[0].format = TextureFormat::RGBA16F;
		fbDesc.colors[1].format = TextureFormat::RGBA8;
		fbDesc.colors[2].format = TextureFormat::RGBA16F;
		fbDesc.colors[3].format = TextureFormat::RGBA16F;
		fbDesc.depth.format = TextureFormat::DepthStencil;
		fbDesc.count = 4;
		m_depth = Texture::create2D(width, height, fbDesc.depth.format, TextureFlag::RenderTarget | TextureFlag::ShaderResource);
		m_position = Texture::create2D(width, height, fbDesc.colors[0].format, TextureFlag::RenderTarget | TextureFlag::ShaderResource);
		m_albedo = Texture::create2D(width, height, fbDesc.colors[1].format, TextureFlag::RenderTarget | TextureFlag::ShaderResource);
		m_normal = Texture::create2D(width, height, fbDesc.colors[2].format, TextureFlag::RenderTarget | TextureFlag::ShaderResource);
		m_material = Texture::create2D(width, height, fbDesc.colors[3].format, TextureFlag::RenderTarget | TextureFlag::ShaderResource);
		Attachment colorAttachments[] = {
			Attachment{ m_position, AttachmentFlag::None, AttachmentLoadOp::Clear, 0, 0 },
			Attachment{ m_albedo, AttachmentFlag::None, AttachmentLoadOp::Clear, 0, 0 },
			Attachment{ m_normal, AttachmentFlag::None, AttachmentLoadOp::Clear, 0, 0 },
			Attachment{ m_material, AttachmentFlag::None, AttachmentLoadOp::Clear, 0, 0 }
		};
		Attachment depthAttachment = Attachment{ m_depth, AttachmentFlag::None, AttachmentLoadOp::Clear,  0, 0 };
		m_gbuffer = device->createFramebuffer(colorAttachments, sizeof(colorAttachments) / sizeof(Attachment), &depthAttachment);

		m_gbufferVertices.attributes[0] = VertexAttribute{ VertexSemantic::Position, VertexFormat::Float, VertexType::Vec3 };
		m_gbufferVertices.attributes[1] = VertexAttribute{ VertexSemantic::Normal, VertexFormat::Float, VertexType::Vec3 };
		m_gbufferVertices.attributes[2] = VertexAttribute{ VertexSemantic::TexCoord0, VertexFormat::Float, VertexType::Vec2 };
		m_gbufferVertices.attributes[3] = VertexAttribute{ VertexSemantic::Color0, VertexFormat::Float, VertexType::Vec4 };
		m_gbufferVertices.count = 4;
		m_gbufferVertices.offsets[0] = offsetof(Vertex, position);
		m_gbufferVertices.offsets[1] = offsetof(Vertex, normal);
		m_gbufferVertices.offsets[2] = offsetof(Vertex, texcoord);
		m_gbufferVertices.offsets[3] = offsetof(Vertex, color);

		Shader* shaders[2] = { m_gbufferProgram->vertex, m_gbufferProgram->fragment };
		m_gbufferPipeline = device->createPipeline(
			shaders,
			2,
			PrimitiveType::Triangles,
			fbDesc,
			m_gbufferVertices,
			m_gbufferProgram->bindings,
			m_gbufferProgram->setCount,
			viewport,
			DepthState{ DepthOp::Less, true },
			StencilState{ StencilState::Face { StencilMode::Keep, StencilMode::Keep, StencilMode::Keep, StencilOp::None }, StencilState::Face {StencilMode::Keep, StencilMode::Keep, StencilMode::Keep, StencilOp::None }, 0xff, 0xff },
			CullState{ CullMode::BackFace, CullOrder::CounterClockWise },
			BlendState{ BlendMode::One, BlendMode::Zero, BlendOp::Add,BlendMode::One, BlendMode::Zero, BlendOp::Add, BlendMask::Rgba, 0xff }, 
			FillState{ FillMode::Fill, 1.f } 
		);
	}
	{
		// --- Storage
		//m_storageDepth = Texture::create2D(width, height, TextureFormat::DepthStencil, TextureFlag::RenderTarget | TextureFlag::ShaderResource);
		m_storage = Texture::create2D(width, height, TextureFormat::RGBA16F, TextureFlag::RenderTarget | TextureFlag::ShaderResource);
		Attachment depth = Attachment{ m_depth, AttachmentFlag::None, AttachmentLoadOp::Load, 0, 0 };
		Attachment color = Attachment{ m_storage, AttachmentFlag::None, AttachmentLoadOp::Clear, 0, 0 };
		m_storageFramebuffer = device->createFramebuffer(&color, 1, nullptr);
		color.loadOp = AttachmentLoadOp::Load;
		m_storageDepthFramebuffer = device->createFramebuffer(&color, 1, &depth);
	}
	{
		// --- Ambient
		Shader* shaders[2] = { m_ambientProgram->vertex, m_ambientProgram->fragment };
		m_ambientPipeline = device->createPipeline(
			shaders,
			2,
			PrimitiveType::Triangles,
			m_storageFramebuffer->framebuffer,
			m_quad->bindings,
			m_ambientProgram->bindings,
			m_ambientProgram->setCount,
			viewport,
			DepthState{},
			StencilState{},
			CullState{ CullMode::BackFace, CullOrder::CounterClockWise },
			BlendState{ BlendMode::One, BlendMode::One, BlendOp::Add, BlendMode::One, BlendMode::Zero, BlendOp::Add, BlendMask::Rgb, 0xff },
			FillState{ FillMode::Fill, 1.f }
		);
	}

	{
		// --- Directional
		Shader* shaders[2] = { m_dirProgram->vertex, m_dirProgram->fragment };
		m_dirPipeline = device->createPipeline(
			shaders,
			2,
			PrimitiveType::Triangles,
			m_storageFramebuffer->framebuffer,
			m_quad->bindings,
			m_dirProgram->bindings,
			m_dirProgram->setCount,
			viewport,
			DepthState{},
			StencilState{},
			CullState{ CullMode::BackFace, CullOrder::CounterClockWise },
			BlendState{ BlendMode::One, BlendMode::One, BlendOp::Add, BlendMode::One, BlendMode::Zero, BlendOp::Add, BlendMask::Rgb, 0xff },
			FillState{ FillMode::Fill, 1.f }
		);
	}

	{
		// --- Point
		Shader* shaders[2] = { m_pointProgram->vertex, m_pointProgram->fragment };
		m_pointPipeline = device->createPipeline(
			shaders,
			2,
			PrimitiveType::Triangles,
			m_storageFramebuffer->framebuffer,
			m_sphere->bindings,
			m_pointProgram->bindings,
			m_pointProgram->setCount,
			viewport,
			DepthState{},
			StencilState{},
			CullState{ CullMode::FrontFace, CullOrder::CounterClockWise }, // Important to avoid rendering 2 times or clipping
			BlendState{ BlendMode::One, BlendMode::One, BlendOp::Add, BlendMode::One, BlendMode::Zero, BlendOp::Add, BlendMask::Rgb, 0xff },
			FillState{ FillMode::Fill, 1.f }
		);
	}

	{
		// --- Skybox
		Shader* shaders[2] = { m_skyboxProgram->vertex, m_skyboxProgram->fragment };
		m_skyboxPipeline = device->createPipeline(
			shaders,
			2,
			PrimitiveType::Triangles,
			m_storageDepthFramebuffer->framebuffer,
			m_cube->bindings,
			m_skyboxProgram->bindings,
			1,
			viewport,
			DepthState{ DepthOp::LessOrEqual, false },
			StencilState{},
			CullState{ CullMode::BackFace, CullOrder::CounterClockWise }, 
			BlendState{ BlendMode::One, BlendMode::Zero, BlendOp::Add, BlendMode::One, BlendMode::Zero, BlendOp::Add, BlendMask::Rgba, 0xff }, 
			FillState{ FillMode::Fill, 1.f } 
		);
	}

	{
		// --- Post process
		// TODO get backbuffer fbDesc.
		// TODO blit as it is for storage
		FramebufferState fbDesc{};
		fbDesc.colors[0].format = TextureFormat::BGRA8;
		fbDesc.colors[0].loadOp = AttachmentLoadOp::Load;
		fbDesc.depth.format = TextureFormat::Depth32F;
		fbDesc.depth.loadOp = AttachmentLoadOp::Load;
		fbDesc.count = 1;

		Shader* shaders[2] = { m_postprocessProgram->vertex, m_postprocessProgram->fragment};
		m_postPipeline = device->createPipeline(
			shaders,
			2,
			PrimitiveType::Triangles,
			fbDesc,
			m_quad->bindings,
			m_postprocessProgram->bindings,
			1,
			viewport,
			DepthState{},
			StencilState{},
			CullState{ CullMode::BackFace, CullOrder::CounterClockWise },
			BlendState{ BlendMode::One, BlendMode::Zero, BlendOp::Add,BlendMode::One, BlendMode::Zero, BlendOp::Add, BlendMask::Rgba, 0xff },
			FillState{ FillMode::Fill, 1.f }
		);
	}
}

};