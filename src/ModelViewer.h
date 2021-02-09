#pragma once

#include <Aka/Aka.h>

#include "Model/ModelLoader.h"

namespace viewer {

class Viewer : public aka::Application
{
	void loadShader();
public:
	void initialize() override;
	void destroy() override;
	void update(Time::Unit deltaTime) override;
	void render() override;
private:
	aka::ShaderMaterial::Ptr m_material;
	aka::Shader::Ptr m_shader;
	Model::Ptr m_model;
	ArcballCamera m_camera;
};

};