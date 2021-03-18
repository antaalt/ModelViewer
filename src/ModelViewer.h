#pragma once

#include <Aka/Aka.h>

#include "Model/ModelLoader.h"

namespace viewer {

class Viewer : public aka::View
{
	void loadShader();
public:
	void onCreate() override;
	void onDestroy() override;
	void onUpdate(Time::Unit deltaTime) override;
	void onRender() override;
private:
	aka::Framebuffer::Ptr m_shadowFramebuffer;
	aka::ShaderMaterial::Ptr m_shadowMaterial;
	aka::Shader::Ptr m_shadowShader;
	aka::ShaderMaterial::Ptr m_material;
	aka::Shader::Ptr m_shader;
	vec3f m_lightDir;
	Model::Ptr m_model;
	ArcballCamera m_camera;
};

};