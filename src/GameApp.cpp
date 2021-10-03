#include "GameApp.h"

#include "System/SceneSystem.h"
#include "System/RenderSystem.h"
#include "System/ScriptSystem.h"
#include "System/ShadowMapSystem.h"

namespace app {


void Game::onCreate(int argc, char* argv[])
{
	ProgramManager::parse(ResourceManager::path("shaders/shader.json"));
	m_world.attach<SceneSystem>();
	m_world.attach<ShadowMapSystem>();
	m_world.attach<RenderSystem>();
	m_world.attach<ScriptSystem>();
	m_world.create();

	// --- Model
	ResourceManager::parse("library/library.json");
	Scene::load(m_world, "library/scene.json");
}

void Game::onDestroy()
{
	m_world.destroy();
}

void Game::onUpdate(aka::Time deltaTime)
{
	// Quit the app if requested
	if (Keyboard::pressed(KeyboardKey::Escape))
	{
		EventDispatcher<QuitEvent>::emit();
	}
	m_world.update(deltaTime);
}

void Game::onResize(uint32_t width, uint32_t height)
{
}

void Game::onRender()
{
	m_world.render();
}

};
