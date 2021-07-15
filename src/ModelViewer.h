#pragma once

#include <Aka/Aka.h>

#include "Model/ModelLoader.h"

namespace viewer {

class Viewer : public aka::Application
{
	void loadShader();
public:
	void onCreate() override;
	void onDestroy() override;
	void onUpdate(Time::Unit deltaTime) override;
	void onRender() override;
private:
	// shadow map pass
	static const size_t cascadeCount = 3;
	aka::Texture::Ptr m_shadowCascadeTexture[cascadeCount];
	aka::Framebuffer::Ptr m_shadowFramebuffer;
	aka::ShaderMaterial::Ptr m_shadowMaterial;

	// gbuffers pass
	aka::Texture::Ptr m_position;
	aka::Texture::Ptr m_albedo;
	aka::Texture::Ptr m_normal;
	aka::Texture::Ptr m_depth;
	aka::Texture::Ptr m_roughness;
	aka::Framebuffer::Ptr m_gbuffer;
	aka::ShaderMaterial::Ptr m_gbufferMaterial;

	// compose pass
	aka::Mesh::Ptr m_quad;
	aka::ShaderMaterial::Ptr m_lightingMaterial;

	// Skybox
	aka::Mesh::Ptr m_cube;
	aka::Texture::Ptr m_skybox;
	aka::ShaderMaterial::Ptr m_skyboxMaterial;

	// Forward pass
	aka::ShaderMaterial::Ptr m_material;
	vec3f m_lightDir;
	Model::Ptr m_model;
	ArcballCamera m_camera;

	// FXAA pass
	aka::Texture::Ptr m_storageDepth;
	aka::Texture::Ptr m_storage;
	aka::Framebuffer::Ptr m_storageFramebuffer;
	aka::ShaderMaterial::Ptr m_fxaaMaterial;

};

};