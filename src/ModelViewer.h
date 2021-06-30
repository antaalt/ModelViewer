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
	aka::Texture::Ptr m_shadowCascadeTexture[3];
	aka::Framebuffer::Ptr m_shadowFramebuffer;
	aka::ShaderMaterial::Ptr m_shadowMaterial;

	aka::Texture::Ptr m_position;
	aka::Texture::Ptr m_albedo;
	aka::Texture::Ptr m_normal;
	aka::Texture::Ptr m_depth;
	aka::Framebuffer::Ptr m_gbuffer;
	aka::ShaderMaterial::Ptr m_gbufferMaterial;

	aka::Mesh::Ptr m_quad;
	aka::ShaderMaterial::Ptr m_lightingMaterial;

	aka::ShaderMaterial::Ptr m_material;
	vec3f m_lightDir;
	Model::Ptr m_model;
	ArcballCamera m_camera;
};

};