#pragma once

#include <Aka/Aka.h>

namespace app {

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
	aka::DescriptorSet* material; // gbuffer descriptor set
	aka::DescriptorSet* matrices; // gbuffer descriptor set
	aka::Buffer* ubo[2]; // model buffer
};

class RenderSystem : 
	public aka::System,
	public aka::EventListener<aka::BackbufferResizeEvent>,
	public aka::EventListener<aka::ProgramReloadedEvent>
{
protected:
	void onCreate(aka::World& world) override;
	void onDestroy(aka::World& world) override;

	void onRender(aka::World& world, aka::Frame* frame) override;
	void onReceive(const aka::BackbufferResizeEvent& e) override;
	void onReceive(const aka::ProgramReloadedEvent& e) override;
 private:
	void createRenderTargets(uint32_t width, uint32_t height);
private:
	// Uniforms
	aka::Buffer* m_cameraUniformBuffer;
	aka::Buffer* m_viewportUniformBuffer;
	//aka::Buffer* m_directionalLightUniformBuffer;
	//aka::Buffer* m_pointLightUniformBuffer;

	// gbuffers pass
	aka::VertexBindingState m_gbufferVertices;
	aka::Pipeline* m_gbufferPipeline;
	aka::Texture* m_position;
	aka::Texture* m_albedo;
	aka::Texture* m_normal;
	aka::Texture* m_depth;
	aka::Texture* m_material;
	aka::Framebuffer* m_gbuffer;
	aka::Program* m_gbufferProgram;
	aka::DescriptorSet* m_viewDescriptorSet;

	// Lighing pass
	aka::Mesh* m_quad;
	aka::Mesh* m_sphere;
	aka::Sampler* m_shadowSampler;
	aka::Sampler* m_defaultSampler;
	aka::Pipeline* m_ambientPipeline;
	aka::DescriptorSet* m_ambientDescriptorSet;
	aka::Program* m_ambientProgram;
	aka::DescriptorSet* m_pointDescriptorSet;
	//aka::Program* m_pointProgram;
	aka::Pipeline* m_dirPipeline;
	aka::DescriptorSet* m_dirDescriptorSet;
	aka::Program* m_dirProgram;

	// Skybox
	aka::Mesh* m_cube;
	aka::Texture* m_skybox;
	aka::Sampler* m_skyboxSampler;
	aka::DescriptorSet* m_skyboxDescriptorSet;
	aka::Program* m_skyboxProgram;
	aka::Pipeline* m_skyboxPipeline;

	// Text pass
	aka::DescriptorSet* m_textDescriptorSet;
	//aka::Program* m_textProgram;

	// Post process pass
	aka::Pipeline* m_postPipeline;
	//aka::Texture* m_storageDepth;
	aka::Texture* m_storage;
	aka::Framebuffer* m_storageFramebuffer;
	aka::Framebuffer* m_storageDepthFramebuffer;
	aka::DescriptorSet* m_postprocessDescriptorSet;
	aka::Program* m_postprocessProgram;
};

};