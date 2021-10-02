#pragma once

#include <Aka/Aka.h>

#include "Model/Importer.h"
#include "EditorUI/EditorWindow.h"

namespace app {

class Editor : public aka::Application
{
public:
	void onCreate(int argc, char* argv[]) override;
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
};

};