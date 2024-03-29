#include "GameApp.h"

#include "System/SceneSystem.h"
#include "System/RenderSystem.h"
#include "System/ScriptSystem.h"
#include "System/ShadowMapSystem.h"

namespace app {


void Game::onCreate(int argc, char* argv[])
{
	ProgramManager* program = Application::program();
	program->parse(ResourceManager::path("shaders/shader.json"));

	m_world.attach<SceneSystem>();
	m_world.attach<ShadowMapSystem>();
	m_world.attach<RenderSystem>();
	m_world.attach<ScriptSystem>();

	// --- Model
	ResourceManager* resource = Application::resource();
	resource->parse("library/library.json");
	Scene::load(m_world, "library/scene.json");
}

void Game::onDestroy()
{
}

void Game::onUpdate(aka::Time deltaTime)
{
	PlatformDevice* device = Application::platform();
	// Quit the app if requested
	if (device->keyboard().pressed(KeyboardKey::Escape))
	{
		EventDispatcher<QuitEvent>::emit();
	}
}

void Game::onResize(uint32_t width, uint32_t height)
{
}

void Game::onRender()
{
}

};
