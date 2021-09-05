#include "ModelViewer.h"

#include <imgui.h>
#include <imguizmo.h>
#include <Aka/Layer/ImGuiLayer.h>

#include "System/SceneSystem.h"
#include "System/RenderSystem.h"
#include "System/ShadowMapSystem.h"

#include "Editor/SceneEditor.h"
#include "Editor/InfoEditor.h"
#include "Editor/AssetEditor.h"

namespace viewer {


void Viewer::onCreate(int argc, char* argv[])
{
	m_world.attach<SceneSystem>();
	m_world.attach<ShadowMapSystem>();
	m_world.attach<RenderSystem>();
	m_world.create();

	m_editors.push_back(new SceneEditor);
	m_editors.push_back(new InfoEditor);
	m_editors.push_back(new AssetEditor);
	for (EditorWindow* editor : m_editors)
		editor->onCreate(m_world);

	// --- Lights
	TextureSampler shadowSampler{};
	shadowSampler.filterMag = TextureFilter::Nearest;
	shadowSampler.filterMin = TextureFilter::Nearest;
	shadowSampler.wrapU = TextureWrap::ClampToEdge;
	shadowSampler.wrapV = TextureWrap::ClampToEdge;
	shadowSampler.wrapW = TextureWrap::ClampToEdge;
	shadowSampler.anisotropy = 1.f;

	ResourceManager::parse("library/library.json");
	Scene::load(m_world, "library/scene.json");

	{ // Sun
		m_sun = m_world.createEntity("SunEditor");
		m_sun.add<Transform3DComponent>();
		m_sun.add<Hierarchy3DComponent>();
		m_sun.add<DirectionalLightComponent>();
		m_sun.add<DirtyLightComponent>();
		Transform3DComponent& sunTransform = m_sun.get<Transform3DComponent>();
		Hierarchy3DComponent& h = m_sun.get<Hierarchy3DComponent>();
		DirectionalLightComponent& sun = m_sun.get<DirectionalLightComponent>();
		sun.direction = vec3f::normalize(vec3f(0.1f, 1.f, 0.1f));
		sun.color = color3f(1.f);
		sun.intensity = 10.f;
		sunTransform.transform = mat4f::identity();
		sun.shadowMap[0] = Texture::create2D(2048, 2048, TextureFormat::Depth, TextureFlag::RenderTarget, shadowSampler);
		sun.shadowMap[1] = Texture::create2D(2048, 2048, TextureFormat::Depth, TextureFlag::RenderTarget, shadowSampler);
		sun.shadowMap[2] = Texture::create2D(4096, 4096, TextureFormat::Depth, TextureFlag::RenderTarget, shadowSampler);
		// TODO texture atlas & single shader execution
		for (size_t i = 0; i < 3; i++)
			sun.worldToLightSpaceMatrix[i] = mat4f::identity();
	}

	// --- Editor Camera
	{
		Entity e = Scene::getMainCamera(m_world);
		if (e.valid())
		{
			m_camera = e;
		}
		else
		{
			m_camera = m_world.createEntity("CameraEditor");
			m_camera.add<Transform3DComponent>();
			m_camera.add<Hierarchy3DComponent>();
			m_camera.add<Camera3DComponent>();
			m_camera.add<DirtyCameraComponent>();
			Transform3DComponent& transform = m_camera.get<Transform3DComponent>();
			Camera3DComponent& camera = m_camera.get<Camera3DComponent>();

			auto perspective = std::make_unique<CameraPerspective>();
			perspective->nearZ = 0.01f;
			perspective->farZ = 100.f;
			perspective->hFov = anglef::degree(90.f);
			perspective->ratio = width() / (float)height();

			auto arcball = std::make_unique<CameraArcball>();
			arcball->set(aabbox<>(point3f(0.f), point3f(1.f)));

			transform.transform = arcball->transform();
			camera.view = mat4f::inverse(transform.transform);
			camera.projection = std::move(perspective);
			camera.controller = std::move(arcball);
		}

		m_world.registry().patch<Camera3DComponent>(m_camera.handle());
	}
	

	// --- UI
	m_debug = true;
	attach<ImGuiLayer>();
}

void Viewer::onDestroy()
{
	for (EditorWindow* editor : m_editors)
	{
		editor->onDestroy(m_world);
		delete editor;
	}
	m_editors.clear();
	m_world.destroy();
}

void Viewer::onUpdate(aka::Time::Unit deltaTime)
{
	// Camera status
	m_camera.get<Camera3DComponent>().active = !ImGui::GetIO().WantCaptureKeyboard && !ImGui::GetIO().WantCaptureMouse && !ImGuizmo::IsUsing();

	// TOD
	if (Mouse::pressed(MouseButton::ButtonMiddle))
	{
		const Position& pos = Mouse::position();
		float x = pos.x / (float)GraphicBackend::backbuffer()->width();
		if (m_sun.valid() && m_sun.has<DirectionalLightComponent>())
		{
			m_sun.get<DirectionalLightComponent>().direction = vec3f::normalize(lerp(vec3f(1, 1, 1), vec3f(-1, 1, -1), x));
			if (!m_sun.has<DirtyLightComponent>())
				m_sun.add<DirtyLightComponent>();
		}
	}

	// Reset
	if (Keyboard::down(KeyboardKey::R) && !ImGui::GetIO().WantCaptureKeyboard)
	{
		// Compute scene bounds.
		aabbox<> bounds;
		auto view = m_world.registry().view<Transform3DComponent, MeshComponent>();
		view.each([&](Transform3DComponent& t, MeshComponent& mesh) {
			bounds.include(t.transform * mesh.bounds);
		});
		m_camera.get<Camera3DComponent>().controller->set(bounds);
		m_world.registry().patch<Camera3DComponent>(m_camera.handle());
	}
	if (Keyboard::down(KeyboardKey::D) && !ImGui::GetIO().WantCaptureKeyboard)
	{
		m_debug = !m_debug;
	}
	if (Keyboard::down(KeyboardKey::PrintScreen) && !ImGui::GetIO().WantCaptureKeyboard)
	{
		GraphicBackend::screenshot("screen.png");
	}
	// Hot reload
	if (Keyboard::down(KeyboardKey::Space) && !ImGui::GetIO().WantCaptureKeyboard)
	{
		Logger::info("Reloading shaders...");
		EventDispatcher<ShaderHotReloadEvent>::emit();
	}
	// Quit the app if requested
	if (Keyboard::pressed(KeyboardKey::Escape) && !ImGui::GetIO().WantCaptureKeyboard)
	{
		EventDispatcher<QuitEvent>::emit();
	}
	m_world.update(deltaTime);
	// Editor
	for (EditorWindow* editor : m_editors)
		editor->onUpdate(m_world);
}

void Viewer::onResize(uint32_t width, uint32_t height)
{
}

void Viewer::onRender()
{
	m_world.render();

	if (m_debug)
	{
		// TODO move to UI system ?
		// --- Editor pass
		for (EditorWindow* editor : m_editors)
			editor->onRender(m_world);
	}
}

};
