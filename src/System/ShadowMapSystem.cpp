#include "ShadowMapSystem.h"

#include "../Model/Model.h"

namespace viewer {

using namespace aka;

void ShadowMapSystem::onCreate(aka::World& world)
{
	createShaders();
	Framebuffer::Ptr backbuffer = GraphicBackend::backbuffer();
	Texture::Ptr dummyDepth = Texture::create2D(1, 1, TextureFormat::Depth, TextureFlag::RenderTarget, Sampler{});
	FramebufferAttachment shadowAttachments[] = {
		FramebufferAttachment{
			FramebufferAttachmentType::Depth,
			dummyDepth
		}
	};
	m_shadowFramebuffer = Framebuffer::create(shadowAttachments, 1);
}

void ShadowMapSystem::onDestroy(aka::World& world)
{
}

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

void ShadowMapSystem::onRender(aka::World& world)
{
	Framebuffer::Ptr backbuffer = GraphicBackend::backbuffer();

	Entity cameraEntity = Scene::getMainCamera(world);
	Camera3DComponent& camera = cameraEntity.get<Camera3DComponent>();
	mat4f view = camera.view;
	mat4f projection = camera.projection->projection();
	// TODO near far as ortho aswell
	CameraPerspective* perspective = dynamic_cast<CameraPerspective*>(camera.projection);
	AKA_ASSERT(perspective != nullptr, "Only support perspective camera for now.");

	// --- Shadow map system
	auto pointLightUpdate = world.registry().view<DirtyLightComponent, PointLightComponent>();
	auto dirLightUpdate = world.registry().view<DirtyLightComponent, DirectionalLightComponent>();
	for (entt::entity e : pointLightUpdate)
	{
		Transform3DComponent& transform = world.registry().get<Transform3DComponent>(e);
		PointLightComponent& light = world.registry().get<PointLightComponent>(e);

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
		m_shadowPointMaterial->set<mat4f>("u_lights", light.worldToLightSpaceMatrix, 6);
		m_shadowPointMaterial->set<vec3f>("u_lightPos", vec3f(transform.transform.cols[3]));
		m_shadowPointMaterial->set<float>("u_far", light.radius);
		auto view = world.registry().view<Transform3DComponent, MeshComponent>();
		view.each([&](const Transform3DComponent& transform, const MeshComponent& mesh) {
			m_shadowPointMaterial->set<mat4f>("u_model", transform.transform);
			shadowPass.submesh = mesh.submesh;
			shadowPass.execute();
			});
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
			light.worldToLightSpaceMatrix[i] = computeShadowViewProjectionMatrix(view, p, light.shadowMap[i]->width(), n, f, light.direction);
			vec4f clipSpace = projection * vec4f(0.f, 0.f, -offset[i + 1], 1.f);
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
			auto view = world.registry().view<Transform3DComponent, MeshComponent>();
			view.each([&](const Transform3DComponent& transform, const MeshComponent& mesh) {
				frustum<>::planes p = frustum<>::extract(light.worldToLightSpaceMatrix[i]);
				if (!p.intersect(transform.transform * mesh.bounds))
					return;
				m_shadowMaterial->set<mat4f>("u_model", transform.transform);
				shadowPass.submesh = mesh.submesh;
				shadowPass.execute();
				});
		}
		world.registry().remove<DirtyLightComponent>(e);
	}
}

void ShadowMapSystem::onReceive(const ShaderHotReloadEvent& e)
{
	createShaders();
}

void ShadowMapSystem::createShaders()
{
	std::vector<VertexAttribute> defaultAttributes = {
		   VertexAttribute{ VertexSemantic::Position, VertexFormat::Float, VertexType::Vec3 },
		   VertexAttribute{ VertexSemantic::Normal, VertexFormat::Float, VertexType::Vec3 },
		   VertexAttribute{ VertexSemantic::TexCoord0, VertexFormat::Float, VertexType::Vec2 },
		   VertexAttribute{ VertexSemantic::Color0, VertexFormat::Float, VertexType::Vec4 }
	};
	{
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
			aka::Shader::Ptr shader = aka::Shader::create(vert, frag, defaultAttributes.data(), defaultAttributes.size());
			if (shader->valid())
				m_shadowMaterial = aka::ShaderMaterial::create(shader);
		}
	}
	{
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
			aka::Shader::Ptr shader = aka::Shader::createGeometry(vert, frag, geo, defaultAttributes.data(), defaultAttributes.size());
			if (shader->valid())
				m_shadowPointMaterial = aka::ShaderMaterial::create(shader);
		}
	}
}

};