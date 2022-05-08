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
	aka::gfx::BufferHandle ubo[2]; // model buffer
};

inline aka::gfx::PipelineHandle resizePipeline(aka::gfx::GraphicDevice* device, aka::gfx::PipelineHandle pipeline, uint32_t w, uint32_t h)
{
	const aka::gfx::Pipeline p = *pipeline.data;
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
inline aka::gfx::PipelineHandle reloadPipeline(aka::gfx::GraphicDevice* device, aka::gfx::PipelineHandle pipeline, aka::gfx::ProgramHandle program)
{
	const aka::gfx::Pipeline p = *pipeline.data;
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
	aka::gfx::BufferHandle m_cameraUniformBuffer;
	aka::gfx::BufferHandle m_viewportUniformBuffer;

	// gbuffers pass
	aka::gfx::VertexBindingState m_gbufferVertices;
	aka::gfx::PipelineHandle m_gbufferPipeline;
	aka::gfx::TextureHandle m_position;
	aka::gfx::TextureHandle m_albedo;
	aka::gfx::TextureHandle m_normal;
	aka::gfx::TextureHandle m_depth;
	aka::gfx::TextureHandle m_material;
	aka::gfx::FramebufferHandle m_gbuffer;
	aka::gfx::ProgramHandle m_gbufferProgram;
	aka::gfx::DescriptorSetHandle m_viewDescriptorSet;

	// Lighing pass
	aka::Mesh* m_quad;
	aka::Mesh* m_sphere;
	aka::gfx::SamplerHandle m_shadowSampler;
	aka::gfx::SamplerHandle m_defaultSampler;
	aka::gfx::PipelineHandle m_ambientPipeline;
	aka::gfx::DescriptorSetHandle m_ambientDescriptorSet;
	aka::gfx::ProgramHandle m_ambientProgram;
	aka::gfx::PipelineHandle m_pointPipeline;
	aka::gfx::DescriptorSetHandle m_pointDescriptorSet;
	aka::gfx::ProgramHandle m_pointProgram;
	aka::gfx::PipelineHandle m_dirPipeline;
	aka::gfx::DescriptorSetHandle m_dirDescriptorSet;
	aka::gfx::ProgramHandle m_dirProgram;

	// Skybox
	aka::Mesh* m_cube;
	aka::gfx::TextureHandle m_skybox;
	aka::gfx::SamplerHandle m_skyboxSampler;
	aka::gfx::DescriptorSetHandle m_skyboxDescriptorSet;
	aka::gfx::ProgramHandle m_skyboxProgram;
	aka::gfx::PipelineHandle m_skyboxPipeline;

	// Text pass
	aka::gfx::DescriptorSetHandle m_textDescriptorSet;
	//const aka::Program* m_textProgram;

	// Post process pass
	aka::gfx::PipelineHandle m_postPipeline;
	//const aka::Texture* m_storageDepth;
	aka::gfx::TextureHandle m_storage;
	aka::gfx::FramebufferHandle m_storageFramebuffer;
	aka::gfx::FramebufferHandle m_storageDepthFramebuffer;
	aka::gfx::DescriptorSetHandle m_postprocessDescriptorSet;
	aka::gfx::ProgramHandle m_postprocessProgram;
};

};