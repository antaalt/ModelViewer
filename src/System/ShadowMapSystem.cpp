#include "ShadowMapSystem.h"

#include "../Model/Model.h"

namespace app {

using namespace aka;

struct alignas(16) LightModelUniformBuffer {
	alignas(16) mat4f model;
};

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
	l.shadowMap[0] = Texture2D::create(2048, 2048, TextureFormat::Depth, TextureFlag::RenderTarget | TextureFlag::ShaderResource);
	l.shadowMap[1] = Texture2D::create(2048, 2048, TextureFormat::Depth, TextureFlag::RenderTarget | TextureFlag::ShaderResource);
	l.shadowMap[2] = Texture2D::create(4096, 4096, TextureFormat::Depth, TextureFlag::RenderTarget | TextureFlag::ShaderResource);
	if (!registry.has<DirtyLightComponent>(entity))
		registry.emplace<DirtyLightComponent>(entity);
}

void onDirectionalLightDestroy(entt::registry& registry, entt::entity entity)
{
	// nothing to do here
}

void onPointLightConstruct(entt::registry& registry, entt::entity entity)
{
	PointLightComponent& l = registry.get<PointLightComponent>(entity);
	l.shadowMap = TextureCubeMap::create(1024, 1024, TextureFormat::Depth, TextureFlag::RenderTarget | TextureFlag::ShaderResource);
	if (!registry.has<DirtyLightComponent>(entity))
		registry.emplace<DirtyLightComponent>(entity);
}

void onPointLightDestroy(entt::registry& registry, entt::entity entity)
{
	// nothing to do here
}

void ShadowMapSystem::onCreate(aka::World& world)
{
	ProgramManager* program = Application::program();
	m_shadowMaterial = Material::create(program->get("shadowDirectional"));
	m_shadowPointMaterial = Material::create(program->get("shadowPoint"));

	GraphicDevice* device = Application::graphic();
	Backbuffer::Ptr backbuffer = device->backbuffer();
	Texture::Ptr dummyDepth = Texture2D::create(1, 1, TextureFormat::Depth, TextureFlag::RenderTarget);
	Attachment shadowAttachments[] = {
		Attachment{ AttachmentType::Depth, dummyDepth, AttachmentFlag::None, 0, 0 }
	};
	m_shadowFramebuffer = Framebuffer::create(shadowAttachments, 1);
	m_modelUniformBuffer = Buffer::create(BufferType::Uniform, sizeof(LightModelUniformBuffer), BufferUsage::Default, BufferCPUAccess::None);
	m_pointLightUniformBuffer = Buffer::create(BufferType::Uniform, sizeof(PointLightUniformBuffer), BufferUsage::Default, BufferCPUAccess::None);
	m_directionalLightUniformBuffer = Buffer::create(BufferType::Uniform, sizeof(DirectionalLightUniformBuffer), BufferUsage::Default, BufferCPUAccess::None);

	world.registry().on_construct<DirectionalLightComponent>().connect<&onDirectionalLightConstruct>();
	world.registry().on_destroy<DirectionalLightComponent>().connect<&onDirectionalLightDestroy>();
	world.registry().on_construct<PointLightComponent>().connect<&onPointLightConstruct>();
	world.registry().on_destroy<PointLightComponent>().connect<&onPointLightDestroy>();
}

void ShadowMapSystem::onDestroy(aka::World& world)
{
	world.registry().on_construct<DirectionalLightComponent>().disconnect<&onDirectionalLightConstruct>();
	world.registry().on_destroy<DirectionalLightComponent>().disconnect<&onDirectionalLightDestroy>();
	world.registry().on_construct<PointLightComponent>().disconnect<&onPointLightConstruct>();
	world.registry().on_destroy<PointLightComponent>().disconnect<&onPointLightDestroy>();
}

