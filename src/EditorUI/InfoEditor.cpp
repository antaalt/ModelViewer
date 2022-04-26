#include "InfoEditor.h"


#include <imgui.h>

namespace app {

void InfoEditor::onRender(aka::World& world, aka::gfx::Frame* frame)
{
	aka::Application* app = aka::Application::app();
	aka::gfx::GraphicDevice* graphic = app->graphic();
	uint32_t width = graphic->backbuffer(frame)->width;
	uint32_t height = graphic->backbuffer(frame)->height;
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;
	ImGui::SetNextWindowPos(ImVec2((float)(width - 5), 25.f), ImGuiCond_Always, ImVec2(1.f, 0.f));
	if (ImGui::Begin("Info", nullptr, flags))
	{
		//static aka::Device device = aka::Device::getDefault();
		ImGuiIO& io = ImGui::GetIO();
		ImGui::Text("Resolution : %ux%u", width, height);
		ImGui::Text("%.3f ms/frame", 1000.0f / ImGui::GetIO().Framerate);
		ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
		//ImGui::Text("Draw call : %zu", m_batch.batchCount());
		//ImGui::Text("Vertices : %zu", m_batch.verticesCount());
		//ImGui::Text("Indices : %zu", m_batch.indicesCount());
		ImGui::Separator();
		const char* apiName[] = {
			"None",
			"Vulkan",
			"OpenGL3",
			"DirectX11"
		};
		ImGui::Text("Api : %s", apiName[(int)graphic->api()]);
		//ImGui::Text("Device : %s", device.vendor);
		//ImGui::Text("Renderer : %s", device.renderer);
	}
	ImGui::End();

}

};
