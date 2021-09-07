#pragma once

#include <Aka/Aka.h>

namespace viewer {

// Vertex struct bound to this render system
struct Vertex {
	aka::point3f position;
	aka::norm3f normal;
	aka::uv2f texcoord;
	aka::color4f color;
	//static VertexAttribute* get();
};

struct ShaderHotReloadEvent {};

class RenderSystem : 
	public aka::System,
	public aka::EventListener<aka::BackbufferResizeEvent>,
	public aka::EventListener<ShaderHotReloadEvent>
{
protected:
	void onCreate(aka::World& world) override;
	void onDestroy(aka::World& world) override;

	void onRender(aka::World& world) override;
	void onReceive(const aka::BackbufferResizeEvent& e) override;
	void onReceive(const ShaderHotReloadEvent& e) override;
 private:
	void createShaders();
	void createRenderTargets(uint32_t width, uint32_t height);
private:
	aka::Buffer::Ptr m_cameraUniformBuffer;
	aka::Buffer::Ptr m_viewportUniformBuffer;
	aka::Buffer::Ptr m_modelUniformBuffer;
	aka::Buffer::Ptr m_directionalLightUniformBuffer;
	aka::Buffer::Ptr m_pointLightUniformBuffer;
	// gbuffers pass
	aka::Texture::Ptr m_position;
	aka::Texture::Ptr m_albedo;
	aka::Texture::Ptr m_normal;
	aka::Texture::Ptr m_depth;
	aka::Texture::Ptr m_material;
	aka::Framebuffer::Ptr m_gbuffer;
	aka::ShaderMaterial::Ptr m_gbufferMaterial;

	// Lighing pass
	aka::Mesh::Ptr m_quad;
	aka::Mesh::Ptr m_sphere;
	aka::TextureSampler m_shadowSampler;
	aka::ShaderMaterial::Ptr m_ambientMaterial;
	//aka::Buffer::Ptr m_ambientUniformBuffer;
	aka::ShaderMaterial::Ptr m_pointMaterial;
	//aka::Buffer::Ptr m_pointUniformBuffer;
	aka::ShaderMaterial::Ptr m_dirMaterial;
	//aka::Buffer::Ptr m_directionalUniformBuffer;

	// Skybox
	aka::Mesh::Ptr m_cube;
	aka::Texture::Ptr m_skybox;
	aka::TextureSampler m_skyboxSampler;
	aka::ShaderMaterial::Ptr m_skyboxMaterial;
	//aka::Buffer::Ptr m_skyboxUniformBuffer;

	// Post process pass
	aka::Texture::Ptr m_storageDepth;
	aka::Texture::Ptr m_storage;
	aka::Framebuffer::Ptr m_storageFramebuffer;
	aka::ShaderMaterial::Ptr m_postprocessMaterial;
	//aka::Buffer::Ptr m_postProcessUniformBuffer;
};

};