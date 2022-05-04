#pragma once

#include <Aka/Aka.h>

namespace app {

/*template <typename T>
struct UniformBuffer
{
	void update(aka::gfx::GraphicDevice* device, const T& data);

	aka::gfx::Buffer* buffer;
	uint32_t offset;
};

template <typename T, size_t Size = 1024>
class UniformBufferAllocator
{
public:
	UniformBufferAllocator(aka::gfx::GraphicDevice* device);
	~UniformBufferAllocator();
	UniformBuffer<T>* acquire();
	void release(UniformBuffer<T>* buffer);
private:
	uint32_t m_offset;
	aka::gfx::Buffer* m_buffer;
	aka::gfx::GraphicDevice* m_device;
};

template <typename T>
void UniformBuffer<T>::update(aka::gfx::GraphicDevice* device, const T& data) 
{
	device->upload(buffer, &data, offset, sizeof(T));
}

template <typename T, size_t Size>
UniformBufferAllocator<T, Size>::UniformBufferAllocator(aka::gfx::GraphicDevice* device)
{
	m_offset = 0;
	m_device = device;
	m_buffer = m_device->createBuffer(BufferType::Uniform, sizeof(T) * Size, aka::gfx::BufferUsage::Default, aka::gfx::BufferCPUAccess::None);
}
template <typename T, size_t Size>
UniformBufferAllocator<T, Size>::~UniformBufferAllocator()
{
	m_device->destroy(m_buffer);
}
template <typename T, size_t Size>
UniformBuffer<T>* UniformBufferAllocator<T, Size>::acquire()
{
	uint32_t offset = m_offset;
	m_offset += sizeof(T);
	return UniformBuffer<T>{ m_buffer, offset };
}
template <typename T, size_t Size>
void UniformBufferAllocator<T, Size>::release(UniformBuffer<T>* buffer)
{
	// nothing in linear allocator
}*/


/*struct Material {
	color4f color;
	gfx::Texture* albedo;
	gfx::Texture* normal;
	gfx::Texture* material;
};

struct Mesh {
	std::vector<uint32_t> indices;
	std::vector<Vertex> vertices;
	Material* material;
};

using namespace aka;
namespace rnd {

struct InstanceData {
	mat4f model;
	mat3f normal;
};

struct MeshBatch
{
	static const aka::gfx::VertexBindingState bindings;
	//gfx::Buffer* position; // TODO separate position for shadow
	gfx::Buffer* vertices;

	gfx::Buffer* indices;
	gfx::IndexFormat format; // Depend on vertices count.

	gfx::Buffer* materialBuffer;
	gfx::DescriptorSet* materialDescriptorSet;

	struct SingleMesh {
		uint32_t count; // index count in batch
		uint32_t offset; // index offset in batch

		// Matrices in 
		gfx::Buffer* matricesBuffer;
		gfx::DescriptorSet* matricesDescriptorSet;

		uint32_t instanceCount;

		void setInstanceData(uint32_t i, const InstanceData& data);
	};
	std::vector<SingleMesh> meshes;

	void draw(gfx::CommandList* cmd)
	{
		// draw indirect.
		// material index, access from material buffer

		// TODO check pipeline ?
		uint32_t offsets = 0;
		cmd->bindVertexBuffer(&vertices, 0, 1, &offsets);
		cmd->bindIndexBuffer(indices, format, 0);
		cmd->bindDescriptorSet(0, view); // View descriptor set
		for (SingleMesh& mesh : meshes)
		{
			cmd->bindDescriptorSet(1, mesh.matricesDescriptorSet);
			cmd->bindDescriptorSet(2, materialDescriptorSet);
			cmd->drawIndexed(mesh.count, mesh.offset, mesh.instanceCount);
		}
	}
};

struct MeshData {
	std::vector<uint32_t> indices;
	std::vector<Vertex> vertices;
	Material material;
};

struct LightData;

class Renderer
{
public:
	Renderer();
	~Renderer();

public: // Renderer cycles
	// Create a renderer
	void create(gfx::GraphicDevice* device);
	// Destroy a renderer
	void destroy();
	// Update the renderer
	void update(Time deltaTime);
	// Render a frame
	void render(gfx::Frame* frame);

public: // Entity creation
	// Create a mesh and register it in renderer
	Entity createMesh(World& world, const MeshData& mesh);
	// Create a light and register it in renderer
	Entity createLight(World& world, const LightData& light);
	// Destroy an entity linked to renderer
	void destroy(Entity entity);

private:
	// Create all the render targets
	void createRenderTarget();
	// Destroy all the render targets
	void destroyRenderTarget();

private:
	void renderShadowMap(gfx::Frame* frame);
	void renderFrame(gfx::Frame* frame);
	void renderLights(gfx::Frame* frame);
	void renderPostProcess(gfx::Frame* frame);

private:
	gfx::GraphicDevice* m_device;
	std::vector<MeshBatch> m_batches; // TODO map key material

	std::vector<Entity> m_meshEntities; // TODO linked to a specific mesh
	std::vector<Entity> m_lightEntities;
};

}; // rnd


struct Scene
{
	// Create a batch from a mesh list
	BatchID create(const Mesh* meshes, size_t count, const Material& material);
	// Destroy a batch
	void destroy(BatchID batchID);
	// Destroy all batches
	void destroy();

	// Renderer data.
	std::vector<rnd::MeshBatch> batches;

	// Scene
	std::vector<std::vector<Mesh>> meshes;
	std::vector<Material> materials;
};*/


// Vertex struct bound to this render system
struct Vertex {
	aka::point3f position;
	aka::norm3f normal;
	aka::uv2f texcoord;
	aka::color4f color;
	//static VertexAttribute* get();
};

struct RenderComponent
{
	aka::gfx::DescriptorSetHandle material; // gbuffer descriptor set
	aka::gfx::DescriptorSetHandle matrices; // gbuffer descriptor set
	const aka::gfx::Buffer* ubo[2]; // model buffer
};

inline const aka::gfx::Pipeline* resizePipeline(aka::gfx::GraphicDevice* device, const aka::gfx::Pipeline* pipeline, uint32_t w, uint32_t h)
{
	const aka::gfx::Pipeline p = *pipeline;
	device->destroy(pipeline);
	aka::gfx::ViewportState viewport = p.viewport;
	viewport.viewport.w = w;
	viewport.viewport.h = h;
	viewport.scissor.w = w;
	viewport.scissor.h = h;
	return device->createPipeline(
		p.program,
		p.primitive,
		p.framebuffer,
		p.vertices,
		viewport,
		p.depth,
		p.stencil,
		p.cull,
		p.blend,
		p.fill
	);
}
inline const aka::gfx::Pipeline* reloadPipeline(aka::gfx::GraphicDevice* device, const aka::gfx::Pipeline* pipeline, const aka::gfx::Program* program)
{
	const aka::gfx::Pipeline p = *pipeline;
	device->destroy(pipeline);
	return device->createPipeline(
		program,
		p.primitive,
		p.framebuffer,
		p.vertices,
		p.viewport,
		p.depth,
		p.stencil,
		p.cull,
		p.blend,
		p.fill
	);
}

class RenderSystem : 
	public aka::System,
	public aka::EventListener<aka::BackbufferResizeEvent>,
	public aka::EventListener<aka::ProgramReloadedEvent>
{
protected:
	void onCreate(aka::World& world) override;
	void onDestroy(aka::World& world) override;

	void onRender(aka::World& world, aka::gfx::Frame* frame) override;
	void onReceive(const aka::BackbufferResizeEvent& e) override;
	void onReceive(const aka::ProgramReloadedEvent& e) override;
 private:
	void createRenderTargets(uint32_t width, uint32_t height);
	void destroyRenderTargets();
private:
	// Uniforms
	const aka::gfx::Buffer* m_cameraUniformBuffer;
	const aka::gfx::Buffer* m_viewportUniformBuffer;

	// gbuffers pass
	aka::gfx::VertexBindingState m_gbufferVertices;
	const aka::gfx::Pipeline* m_gbufferPipeline;
	aka::gfx::TextureHandle m_position;
	aka::gfx::TextureHandle m_albedo;
	aka::gfx::TextureHandle m_normal;
	aka::gfx::TextureHandle m_depth;
	aka::gfx::TextureHandle m_material;
	const aka::gfx::Framebuffer* m_gbuffer;
	const aka::gfx::Program* m_gbufferProgram;
	aka::gfx::DescriptorSetHandle m_viewDescriptorSet;

	// Lighing pass
	aka::Mesh* m_quad;
	aka::Mesh* m_sphere;
	const aka::gfx::Sampler* m_shadowSampler;
	const aka::gfx::Sampler* m_defaultSampler;
	const aka::gfx::Pipeline* m_ambientPipeline;
	aka::gfx::DescriptorSetHandle m_ambientDescriptorSet;
	const aka::gfx::Program* m_ambientProgram;
	const aka::gfx::Pipeline* m_pointPipeline;
	aka::gfx::DescriptorSetHandle m_pointDescriptorSet;
	const aka::gfx::Program* m_pointProgram;
	const aka::gfx::Pipeline* m_dirPipeline;
	aka::gfx::DescriptorSetHandle m_dirDescriptorSet;
	const aka::gfx::Program* m_dirProgram;

	// Skybox
	aka::Mesh* m_cube;
	aka::gfx::TextureHandle m_skybox;
	const aka::gfx::Sampler* m_skyboxSampler;
	aka::gfx::DescriptorSetHandle m_skyboxDescriptorSet;
	const aka::gfx::Program* m_skyboxProgram;
	const aka::gfx::Pipeline* m_skyboxPipeline;

	// Text pass
	aka::gfx::DescriptorSetHandle m_textDescriptorSet;
	//const aka::Program* m_textProgram;

	// Post process pass
	const aka::gfx::Pipeline* m_postPipeline;
	//const aka::Texture* m_storageDepth;
	aka::gfx::TextureHandle m_storage;
	const aka::gfx::Framebuffer* m_storageFramebuffer;
	const aka::gfx::Framebuffer* m_storageDepthFramebuffer;
	aka::gfx::DescriptorSetHandle m_postprocessDescriptorSet;
	const aka::gfx::Program* m_postprocessProgram;

	//UniformBufferAllocator<MatricesUniformBuffer> m_matricesBuffer;
	//UniformBufferAllocator<MaterialUniformBuffer> m_matricesBuffer;
};

};