#include "ModelViewer.h"

#include <imgui.h>
#include <Aka/Layer/ImGuiLayer.h>

namespace viewer {

void Viewer::loadShader()
{
	std::vector<Attributes> attributes = { // HLSL only
		Attributes{ AttributeID(0), "POS" },
		Attributes{ AttributeID(0), "NORM" },
		Attributes{ AttributeID(0), "TEX" },
		Attributes{ AttributeID(0), "COL" }
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
			aka::Shader::Ptr shader = aka::Shader::create(vert, frag, aka::ShaderID(0), attributes);
			if (shader->valid())
				m_gbufferMaterial = aka::ShaderMaterial::create(shader);
		}
	}
	{
#if defined(AKA_USE_OPENGL)
		aka::ShaderID vert = aka::Shader::compile(File::readString(Asset::path("shaders/GL/shading.vert")), aka::ShaderType::Vertex);
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
}

void Viewer::onCreate()
{
	StopWatch<> stopWatch;
	// TODO use args
	//m_model = ModelLoader::load(Asset::path("glTF-Sample-Models/2.0/Sponza/glTF/Sponza.gltf"), point3f(0.f), 1.f);
	//m_model = ModelLoader::load(Asset::path("glTF-Sample-Models/2.0/AlphaBlendModeTest/glTF/AlphaBlendModeTest.gltf"));
	//m_model = ModelLoader::load(Asset::path("glTF-Sample-Models/2.0/CesiumMilkTruck/glTF/CesiumMilkTruck.gltf"));
	m_model = ModelLoader::load(Asset::path("glTF-Sample-Models/2.0/Lantern/glTF/Lantern.gltf"), point3f(0.f), 1.f);
	if (m_model == nullptr)
		throw std::runtime_error("Could not load model.");
	aka::Logger::info("Model loaded : ", stopWatch.elapsed(), "ms");
	aka::Logger::info("Scene Bounding box : ", m_model->bbox.min, " - ", m_model->bbox.max);

	loadShader();

	// --- G-Buffer pass
	Sampler gbufferSampler{};
	gbufferSampler.filterMag = Sampler::Filter::Linear;
	gbufferSampler.filterMin = Sampler::Filter::Linear;
	gbufferSampler.wrapU = Sampler::Wrap::ClampToEdge;
	gbufferSampler.wrapV = Sampler::Wrap::ClampToEdge;
	gbufferSampler.wrapW = Sampler::Wrap::ClampToEdge;
	m_depth = Texture::create2D(width(), height(), TextureFormat::UnsignedInt248, TextureComponent::Depth24Stencil8, TextureFlag::RenderTarget, gbufferSampler);
	m_position = Texture::create2D(width(), height(), TextureFormat::Float, TextureComponent::RGBA16F, TextureFlag::RenderTarget, gbufferSampler);
	m_albedo = Texture::create2D(width(), height(), TextureFormat::UnsignedByte, TextureComponent::RGBA, TextureFlag::RenderTarget, gbufferSampler);
	m_normal = Texture::create2D(width(), height(), TextureFormat::Float, TextureComponent::RGBA16F, TextureFlag::RenderTarget, gbufferSampler);
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
		}
	};
	m_gbuffer = Framebuffer::create(width(), height(), gbufferAttachments, sizeof(gbufferAttachments) / sizeof(FramebufferAttachment));

	// --- Lighting pass
	m_quad = Mesh::create();
	VertexData data;
	data.attributes.push_back(VertexData::Attribute{ 0, VertexFormat::Float, VertexType::Vec2 });
	float vertices[] = {
		-1, -1, // bottom left corner
		 1, -1,  // bottom right corner
		 1,  1, // top right corner
		-1,  1, // top left corner
	};
	uint8_t indices[] = { 0,1,2,0,2,3 };
	m_quad->vertices(data, vertices, 4);
	m_quad->indices(IndexFormat::UnsignedByte, indices, 6);

	// --- Forward pass 

	// --- Shadows
	uint32_t shadowMapWidth = 2048;
	uint32_t shadowMapHeight = 2048;
	Sampler shadowSampler{};
	shadowSampler.filterMag = Sampler::Filter::Linear;
	shadowSampler.filterMin = Sampler::Filter::Linear;
	shadowSampler.wrapU = Sampler::Wrap::ClampToEdge;
	shadowSampler.wrapV = Sampler::Wrap::ClampToEdge;
	shadowSampler.wrapW = Sampler::Wrap::ClampToEdge;
	// TODO texture atlas & single shader execution
	m_shadowCascadeTexture[0] = Texture::create2D(shadowMapWidth, shadowMapHeight, TextureFormat::Float, TextureComponent::Depth, TextureFlag::RenderTarget, shadowSampler);
	m_shadowCascadeTexture[1] = Texture::create2D(shadowMapWidth, shadowMapHeight, TextureFormat::Float, TextureComponent::Depth, TextureFlag::RenderTarget, shadowSampler);
	m_shadowCascadeTexture[2] = Texture::create2D(shadowMapWidth, shadowMapHeight, TextureFormat::Float, TextureComponent::Depth, TextureFlag::RenderTarget, shadowSampler);
	FramebufferAttachment shadowAttachments[] = {
		FramebufferAttachment{
			FramebufferAttachmentType::Depth,
			m_shadowCascadeTexture[0]
		}
	}; 
	m_shadowFramebuffer = Framebuffer::create(shadowMapWidth, shadowMapHeight, shadowAttachments, sizeof(shadowAttachments) / sizeof(FramebufferAttachment));

	m_lightDir = vec3f(0.1f, 1.f, 0.1f);

	m_camera.set(m_model->bbox);
	attach<ImGuiLayer>();
}

