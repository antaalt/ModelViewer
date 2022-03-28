#pragma once

#include <Aka/Aka.h>

#include "Model/Importer.h"
#include "EditorUI/EditorWindow.h"

namespace app {

class Game : public aka::Application
{
public:
	void onCreate(int argc, char* argv[]) override;
	void onDestroy() override;
	void onUpdate(Time deltaTime) override;
	void onResize(uint32_t width, uint32_t height) override;
	void onRender(Frame* frame) override;
private:
	// Scene
	aka::World m_world;
	aka::Entity m_sun;
	aka::Entity m_camera;
};

};