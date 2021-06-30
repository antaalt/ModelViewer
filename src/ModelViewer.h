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
	aka::ShaderMaterial::Ptr m_material;
	vec3f m_lightDir;
	Model::Ptr m_model;
	ArcballCamera m_camera;
};

};