void Viewer::onDestroy()
{
}

void Viewer::onUpdate(aka::Time::Unit deltaTime)
{
	// Arcball
	m_camera.update(deltaTime);

	// TOD
	if (Mouse::pressed(MouseButton::ButtonMiddle))
	{
		const Position& pos = Mouse::position();
		float x = pos.x / (float)GraphicBackend::backbuffer()->width();
		m_lightDir = vec3f::normalize(lerp(vec3f(1, 1, 1), vec3f(-1, 1, -1), x));
	}

	// Reset
	if (Keyboard::down(KeyboardKey::R))
	{
		m_camera.set(m_model->bbox);
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
}

// --- Shadows
// Compute the shadow projection around the view projection.
mat4f computeShadowViewProjectionMatrix(const mat4f& view, const mat4f& projection, float near, float far, const vec3f& lightDirWorld)
{
	frustum<> f = frustum<>::fromProjection(projection * view);
	point3f centerWorld = f.center();
	// http://alextardif.com/shadowmapping.html
	// We need to snap position to avoid flickering
	centerWorld = point3f(
		floor(centerWorld.x),
		floor(centerWorld.y),
		floor(centerWorld.z)
	);
	// TODO handle up.
	mat4f lightViewMatrix = mat4f::inverse(mat4f::lookAt(centerWorld, centerWorld - lightDirWorld, norm3f(0, 1, 0)));
	// Set to light space.
	aabbox bbox;
	for (size_t i = 0; i < 8; i++)
		bbox.include(lightViewMatrix.multiplyPoint(f.corners[i]));
	// snap bbox to upper bounds
	// TODO scale depend on scene bbox vs frustum bbox
	float scale = 6.f; // scalar to improve depth so that we don't miss shadow of tall objects
	mat4f lightProjectionMatrix = mat4f::orthographic(
		floor(bbox.min.y), ceil(bbox.max.y),
		floor(bbox.min.x), ceil(bbox.max.x),
		floor(bbox.min.z) * scale, ceil(bbox.max.z) * scale
	);
	return lightProjectionMatrix * lightViewMatrix;
}

void Viewer::onRender()
{
	Framebuffer::Ptr backbuffer = GraphicBackend::backbuffer();
	mat4f debugView = mat4f::inverse(mat4f::lookAt(m_model->bbox.center() + m_model->bbox.extent(), point3f(0.f)));
	mat4f view = mat4f::inverse(m_camera.transform());
	// TODO use camera (Arcball inherit camera ?)
	const float near = 0.01f;
	const float far = 100.f;
	const anglef angle = anglef::degree(90.f);
	mat4f debugPerspective = mat4f::perspective(angle, (float)backbuffer->width() / (float)backbuffer->height(), 0.01f, 1000.f);
	mat4f perspective = mat4f::perspective(angle, (float)backbuffer->width() / (float)backbuffer->height(), near, far);

	// Generate shadow cascades
	mat4f worldToLightSpaceMatrix[3];
	const size_t cascadeCount = 3;
	const float offset[cascadeCount + 1] = { near, far / 20.f, far / 5.f, far };
	float cascadeEndClipSpace[cascadeCount];
	for (size_t i = 0; i < cascadeCount; i++)
	{
		float w = (float)width();
		float h = (float)height();
		float n = offset[i];
		float f = offset[i + 1];
		mat4f p = mat4f::perspective(angle, w / h, n, f);
		worldToLightSpaceMatrix[i] = computeShadowViewProjectionMatrix(view, p, n, f, m_lightDir);
		vec4f clipSpace = perspective * vec4f(0.f, 0.f, -offset[i + 1], 1.f);
		cascadeEndClipSpace[i] = clipSpace.z / clipSpace.w;
	}
	mat4f projectionToTextureCoordinateMatrix(
		col4f(0.5, 0.0, 0.0, 0.0),
		col4f(0.0, 0.5, 0.0, 0.0),
		col4f(0.0, 0.0, 0.5, 0.0),
		col4f(0.5, 0.5, 0.5, 1.0)
	);
	mat4f worldToLightTextureSpaceMatrix[cascadeCount];
	for (size_t i = 0; i < cascadeCount; i++)
		worldToLightTextureSpaceMatrix[i] = projectionToTextureCoordinateMatrix * worldToLightSpaceMatrix[i];

	// --- Shadow pass
	RenderPass shadowPass;
	shadowPass.framebuffer = m_shadowFramebuffer;
	shadowPass.primitive = PrimitiveType::Triangles;
	shadowPass.indexOffset = 0;
	shadowPass.material = m_shadowMaterial;
	shadowPass.clear = Clear{ ClearMask::None, color4f(1.f), 1.f, 0 };
	shadowPass.blend = Blending::none();
	shadowPass.depth = Depth{ DepthCompare::LessOrEqual, true };
	shadowPass.stencil = Stencil::none();
	shadowPass.viewport = aka::Rect{ 0 };
	shadowPass.scissor = aka::Rect{ 0 };
	for (size_t i = 0; i < cascadeCount; i++)
	{
		m_shadowFramebuffer->attachment(FramebufferAttachmentType::Depth, m_shadowCascadeTexture[i]);
		m_shadowFramebuffer->clear(color4f(1.f), 1.f, 0, ClearMask::Depth);
		m_shadowMaterial->set<mat4f>("u_light", worldToLightSpaceMatrix[i]);
		for (size_t i = 0; i < m_model->nodes.size(); i++)
		{
			aka::mat4f model = m_model->nodes[i].transform;
			m_shadowMaterial->set<mat4f>("u_model", model);
			shadowPass.mesh = m_model->nodes[i].mesh;
			shadowPass.indexCount = m_model->nodes[i].mesh->getIndexCount(); // TODO set zero means all ?
			shadowPass.cull = Culling{ CullMode::BackFace, CullOrder::CounterClockWise };

			shadowPass.execute();
		}
	}

	mat4f renderView = view;
	mat4f renderPerspective = perspective;

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
		gbufferPass.primitive = PrimitiveType::Triangles;
		gbufferPass.indexOffset = 0;
		gbufferPass.material = m_gbufferMaterial;
		gbufferPass.clear = Clear{ ClearMask::None, color4f(1.f), 1.f, 0 };
		gbufferPass.blend = Blending::none();
		gbufferPass.depth = Depth{ DepthCompare::LessOrEqual, true };
		gbufferPass.stencil = Stencil::none();
		gbufferPass.viewport = aka::Rect{ 0 };
		gbufferPass.scissor = aka::Rect{ 0 };

		m_gbufferMaterial->set<mat4f>("u_view", renderView);
		m_gbufferMaterial->set<mat4f>("u_projection", renderPerspective);

		m_gbuffer->clear(color4f(0.f), 1.f, 0, ClearMask::All);

		for (size_t i = 0; i < m_model->nodes.size(); i++)
		{
			aka::mat4f model = m_model->nodes[i].transform;
			aka::mat3f normal = aka::mat3f::transpose(aka::mat3f::inverse(mat3f(model)));
			aka::color4f color = m_model->nodes[i].material.color;
			m_gbufferMaterial->set<mat4f>("u_model", model);
			m_gbufferMaterial->set<mat3f>("u_normalMatrix", normal);
			m_gbufferMaterial->set<color4f>("u_color", color);
			m_gbufferMaterial->set<Texture::Ptr>("u_colorTexture", m_model->nodes[i].material.colorTexture);
			m_gbufferMaterial->set<Texture::Ptr>("u_normalTexture", m_model->nodes[i].material.normalTexture);

			m_gbufferMaterial->set<mat4f>("u_model", model);
			gbufferPass.mesh = m_model->nodes[i].mesh;
			gbufferPass.indexCount = m_model->nodes[i].mesh->getIndexCount(); // TODO set zero means all ?
			gbufferPass.cull = Culling{ CullMode::BackFace, CullOrder::CounterClockWise };

			gbufferPass.execute();
		}

		// --- Lighting pass
		backbuffer->blit(m_gbuffer, FramebufferAttachmentType::DepthStencil, Sampler::Filter::Nearest);
		// TODO We can use compute here
		RenderPass lightingPass;
		lightingPass.framebuffer = backbuffer;
		lightingPass.primitive = PrimitiveType::Triangles;
		lightingPass.indexOffset = 0;
		lightingPass.material = m_lightingMaterial;
		lightingPass.clear = Clear{ ClearMask::All, color4f(0.f), 1.f, 0 };
		lightingPass.blend = Blending::none();
		lightingPass.depth = Depth{ DepthCompare::None, true };
		lightingPass.stencil = Stencil::none();
		lightingPass.viewport = aka::Rect{ 0 };
		lightingPass.scissor = aka::Rect{ 0 };
		lightingPass.mesh = m_quad;
		lightingPass.indexCount = m_quad->getIndexCount(); // TODO set zero means all ?
		lightingPass.cull = Culling{ CullMode::BackFace, CullOrder::CounterClockWise };

		m_lightingMaterial->set<Texture::Ptr>("u_position", m_position);
		m_lightingMaterial->set<Texture::Ptr>("u_albedo", m_albedo);
		m_lightingMaterial->set<Texture::Ptr>("u_normal", m_normal);
		m_lightingMaterial->set<Texture::Ptr>("u_depth", m_depth);
		m_lightingMaterial->set<Texture::Ptr>("u_shadowTexture[0]", m_shadowCascadeTexture, cascadeCount);
		m_lightingMaterial->set<mat4f>("u_worldToLightTextureSpace[0]", worldToLightTextureSpaceMatrix, cascadeCount);
		m_lightingMaterial->set<vec3f>("u_lightDir", m_lightDir);
		m_lightingMaterial->set<float>("u_cascadeEndClipSpace[0]", cascadeEndClipSpace, cascadeCount);

		lightingPass.execute();

	}
	else
	{
		// --- Forward pass
		backbuffer->clear(color4f(0.f, 0.f, 0.f, 1.f), 1.f, 0, ClearMask::All);

		if (Keyboard::pressed(KeyboardKey::ControlLeft))
		{
			renderView = debugView;
			renderPerspective = debugPerspective;
		}
		m_material->set<mat4f>("u_view", renderView);
		m_material->set<mat4f>("u_projection", renderPerspective);
		m_material->set<mat4f>("u_light[0]", worldToLightTextureSpaceMatrix, cascadeCount);
		m_material->set<vec3f>("u_lightDir", m_lightDir);
		m_material->set<Texture::Ptr>("u_shadowTexture[0]", m_shadowCascadeTexture, cascadeCount);
		m_material->set<float>("u_cascadeEndClipSpace[0]", cascadeEndClipSpace, cascadeCount);
		RenderPass renderPass{};
		renderPass.framebuffer = backbuffer;
		renderPass.primitive = PrimitiveType::Triangles;
		renderPass.indexOffset = 0;
		renderPass.material = m_material;
		renderPass.clear = Clear{ ClearMask::None, color4f(1.f), 1.f, 0 };
		renderPass.blend = Blending::nonPremultiplied();
		renderPass.depth = Depth{ DepthCompare::LessOrEqual, true };
		renderPass.stencil = Stencil::none();
		renderPass.viewport = aka::Rect{ 0 };
		renderPass.scissor = aka::Rect{ 0 };
		for (size_t i = 0; i < m_model->nodes.size(); i++)
		{
			aka::mat4f model = m_model->nodes[i].transform;
			aka::mat3f normal = aka::mat3f::transpose(aka::mat3f::inverse(mat3f(model)));
			aka::color4f color = m_model->nodes[i].material.color;
			m_material->set<mat4f>("u_model", model);
			m_material->set<mat3f>("u_normalMatrix", normal);
			m_material->set<color4f>("u_color", color);
			m_material->set<Texture::Ptr>("u_colorTexture", m_model->nodes[i].material.colorTexture);
			m_material->set<Texture::Ptr>("u_normalTexture", m_model->nodes[i].material.normalTexture);
			renderPass.mesh = m_model->nodes[i].mesh;
			renderPass.indexCount = m_model->nodes[i].mesh->getIndexCount(); // TODO set zero means all ?
			renderPass.cull = m_model->nodes[i].material.doubleSided ? Culling{ CullMode::None, CullOrder::CounterClockWise } : Culling{ CullMode::BackFace, CullOrder::CounterClockWise };

			renderPass.execute();
		}
	}

	// --- Debug pass
	// TODO add pipeline (default pipeline, debug pipeline...)
	// Debug pipeline : create origin mesh. for every mesh in scene, draw bbox & origin as line.
	vec2f windowSize = vec2f((float)backbuffer->width(), (float)backbuffer->height());
	vec2f pos[3];
	vec2f size = windowSize / 8.f;
	for (size_t i = 0; i < cascadeCount; i++)
		pos[i] = vec2f(size.x * i + 10.f * (i + 1), windowSize.y - size.y - 10.f);
	{
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
		Renderer3D::drawAxis(mat4f::translate(vec3f(m_model->bbox.center())));
		if (hovered < 0)
		{
			Renderer3D::drawFrustum(perspective * view);
		}
		else
		{
			Renderer3D::drawFrustum(mat4f::perspective(angle, (float)backbuffer->width() / (float)backbuffer->height(), offset[hovered], offset[hovered + 1]) * view);
			Renderer3D::drawFrustum(worldToLightSpaceMatrix[hovered]);
		}
		Renderer3D::drawTransform(mat4f::translate(vec3f(m_model->bbox.center())) * mat4f::scale(m_model->bbox.extent() / 2.f));
		Renderer3D::render(GraphicBackend::backbuffer(), renderView, renderPerspective);
		Renderer3D::clear();
	}
	{
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

	/* {
		if (ImGui::Begin("Info"))
		{
			ImGuiIO& io = ImGui::GetIO();
			ImGui::Text("Resolution : %ux%u", width(), height());
			ImGui::Text("%.3f ms/frame", 1000.0f / ImGui::GetIO().Framerate);
			ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
		}
		ImGui::End();

		if (ImGui::Begin("Scene"))
		{
			m_model->bbox;
			uint32_t id = 0;
			uint32_t vertexCount = 0;
			uint32_t indexCount = 0;
			for (Node& node : m_model->nodes)
			{
				char buffer[256];
				vertexCount += node.mesh->getVertexCount();
				indexCount += node.mesh->getIndexCount();
				snprintf(buffer, 256, "Node %u", id++);
				if (ImGui::TreeNode(buffer))
				{
					// TODO use ImGuiGizmo
					ImGui::Text("Transform");
					ImGui::InputFloat4("##col0", node.transform.cols[0].data);
					ImGui::InputFloat4("##col1", node.transform.cols[1].data);
					ImGui::InputFloat4("##col2", node.transform.cols[2].data);
					ImGui::InputFloat4("##col3", node.transform.cols[3].data);
					ImGui::Text("Mesh");
					ImGui::Text("Vertices : %d", node.mesh->getVertexCount());
					ImGui::Text("Indices : %d", node.mesh->getIndexCount());
					node.material;

					ImGui::TreePop();
				}
			}
			ImGui::Text("Vertices : %d", vertexCount);
			ImGui::Text("Indices : %d", indexCount);
		}
		ImGui::End();
	}*/
}

};
