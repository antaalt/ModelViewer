#include "ShadowMapSystem.h"

#include "../Model/Model.h"

namespace app {

using namespace aka;

struct alignas(16) DirectionalLightUniformBuffer {
	alignas(16) mat4f light;
};

struct alignas(16) PointLightUniformBuffer {
	alignas(16) mat4f lightView;
	alignas(16) point3f lightPos;
	alignas(4) float far;
};

void onDirectionalLightConstruct(entt::registry& registry, entt::entity entity)
{
	DirectionalLightComponent& l = registry.get<DirectionalLightComponent>(entity);
	l.shadowMap = Texture::create2DArray(
		DirectionalLightComponent::cascadeResolution, 
		DirectionalLightComponent::cascadeResolution, 
		DirectionalLightComponent::cascadeCount, 
		TextureFormat::Depth, 
		TextureFlag::RenderTarget | TextureFlag::ShaderResource
	);
	for (uint32_t iLayer = 0; iLayer < DirectionalLightComponent::cascadeCount; iLayer++)
	{
		// TODO use push constant to send cascade index instead.
		l.ubo[iLayer] = Buffer::createUniformBuffer(sizeof(DirectionalLightUniformBuffer), BufferUsage::Default, BufferCPUAccess::None, nullptr);
		Attachment shadowAttachment = { l.shadowMap, AttachmentFlag::None, AttachmentLoadOp::Clear, iLayer, 0 };
		l.framebuffer[iLayer] = Framebuffer::create(nullptr, 0, &shadowAttachment);
		l.descriptorSet[iLayer] = DescriptorSet::create(Application::app()->program()->get("shadowDirectional")->bindings[0]);
		l.descriptorSet[iLayer]->setUniformBuffer(0, l.ubo[iLayer]);
	}
	if (!registry.has<DirtyLightComponent>(entity))
		registry.emplace<DirtyLightComponent>(entity);
}

void onDirectionalLightDestroy(entt::registry& registry, entt::entity entity)
{
	DirectionalLightComponent& l = registry.get<DirectionalLightComponent>(entity);
	for (uint32_t i = 0; i < DirectionalLightComponent::cascadeCount; i++)
	{
		Buffer::destroy(l.ubo[i]);
		Framebuffer::destroy(l.framebuffer[i]);
		DescriptorSet::destroy(l.descriptorSet[i]);
	}
	DescriptorSet::destroy(l.renderDescriptorSet);
	Buffer::destroy(l.renderUBO);
	Texture::destroy(l.shadowMap);
}

void onPointLightConstruct(entt::registry& registry, entt::entity entity)
{
	PointLightComponent& l = registry.get<PointLightComponent>(entity);
	l.shadowMap = Texture::createCubemap(PointLightComponent::faceResolution, PointLightComponent::faceResolution, TextureFormat::Depth, TextureFlag::RenderTarget | TextureFlag::ShaderResource);

	for (uint32_t iLayer = 0; iLayer < 6; iLayer++)
	{
		l.ubo[iLayer] = Buffer::createUniformBuffer(sizeof(PointLightUniformBuffer), BufferUsage::Default, BufferCPUAccess::None, nullptr);
		Attachment shadowAttachment = { l.shadowMap, AttachmentFlag::None, AttachmentLoadOp::Clear, iLayer, 0 };
		l.framebuffer[iLayer] = Framebuffer::create(nullptr, 0, &shadowAttachment);
		l.descriptorSet[iLayer] = DescriptorSet::create(Application::app()->program()->get("shadowPoint")->bindings[0]);
		l.descriptorSet[iLayer]->setUniformBuffer(0, l.ubo[iLayer]);
	}
	if (!registry.has<DirtyLightComponent>(entity))
		registry.emplace<DirtyLightComponent>(entity);
}

void onPointLightDestroy(entt::registry& registry, entt::entity entity)
{
	PointLightComponent& l = registry.get<PointLightComponent>(entity);
	for (uint32_t iLayer = 0; iLayer < 6; iLayer++)
	{
		Buffer::destroy(l.ubo[iLayer]);
		Framebuffer::destroy(l.framebuffer[iLayer]);
		DescriptorSet::destroy(l.descriptorSet[iLayer]);
	}
	DescriptorSet::destroy(l.renderDescriptorSet);
	Buffer::destroy(l.renderUBO);
	Texture::destroy(l.shadowMap);
}

void ShadowMapSystem::onCreate(aka::World& world)
{
	Application* app = Application::app();
	ProgramManager* program = app->program();
	GraphicDevice* device = app->graphic();

	FramebufferState fbDesc;
	fbDesc.colors[0].format = TextureFormat::Unknown;
	fbDesc.depth.format = TextureFormat::Depth;
	fbDesc.count = 0;

	// TODO do not need as many bindings
	VertexBindingState vertexBindings{};
	vertexBindings.attributes[0] = VertexAttribute{ VertexSemantic::Position, VertexFormat::Float, VertexType::Vec3 };
	vertexBindings.attributes[1] = VertexAttribute{ VertexSemantic::Normal, VertexFormat::Float, VertexType::Vec3 };
	vertexBindings.attributes[2] = VertexAttribute{ VertexSemantic::TexCoord0, VertexFormat::Float, VertexType::Vec2 };
	vertexBindings.attributes[3] = VertexAttribute{ VertexSemantic::Color0, VertexFormat::Float, VertexType::Vec4 };
	vertexBindings.count = 4;
	vertexBindings.offsets[0] = offsetof(Vertex, position);
	vertexBindings.offsets[1] = offsetof(Vertex, normal);
	vertexBindings.offsets[2] = offsetof(Vertex, texcoord);
	vertexBindings.offsets[3] = offsetof(Vertex, color);

	{
		m_shadowProgram = program->get("shadowDirectional");
		Rect rectDir = Rect{ 0, 0, DirectionalLightComponent::cascadeResolution, DirectionalLightComponent::cascadeResolution };
		ViewportState viewportDir{ rectDir, rectDir };
		Shader* shaders[2] = { m_shadowProgram->vertex, m_shadowProgram->fragment };
		m_shadowPipeline = device->createPipeline(
			shaders,
			2,
			PrimitiveType::Triangles,
			fbDesc,
			vertexBindings,
			m_shadowProgram->bindings,
			m_shadowProgram->setCount,
			viewportDir,
			DepthState{ DepthOp::Less, true },
			StencilState{}, // TODO None
			CullState{ CullMode::BackFace, CullOrder::CounterClockWise }, // TODO None
			BlendState{ BlendMode::One, BlendMode::One, BlendOp::Add, BlendMode::One, BlendMode::Zero, BlendOp::Add, BlendMask::Rgb, 0xff }, // TODO None
			FillState{ FillMode::Fill, 1.f } // TODO None
		);
	}

	{
		m_shadowPointProgram = program->get("shadowPoint");
		Rect rectPoint = Rect{ 0, 0, PointLightComponent::faceResolution, PointLightComponent::faceResolution };
		ViewportState viewportPoint{ rectPoint, rectPoint };
		Shader* shadersPoint[2] = { m_shadowPointProgram->vertex, m_shadowPointProgram->fragment };
		m_shadowPointPipeline = device->createPipeline(
			shadersPoint,
			2,
			PrimitiveType::Triangles,
			fbDesc,
			vertexBindings,
			m_shadowPointProgram->bindings,
			m_shadowPointProgram->setCount,
			viewportPoint,
			DepthState{ DepthOp::Less, true },
			StencilState{}, // TODO None
			CullState{ CullMode::BackFace, CullOrder::CounterClockWise }, // TODO None
			BlendState{ BlendMode::One, BlendMode::One, BlendOp::Add, BlendMode::One, BlendMode::Zero, BlendOp::Add, BlendMask::Rgb, 0xff }, // TODO None
			FillState{ FillMode::Fill, 1.f } // TODO None
		);
	}

	world.registry().on_construct<DirectionalLightComponent>().connect<&onDirectionalLightConstruct>();
	world.registry().on_destroy<DirectionalLightComponent>().connect<&onDirectionalLightDestroy>();
	world.registry().on_construct<PointLightComponent>().connect<&onPointLightConstruct>();
	world.registry().on_destroy<PointLightComponent>().connect<&onPointLightDestroy>();
}

void ShadowMapSystem::onDestroy(aka::World& world)
{
	Application* app = Application::app();
	GraphicDevice* device = app->graphic();

	world.registry().on_construct<DirectionalLightComponent>().disconnect<&onDirectionalLightConstruct>();
	world.registry().on_destroy<DirectionalLightComponent>().disconnect<&onDirectionalLightDestroy>();
	world.registry().on_construct<PointLightComponent>().disconnect<&onPointLightConstruct>();
	world.registry().on_destroy<PointLightComponent>().disconnect<&onPointLightDestroy>();

	device->destroy(m_shadowPipeline);
	device->destroy(m_shadowPointPipeline);
}

// Compute the shadow projection around the view projection.
mat4f computeShadowViewProjectionMatrix(const mat4f& view, const mat4f& projection, uint32_t resolution, const vec3f& lightDirWorld)
{
	mat4f clipToWorld = mat4f::inverse(projection * view);
	frustum<> f = frustum<>::fromInverseProjection(clipToWorld); // Frustum corners in world space

	point3f frustumCenter = f.center();
	// http://alextardif.com/shadowmapping.html
	float frustumRadius = f.radius();
	float texelsPerUnit = (float)resolution / (frustumRadius * 2.f);

	mat4f lookAt = mat4f::lookAtView(point3f(0.f), point3f(-lightDirWorld), norm3f(0, 1, 0));
	mat4f scalar = mat4f::scale(vec3f(texelsPerUnit));
	lookAt *= scalar;
	mat4f lookAtInverse = mat4f::inverse(lookAt);

	// Snap frustum center to texel grid
	point3f newFrustumCenter = lookAt * frustumCenter;
	newFrustumCenter.x = floor(newFrustumCenter.x);
	newFrustumCenter.y = floor(newFrustumCenter.y);
	newFrustumCenter = lookAtInverse * newFrustumCenter;

	vec3f offset = newFrustumCenter - frustumCenter; // old + offset = new
	//offset.norm(); // TODO take offset into account instead of * 2 ?
	
	// Get view matrix
	point3f eye = newFrustumCenter + (lightDirWorld * frustumRadius * 2.f);
	mat4f lightViewMatrix = mat4f::lookAtView(eye, newFrustumCenter, norm3f(0, 1, 0));

	// Get projection matrix
	float scale = 6.f; // scalar to improve depth so that we don't miss shadow of tall objects
	// TODO Ideally, compute the scale using some scene bounds data.
	mat4 lightProjectionMatrix = mat4f::orthographic(
		-frustumRadius, frustumRadius,
		-frustumRadius, frustumRadius,
		-frustumRadius * scale, frustumRadius * scale
	);
	return lightProjectionMatrix * lightViewMatrix;
}

void ShadowMapSystem::onRender(aka::World& world, aka::Frame* frame)
{
	GraphicDevice* device = Application::app()->graphic();

	Entity cameraEntity = Scene::getMainCamera(world);
	Camera3DComponent& camera = cameraEntity.get<Camera3DComponent>();
	mat4f view = camera.view;
	mat4f projection = camera.projection->projection();
	// TODO near far as ortho aswell
	CameraPerspective* perspective = dynamic_cast<CameraPerspective*>(camera.projection.get());
	AKA_ASSERT(perspective != nullptr, "Only support perspective camera for now.");

	CommandList* cmd = frame->commandList;

	// --- Shadow map system
	auto pointLightUpdate = world.registry().view<DirtyLightComponent, PointLightComponent>();
	auto dirLightUpdate = world.registry().view<DirtyLightComponent, DirectionalLightComponent>();
	
	cmd->bindPipeline(m_shadowPointPipeline);
	
	for (entt::entity e : pointLightUpdate)
	{
		Transform3DComponent& lightTransform = world.registry().get<Transform3DComponent>(e);
		PointLightComponent& light = world.registry().get<PointLightComponent>(e);

		// Generate shadow projection
		const point3f lightPos = point3f(lightTransform.transform.cols[3]);
		const mat4f faceView[6] = {
			mat4f::lookAtView(lightPos, lightPos + vec3f( 1.0,  0.0,  0.0), norm3f(0.0, 1.0,  0.0)), // POSITIVE_X
			mat4f::lookAtView(lightPos, lightPos + vec3f(-1.0,  0.0,  0.0), norm3f(0.0, 1.0,  0.0)), // NEGATIVE_X
			mat4f::lookAtView(lightPos, lightPos + vec3f( 0.0,  1.0,  0.0), norm3f(0.0, 0.0,  1.0)), // POSITIVE_Y
			mat4f::lookAtView(lightPos, lightPos + vec3f( 0.0, -1.0,  0.0), norm3f(0.0, 0.0, -1.0)), // NEGATIVE_Y
			mat4f::lookAtView(lightPos, lightPos + vec3f( 0.0,  0.0, -1.0), norm3f(0.0, 1.0,  0.0)), // POSITIVE_Z
			mat4f::lookAtView(lightPos, lightPos + vec3f( 0.0,  0.0,  1.0), norm3f(0.0, 1.0,  0.0)), // NEGATIVE_Z
		};
		const mat4f shadowProjection = mat4f::perspective(anglef::degree(90.f), 1.f, 0.1f, light.radius);
		for (uint32_t i = 0; i < 6; ++i)
		{
			// TODO do we need to store this ?
			light.worldToLightSpaceMatrix[i] = shadowProjection * faceView[i];
		}

		// Update light ubo
		PointLightUniformBuffer pointUBO;
		pointUBO.far = light.radius;
		pointUBO.lightPos = lightPos;
		for (uint32_t i = 0; i < 6; ++i)
		{
			pointUBO.lightView = light.worldToLightSpaceMatrix[i];
			device->upload(light.ubo[i], &pointUBO, 0, light.ubo[i]->size);
			AKA_ASSERT(light.ubo[i]->size == sizeof(PointLightUniformBuffer), "Invalid sizes");
		}

		auto view = world.registry().view<Transform3DComponent, MeshComponent, RenderComponent>();

		for (uint32_t iLayer = 0; iLayer < 6; ++iLayer) // for each faces
		{
			// Set output target and clear it.
			device->update(light.descriptorSet[iLayer]); // TODO only once
			cmd->beginRenderPass(light.framebuffer[iLayer], ClearState{ ClearMask::Depth, { 0.f }, 1.f, 0 });
			view.each([&](const Transform3DComponent& transform, const MeshComponent& mesh, const RenderComponent& render) {

				DescriptorSet* sets[2] = { light.descriptorSet[iLayer], render.matrices };
				cmd->bindIndexBuffer(mesh.mesh->indices, mesh.mesh->format, 0);
				cmd->bindVertexBuffer(mesh.mesh->vertices, 0, 1, mesh.mesh->bindings.offsets);
				cmd->bindDescriptorSets(sets, 2);

				cmd->drawIndexed(mesh.mesh->count, 0, 0, 1);
			});
			cmd->endRenderPass();
		}
		world.registry().remove<DirtyLightComponent>(e);
	}

	cmd->bindPipeline(m_shadowPipeline);

	for (entt::entity e : dirLightUpdate)
	{
		DirectionalLightComponent& light = world.registry().get<DirectionalLightComponent>(e);

		const float offset[DirectionalLightComponent::cascadeCount + 1] = { perspective->nearZ, perspective->farZ / 20.f, perspective->farZ / 5.f, perspective->farZ };
		// Generate shadow cascades
		for (size_t i = 0; i < DirectionalLightComponent::cascadeCount; i++)
		{
			float w = (float)Application::app()->width();
			float h = (float)Application::app()->height();
			float n = offset[i];
			float f = offset[i + 1];
			mat4f p = mat4f::perspective(perspective->hFov, w / h, n, f);
			light.worldToLightSpaceMatrix[i] = computeShadowViewProjectionMatrix(view, p, light.shadowMap->width, light.direction);
			vec4f clipSpace = projection * vec4f(0.f, 0.f, -offset[i + 1], 1.f);
			light.cascadeEndClipSpace[i] = clipSpace.z / clipSpace.w;
		}


		for (size_t i = 0; i < DirectionalLightComponent::cascadeCount; i++)
		{
			DirectionalLightUniformBuffer lightUBO{};
			lightUBO.light = light.worldToLightSpaceMatrix[i];
			device->upload(light.ubo[i], &lightUBO, 0, light.ubo[i]->size);
			device->update(light.descriptorSet[i]); // TODO only once

			cmd->beginRenderPass(light.framebuffer[i], ClearState{ ClearMask::Depth, { 0.f }, 1.f, 0 });

			auto view = world.registry().view<Transform3DComponent, MeshComponent, RenderComponent>();
			view.each([&](const Transform3DComponent& transform, const MeshComponent& mesh, const RenderComponent& render) {
				frustum<>::planes p = frustum<>::extract(light.worldToLightSpaceMatrix[i]);
				if (!p.intersect(transform.transform * mesh.bounds))
					return;

				DescriptorSet* descriptorSets[2] = { render.matrices, light.descriptorSet[i] };
				cmd->bindIndexBuffer(mesh.mesh->indices, mesh.mesh->format, 0);
				cmd->bindVertexBuffer(mesh.mesh->vertices, 0, 1, mesh.mesh->bindings.offsets);
				cmd->bindDescriptorSets(descriptorSets, 2);

				cmd->drawIndexed(mesh.mesh->count, 0, 0, 1);
			});
			cmd->endRenderPass();
		}
		world.registry().remove<DirtyLightComponent>(e);
	}
}

void ShadowMapSystem::onReceive(const ProgramReloadedEvent& e)
{
	// TODO fix this
	if (e.name == "shadowDirectional")
		m_shadowProgram = e.program;
	else if (e.name == "shadowPoint")
		m_shadowPointProgram = e.program;
}

};
