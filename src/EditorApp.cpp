#include "EditorApp.h"

#include <imgui.h>
#include <imguizmo.h>

#include "GameApp.h"

#include "System/SceneSystem.h"
#include "System/RenderSystem.h"
#include "System/ScriptSystem.h"
#include "System/ShadowMapSystem.h"

#include "EditorUI/SceneEditor.h"
#include "EditorUI/InfoEditor.h"
#include "EditorUI/AssetEditor.h"

namespace app {

Editor::Editor() :
	Application(std::vector<Layer*>{
#if defined(AKA_USE_IMGUI_LAYER)
		new ImGuiLayer
#endif
	}),
	m_debug(false)
{
}

Editor::~Editor()
{
}

void Editor::onCreate(int argc, char* argv[])
{
	ProgramManager* program = Application::program();
	program->parse(ResourceManager::path("shaders/shader.json"));

	m_world.attach<SceneSystem>();
	m_world.attach<ShadowMapSystem>();
	m_world.attach<RenderSystem>();
	m_world.attach<ScriptSystem>();

	m_editors.push_back(new SceneEditor);
	m_editors.push_back(new InfoEditor);
	m_editors.push_back(new AssetEditor);
	for (EditorWindow* editor : m_editors)
		editor->onCreate(m_world);

	// --- Model
	ResourceManager* resource = Application::resource();
	//resource->parse("library/library.json");
	//Scene::load(m_world, "library/scene.json");
	Importer::importScene("asset/glTF-Sample-Models/2.0/Lantern/glTF/Lantern.glTF", m_world);
	//resource->serialize("");
	//Scene::save("", m_world);

	// --- Lights
	{ // Sun
		m_sun = m_world.createEntity("SunEditor");
		m_sun.add<Transform3DComponent>();
		m_sun.add<Hierarchy3DComponent>();
		m_sun.add<DirectionalLightComponent>();
		Transform3DComponent& sunTransform = m_sun.get<Transform3DComponent>();
		Hierarchy3DComponent& h = m_sun.get<Hierarchy3DComponent>();
		DirectionalLightComponent& sun = m_sun.get<DirectionalLightComponent>();
		sun.direction = vec3f::normalize(vec3f(0.1f, 1.f, 0.1f));
		sun.color = color3f(1.f);
		sun.intensity = 10.f;
		sunTransform.transform = mat4f::identity();
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
			aabbox<> bounds;
			auto view = m_world.registry().view<Transform3DComponent, MeshComponent>();
			view.each([&](Transform3DComponent& t, MeshComponent& mesh) {
				bounds.include(t.transform * mesh.bounds);
			});
			if (bounds.valid())
				arcball->set(bounds);
			else
				arcball->set(aabbox<>(point3f(0.f), point3f(1.f)));

			transform.transform = arcball->transform();
			camera.view = mat4f::inverse(transform.transform);
			camera.projection = std::move(perspective);
			camera.controller = std::move(arcball);
		}

		m_world.registry().patch<Camera3DComponent>(m_camera.handle());
	}
}

void Editor::onDestroy()
{
	graphic()->wait(QueueType::Graphic);
	Scene::destroy(m_world);
	// TODO global detach
	m_world.detach<SceneSystem>();
	m_world.detach<ShadowMapSystem>();
	m_world.detach<RenderSystem>();
	m_world.detach<ScriptSystem>();
	for (EditorWindow* editor : m_editors)
	{
		editor->onDestroy(m_world);
		delete editor;
	}
	m_editors.clear();
}

void Editor::onUpdate(aka::Time deltaTime)
{
	PlatformDevice* platform = Application::platform();

	// Camera status
#if defined(AKA_USE_IMGUI_LAYER)
	bool imguiCaptureKeyboard = ImGui::GetIO().WantCaptureKeyboard;
	m_camera.get<Camera3DComponent>().active = !imguiCaptureKeyboard && !ImGui::GetIO().WantCaptureMouse && !ImGuizmo::IsUsing();
#else
	bool imguiCaptureKeyboard = false;
	m_camera.get<Camera3DComponent>().active = true;
#endif

	// TOD
	const Mouse& mouse = platform->mouse();
	if (mouse.pressed(MouseButton::ButtonMiddle))
	{
		const Position& pos = mouse.position();
		float x = pos.x / (float)width();
		if (m_sun.valid() && m_sun.has<DirectionalLightComponent>())
		{
			m_sun.get<DirectionalLightComponent>().direction = vec3f::normalize(lerp(vec3f(1, 1, 1), vec3f(-1, 1, -1), x));
			if (!m_sun.has<DirtyLightComponent>())
				m_sun.add<DirtyLightComponent>();
		}
	}

	// Reset
	const Keyboard& keyboard = platform->keyboard();
	if (keyboard.down(KeyboardKey::R) && !imguiCaptureKeyboard)
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
	if (keyboard.down(KeyboardKey::D) && !imguiCaptureKeyboard)
	{
		m_debug = !m_debug;
	}
	if (keyboard.down(KeyboardKey::PrintScreen) && !imguiCaptureKeyboard)
	{
		GraphicDevice* device = Application::graphic();
		// TODO create a flag to save in onRender.
		//Backbuffer::Ptr backbuffer = device->backbuffer();
		//Image image(backbuffer->width(), backbuffer->height(), 4, ImageFormat::UnsignedByte);
		//device->backbuffer()->download(image.data());
		//image.encodePNG("screen.png");
	}
	// Quit the app if requested
	if (keyboard.pressed(KeyboardKey::Escape) && !imguiCaptureKeyboard)
	{
		EventDispatcher<QuitEvent>::emit();
	}
	// Editor
	for (EditorWindow* editor : m_editors)
		editor->onUpdate(m_world, deltaTime);
}

void Editor::onResize(uint32_t width, uint32_t height)
{
}

void Editor::onRender(Frame* frame)
{
	if (m_debug)
	{
		// --- Editor pass
		for (EditorWindow* editor : m_editors)
			editor->onRender(m_world, frame);
	}
}

};
