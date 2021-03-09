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
	aka::ShaderMaterial::Ptr m_material;
	aka::Shader::Ptr m_shader;
	Model::Ptr m_model;
	ArcballCamera m_camera;
};

};