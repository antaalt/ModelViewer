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

class RenderSystem : 
	public aka::System,
	public aka::EventListener<aka::BackbufferResizeEvent>,
	public aka::EventListener<aka::ProgramReloadedEvent>
{
protected:
	void onCreate(aka::World& world) override;
	void onDestroy(aka::World& world) override;

	void onRender(aka::World& world) override;
	void onReceive(const aka::BackbufferResizeEvent& e) override;
	void onReceive(const aka::ProgramReloadedEvent& e) override;
 private:
	void createRenderTargets(uint32_t width, uint32_t height);
private:
	// Uniforms
	aka::Buffer::Ptr m_cameraUniformBuffer;
	aka::Buffer::Ptr m_viewportUniformBuffer;
	aka::Buffer::Ptr m_modelUniformBuffer;
	aka::Buffer::Ptr m_directionalLightUniformBuffer;
	aka::Buffer::Ptr m_pointLightUniformBuffer;

	// gbuffers pass
	aka::Texture2D::Ptr m_position;
	aka::Texture2D::Ptr m_albedo;
	aka::Texture2D::Ptr m_normal;
	aka::Texture2D::Ptr m_depth;
	aka::Texture2D::Ptr m_material;
	aka::Framebuffer::Ptr m_gbuffer;
	aka::Material::Ptr m_gbufferMaterial;

	// Lighing pass
	aka::Mesh::Ptr m_quad;
	aka::Mesh::Ptr m_sphere;
	aka::TextureSampler m_shadowSampler;
	aka::Material::Ptr m_ambientMaterial;
	aka::Material::Ptr m_pointMaterial;
	aka::Material::Ptr m_dirMaterial;

	// Skybox
	aka::Mesh::Ptr m_cube;
	aka::TextureCubeMap::Ptr m_skybox;
	aka::TextureSampler m_skyboxSampler;
	aka::Material::Ptr m_skyboxMaterial;

	// Text pass
	aka::Material::Ptr m_textMaterial;

	// Post process pass
	aka::Texture2D::Ptr m_storageDepth;
	aka::Texture2D::Ptr m_storage;
	aka::Framebuffer::Ptr m_storageFramebuffer;
	aka::Material::Ptr m_postprocessMaterial;
};

};