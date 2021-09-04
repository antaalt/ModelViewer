#pragma once

#include <Aka/Aka.h>

#include "Model/ModelLoader.h"
#include "Editor/Editor.h"

namespace viewer {

class Viewer : public aka::Application
{
public:
	void onCreate() override;
	void onDestroy() override;
	void onUpdate(Time::Unit deltaTime) override;
	void onResize(uint32_t width, uint32_t height) override;
	void onRender() override;
private:
	// Debug info
	bool m_debug;
	std::vector<EditorWindow*> m_editors;

	// Scene
	aka::World m_world;
	aka::Entity m_sun;
	aka::Entity m_camera;
	aka::CameraPerspective m_projection;
};

};