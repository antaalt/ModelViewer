#include "ModelViewer.h"

#include <imgui.h>
#include <Aka/Layer/ImGuiLayer.h>

namespace viewer {

void Viewer::loadShader()
{
	{
#if defined(AKA_USE_OPENGL)
		aka::ShaderID vert = aka::Shader::compile(File::readString(Asset::path("shaders/GL/gltf.vert")), aka::ShaderType::Vertex);
		aka::ShaderID frag = aka::Shader::compile(File::readString(Asset::path("shaders/GL/gltf.frag")), aka::ShaderType::Fragment);
		std::vector<aka::Attributes> attributes;
#else
		std::string str = File::readString(Asset::path("shaders/D3D/shader.hlsl"));
		aka::ShaderID vert = aka::Shader::compile(str, aka::ShaderType::Vertex);
		aka::ShaderID frag = aka::Shader::compile(str, aka::ShaderType::Fragment);
		std::vector<aka::Attributes> attributes;
		attributes.push_back(aka::Attributes{ aka::AttributeID(0), "POS" });
		attributes.push_back(aka::Attributes{ aka::AttributeID(0), "NORM" });
		attributes.push_back(aka::Attributes{ aka::AttributeID(0), "TEX" });
		attributes.push_back(aka::Attributes{ aka::AttributeID(0), "COL" });
#endif
		if (vert == ShaderID(0) || frag == ShaderID(0))
		{
			aka::Logger::error("Failed to compile shader");
		}
		else
		{
			aka::Shader::Ptr shader = aka::Shader::create(vert, frag, aka::ShaderID(0), attributes);
			if (shader->valid())
			{
				m_shader = shader;
				m_material = aka::ShaderMaterial::create(shader);
			}
		}
	}
	{
#if defined(AKA_USE_OPENGL)
		// shadow shader
		const char* vertShader = ""
			"#version 330 core\n"
			"layout(location = 0) in vec3 a_position;\n"
			"uniform mat4 u_light;\n"
			"uniform mat4 u_model;\n"
			"void main() {\n"
			"	gl_Position = u_light * u_model * vec4(a_position, 1.0);\n"
			"}\n";
		const char* fragShader = ""
			"#version 330 core\n"
			"void main() {\n"
			"	//gl_FragDepth = gl_FragCoord.z;\n"
			"}\n";
		std::vector<Attributes> attributes;
		ShaderID vert = Shader::compile(vertShader, ShaderType::Vertex);
		ShaderID frag = Shader::compile(fragShader, ShaderType::Fragment);
#else
		// shadow shader
		const char* shader = ""
			"cbuffer constants : register(b0)\n"
			"{\n"
			"	row_major float4x4 u_light;\n"
			"	row_major float4x4 u_model;\n"
			"}\n"
			"struct vs_in\n"
			"{\n"
			"	float3 position : POS;\n"
			"	float3 normal : NORM;\n"
			"	float2 tex : TEX;\n"
			"	float4 color : COL;\n"
			"};\n"
			"struct vs_out\n"
			"{\n"
			"	float4 position : SV_POSITION;\n"
			"};\n"
			"vs_out vs_main(vs_in input)\n"
			"{\n"
			"	vs_out output;\n"
			"	output.position = mul(mul(float4(input.position, 1.0f), u_model), u_light);\n"
			"	return output;\n"
			"}\n"
			"float4 ps_main(vs_out input) : SV_TARGET\n"
			"{\n"
			"	return float4(1.0f, 1.0f, 1.0f, 1.0f);\n"
			"}\n";
		std::vector<Attributes> attributes = { // HLSL only
			Attributes{ AttributeID(0), "POS" },
			Attributes{ AttributeID(0), "NORM" },
			Attributes{ AttributeID(0), "TEX" },
			Attributes{ AttributeID(0), "COL" }
		};
		ShaderID vert = Shader::compile(shader, ShaderType::Vertex);
		ShaderID frag = Shader::compile(shader, ShaderType::Fragment);
#endif
		if (vert == ShaderID(0) || frag == ShaderID(0))
		{
			aka::Logger::error("Failed to compile shadow shader");
		}
		else
		{
			aka::Shader::Ptr shader = aka::Shader::create(vert, frag, aka::ShaderID(0), attributes);
			if (shader->valid())
			{
				m_shadowShader = shader;
				m_shadowMaterial = aka::ShaderMaterial::create(m_shadowShader);
			}
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

	uint32_t shadowMapWidth = 2048;
	uint32_t shadowMapHeight = 2048;
	Sampler shadowSampler{};
	shadowSampler.filterMag = Sampler::Filter::Linear;
	shadowSampler.filterMin = Sampler::Filter::Linear;
	shadowSampler.wrapU = Sampler::Wrap::ClampToEdge;
	shadowSampler.wrapV = Sampler::Wrap::ClampToEdge;
	shadowSampler.wrapW = Sampler::Wrap::ClampToEdge;
	m_shadowCascadeTexture[0] = Texture::create(shadowMapWidth, shadowMapHeight, TextureFormat::Float, TextureComponent::Depth, TextureFlag::RenderTarget, shadowSampler);
	m_shadowCascadeTexture[1] = Texture::create(shadowMapWidth, shadowMapHeight, TextureFormat::Float, TextureComponent::Depth, TextureFlag::RenderTarget, shadowSampler);
	m_shadowCascadeTexture[2] = Texture::create(shadowMapWidth, shadowMapHeight, TextureFormat::Float, TextureComponent::Depth, TextureFlag::RenderTarget, shadowSampler);
	FramebufferAttachment attachments[] = {
		FramebufferAttachment{
			FramebufferAttachmentType::Depth,
			m_shadowCascadeTexture[0]
		}
	}; 
	m_shadowFramebuffer = Framebuffer::create(shadowMapWidth, shadowMapHeight, attachments, sizeof(attachments) / sizeof(FramebufferAttachment));

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

template <typename T = real_t>
struct Frustum
{
	Frustum(const mat4<T>& projection);
	Frustum(float left, float right, float bottom, float top, float near, float far);

	point3<T> center() const;

	point3<T>& operator[](size_t index) { return m_corners[index]; }
	const point3<T>& operator[](size_t index) const { return m_corners[index]; }
	point3<T>* data() { return m_corners; }
	const point3<T>* data() const { return m_corners; }

	bool operator==(const Frustum<T>& rhs) const;
	bool operator!=(const Frustum<T>& rhs) const;
private:
	point3<T> m_corners[8];
};

template <typename T>
Frustum<T>::Frustum(const mat4<T>& frustum)
{
	mat4<T> frustumInverse = mat4<T>::inverse(frustum);
#if defined(GEOMETRY_CLIP_SPACE_NEGATIVE)
	const T clipMinZ = static_cast<T>(-1);
#else
	const T clipMinZ = static_cast<T>(0);
#endif
	const T clipMaxZ = static_cast<T>(1);
	// https://gamedev.stackexchange.com/questions/183196/calculating-directional-shadow-map-using-camera-frustum
	m_corners[0] = frustumInverse.multiplyPoint(point3<T>(static_cast<T>(-1), static_cast<T>(-1), clipMinZ));
	m_corners[1] = frustumInverse.multiplyPoint(point3<T>(static_cast<T>(-1), static_cast<T>(-1), clipMaxZ));
	m_corners[2] = frustumInverse.multiplyPoint(point3<T>(static_cast<T>( 1), static_cast<T>(-1), clipMinZ));
	m_corners[3] = frustumInverse.multiplyPoint(point3<T>(static_cast<T>(-1), static_cast<T>( 1), clipMinZ));
	m_corners[4] = frustumInverse.multiplyPoint(point3<T>(static_cast<T>( 1), static_cast<T>(-1), clipMaxZ));
	m_corners[5] = frustumInverse.multiplyPoint(point3<T>(static_cast<T>(-1), static_cast<T>( 1), clipMaxZ));
	m_corners[6] = frustumInverse.multiplyPoint(point3<T>(static_cast<T>( 1), static_cast<T>( 1), clipMinZ));
	m_corners[7] = frustumInverse.multiplyPoint(point3<T>(static_cast<T>( 1), static_cast<T>( 1), clipMaxZ));
}

template <typename T>
point3<T> Frustum<T>::center() const
{
	point3<T> c = point3<T>(0.f);
	for (size_t i = 0; i < 8; i++)
		for (size_t id = 0; id < 3; id++)
			c.data[id] += m_corners[i].data[id];
	return c / static_cast<T>(8);
}

// --- Shadows
// Compute the shadow projection around the view projection.
mat4f computeShadowViewProjectionMatrix(const mat4f& view, const mat4f& projection, float near, float far, const vec3f& lightDirWorld)
{
	Frustum frustum(projection * view);
	point3f centerWorld = frustum.center();
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
		bbox.include(lightViewMatrix.multiplyPoint(frustum[i]));
	// snap bbox to upper bounds
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
	static aka::RenderPass renderPass{};
	Framebuffer::Ptr backbuffer = GraphicBackend::backbuffer();
	mat4f debugView = mat4f::inverse(mat4f::lookAt(m_model->bbox.center() + m_model->bbox.extent(), point3f(0.f)));
	mat4f view = mat4f::inverse(m_camera.transform());
	// TODO use camera (Arcball inherit camera ?)
	static float near = 0.01f;
	static float far = 100.f;
	static anglef angle = anglef::degree(90.f);
	mat4f debugPerspective = mat4f::perspective(angle, (float)backbuffer->width() / (float)backbuffer->height(), 0.01f, 1000.f);
	mat4f perspective = mat4f::perspective(angle, (float)backbuffer->width() / (float)backbuffer->height(), near, far);

	// Generate shadow cascades
	mat4f worldToDepthCascadeMatrix[3];
	const size_t cascadeCount = 3;
	const float offset[cascadeCount + 1] = { near, far / 20.f, far / 5.f, far };
	float cascadeEndClipSpace[cascadeCount];
	for (size_t i = 0; i < cascadeCount; i++)
	{
		float w = (float)backbuffer->width();
		float h = (float)backbuffer->height();
		float n = offset[i];
		float f = offset[i + 1];
		mat4f p = mat4f::perspective(angle, w / h, n, f);
		worldToDepthCascadeMatrix[i] = computeShadowViewProjectionMatrix(view, p, n, f, m_lightDir);
		vec4f clipSpace = perspective * vec4f(0.f, 0.f, -offset[i + 1], 1.f);
		cascadeEndClipSpace[i] = clipSpace.z;
	}
	mat4f projectionToTextureCoordinateMatrix(
		col4f(0.5, 0.0, 0.0, 0.0),
		col4f(0.0, 0.5, 0.0, 0.0),
		col4f(0.0, 0.0, 0.5, 0.0),
		col4f(0.5, 0.5, 0.5, 1.0)
	);

	// --- Shadow pass
	static RenderPass shadowPass;
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
		m_shadowMaterial->set<mat4f>("u_light", worldToDepthCascadeMatrix[i]);
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
	backbuffer->clear(color4f(0.f, 0.f, 0.f, 1.f), 1.f, 0, ClearMask::All);
	
	// --- Normal pass
	mat4f renderView = view;
	mat4f renderPerspective = perspective;
	mat4f worldToDepthTextureMatrix[cascadeCount];
	for (size_t i = 0; i < cascadeCount; i++)
		worldToDepthTextureMatrix[i] = projectionToTextureCoordinateMatrix * worldToDepthCascadeMatrix[i];
		
	if (Keyboard::pressed(KeyboardKey::ControlLeft))
	{
		renderView = debugView;
		renderPerspective = debugPerspective;
	}
	m_material->set<mat4f>("u_view", renderView);
	m_material->set<mat4f>("u_projection", renderPerspective);
	m_material->set<mat4f>("u_light[0]", worldToDepthTextureMatrix, cascadeCount);
	m_material->set<vec3f>("u_lightDir", m_lightDir);
	m_material->set<Texture::Ptr>("u_shadowTexture[0]", m_shadowCascadeTexture, cascadeCount);
	m_material->set<float>("u_cascadeEndClipSpace[0]", cascadeEndClipSpace, cascadeCount);
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
			Renderer3D::drawFrustum(worldToDepthCascadeMatrix[hovered]);
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

	{
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
				static char buffer[256];
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
	}
}

};