// Compute the shadow projection around the view projection.
mat4f computeShadowViewProjectionMatrix(const mat4f& view, const mat4f& projection, uint32_t resolution, const vec3f& lightDirWorld)
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

	mat4f lookAt = mat4f::lookAtView(point3f(0.f), point3f(-lightDirWorld), norm3f(0, 1, 0));
	mat4f scalar = mat4f::scale(vec3f(texelsPerUnit));
	lookAt *= scalar;
	mat4f lookAtInverse = mat4f::inverse(lookAt);

	centerWorld = lookAt * centerWorld;
	centerWorld.x = floor(centerWorld.x);
	centerWorld.y = floor(centerWorld.y);
	centerWorld = lookAtInverse * centerWorld;

	point3f eye = centerWorld + (lightDirWorld * radius * 2.f);

	mat4f lightViewMatrix = mat4f::lookAtView(eye, centerWorld, norm3f(0, 1, 0));

	float scale = 6.f; // scalar to improve depth so that we don't miss shadow of tall objects
	mat4 lightProjectionMatrix = mat4f::orthographic(
		-radius, radius,
		-radius, radius,
		-radius * scale, radius * scale
	);
	return lightProjectionMatrix * lightViewMatrix;
}

void ShadowMapSystem::onRender(aka::World& world)
{
	GraphicDevice* device = Application::graphic();
	Backbuffer::Ptr backbuffer = device->backbuffer();

	Entity cameraEntity = Scene::getMainCamera(world);
	Camera3DComponent& camera = cameraEntity.get<Camera3DComponent>();
	mat4f view = camera.view;
	mat4f projection = camera.projection->projection();
	// TODO near far as ortho aswell
	CameraPerspective* perspective = dynamic_cast<CameraPerspective*>(camera.projection.get());
	AKA_ASSERT(perspective != nullptr, "Only support perspective camera for now.");

	m_shadowPointMaterial->set("LightModelUniformBuffer", m_modelUniformBuffer);
	m_shadowPointMaterial->set("PointLightUniformBuffer", m_pointLightUniformBuffer);
	m_shadowMaterial->set("LightModelUniformBuffer", m_modelUniformBuffer);
	m_shadowMaterial->set("DirectionalLightUniformBuffer", m_directionalLightUniformBuffer);

	// --- Shadow map system
	auto pointLightUpdate = world.registry().view<DirtyLightComponent, PointLightComponent>();
	auto dirLightUpdate = world.registry().view<DirtyLightComponent, DirectionalLightComponent>();
	for (entt::entity e : pointLightUpdate)
	{
		Transform3DComponent& lightTransform = world.registry().get<Transform3DComponent>(e);
		PointLightComponent& light = world.registry().get<PointLightComponent>(e);

		// Generate shadow cascades
		mat4f shadowProjection = mat4f::perspective(anglef::degree(90.f), 1.f, 0.1f, light.radius);
		point3f lightPos = point3f(lightTransform.transform.cols[3]);
		light.worldToLightSpaceMatrix[0] = shadowProjection * mat4f::lookAtView(lightPos, lightPos + vec3f(1.0, 0.0, 0.0), norm3f(0.0, -1.0, 0.0));
		light.worldToLightSpaceMatrix[1] = shadowProjection * mat4f::lookAtView(lightPos, lightPos + vec3f(-1.0, 0.0, 0.0), norm3f(0.0, -1.0, 0.0));
		light.worldToLightSpaceMatrix[2] = shadowProjection * mat4f::lookAtView(lightPos, lightPos + vec3f(0.0, 1.0, 0.0), norm3f(0.0, 0.0, 1.0));
		light.worldToLightSpaceMatrix[3] = shadowProjection * mat4f::lookAtView(lightPos, lightPos + vec3f(0.0, -1.0, 0.0), norm3f(0.0, 0.0, -1.0));
		light.worldToLightSpaceMatrix[4] = shadowProjection * mat4f::lookAtView(lightPos, lightPos + vec3f(0.0, 0.0, 1.0), norm3f(0.0, -1.0, 0.0));
		light.worldToLightSpaceMatrix[5] = shadowProjection * mat4f::lookAtView(lightPos, lightPos + vec3f(0.0, 0.0, -1.0), norm3f(0.0, -1.0, 0.0));

		RenderPass shadowPass;
		shadowPass.framebuffer = m_shadowFramebuffer;
		shadowPass.material = m_shadowPointMaterial;
		shadowPass.clear = Clear::none;
		shadowPass.blend = Blending::none;
		shadowPass.depth = Depth{ DepthCompare::Less, true };
		shadowPass.cull = Culling{ CullMode::BackFace, CullOrder::CounterClockWise };
		shadowPass.stencil = Stencil::none;
		shadowPass.viewport = aka::Rect{ 0 };
		shadowPass.scissor = aka::Rect{ 0 };

		// Update light ubo
		PointLightUniformBuffer pointUBO;
		pointUBO.far = light.radius;
		pointUBO.lightPos = lightPos;

		LightModelUniformBuffer modelUBO;
		auto view = world.registry().view<Transform3DComponent, MeshComponent>();
		for (int i = 0; i < 6; ++i)
		{
			pointUBO.lightView = light.worldToLightSpaceMatrix[i];
			m_pointLightUniformBuffer->upload(&pointUBO);
			// Set output target and clear it.
			shadowPass.framebuffer->set(AttachmentType::Depth, light.shadowMap, AttachmentFlag::None, i);
			m_shadowFramebuffer->clear(color4f(1.f), 1.f, 0, ClearMask::Depth);
			view.each([&](const Transform3DComponent& transform, const MeshComponent& mesh) {
				modelUBO.model = transform.transform;
				m_modelUniformBuffer->upload(&modelUBO);
				shadowPass.submesh = mesh.submesh;
				shadowPass.execute();
			});
		}
		world.registry().remove<DirtyLightComponent>(e);
	}

	for (entt::entity e : dirLightUpdate)
	{
		DirectionalLightComponent& light = world.registry().get<DirectionalLightComponent>(e);

		const float offset[DirectionalLightComponent::cascadeCount + 1] = { perspective->nearZ, perspective->farZ / 20.f, perspective->farZ / 5.f, perspective->farZ };
		// Generate shadow cascades
		for (size_t i = 0; i < DirectionalLightComponent::cascadeCount; i++)
		{
			float w = (float)backbuffer->width();
			float h = (float)backbuffer->height();
			float n = offset[i];
			float f = offset[i + 1];
			mat4f p = mat4f::perspective(perspective->hFov, w / h, n, f);
			light.worldToLightSpaceMatrix[i] = computeShadowViewProjectionMatrix(view, p, light.shadowMap[i]->width(), light.direction);
			vec4f clipSpace = projection * vec4f(0.f, 0.f, -offset[i + 1], 1.f);
			light.cascadeEndClipSpace[i] = clipSpace.z / clipSpace.w;
		}

		RenderPass shadowPass;
		shadowPass.framebuffer = m_shadowFramebuffer;
		shadowPass.material = m_shadowMaterial;
		shadowPass.clear = Clear::none;
		shadowPass.blend = Blending::none;
		shadowPass.depth = Depth{ DepthCompare::Less, true };
		shadowPass.cull = Culling{ CullMode::BackFace, CullOrder::CounterClockWise };
		shadowPass.stencil = Stencil::none;
		shadowPass.viewport = aka::Rect{ 0 };
		shadowPass.scissor = aka::Rect{ 0 };
		for (size_t i = 0; i < DirectionalLightComponent::cascadeCount; i++)
		{
			m_shadowFramebuffer->set(AttachmentType::Depth, light.shadowMap[i]);
			m_shadowFramebuffer->clear(color4f(1.f), 1.f, 0, ClearMask::Depth);
			DirectionalLightUniformBuffer lightUBO;
			lightUBO.light = light.worldToLightSpaceMatrix[i];
			m_directionalLightUniformBuffer->upload(&lightUBO);

			LightModelUniformBuffer modelUBO;
			auto view = world.registry().view<Transform3DComponent, MeshComponent>();
			view.each([&](const Transform3DComponent& transform, const MeshComponent& mesh) {
				frustum<>::planes p = frustum<>::extract(light.worldToLightSpaceMatrix[i]);
				if (!p.intersect(transform.transform * mesh.bounds))
					return;
				modelUBO.model = transform.transform;
				m_modelUniformBuffer->upload(&modelUBO);
				shadowPass.submesh = mesh.submesh;
				shadowPass.execute();
			});
		}
		world.registry().remove<DirtyLightComponent>(e);
	}
}

void ShadowMapSystem::onReceive(const ProgramReloadedEvent& e)
{
	if (e.name == "shadowDirectional")
		m_shadowMaterial = Material::create(e.program);
	else if (e.name == "shadowPoint")
		m_shadowPointMaterial = Material::create(e.program);
}

};
