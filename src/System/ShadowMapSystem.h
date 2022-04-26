#pragma once

#include <Aka/Aka.h>

#include "RenderSystem.h"

namespace app {

class ShadowMapSystem : 
	public aka::System,
	public aka::EventListener<aka::ProgramReloadedEvent>
{
public:
	void onCreate(aka::World& world) override;
	void onDestroy(aka::World& world) override;

	void onRender(aka::World& world, aka::gfx::Frame* frame) override;

	void onReceive(const aka::ProgramReloadedEvent& e) override;
private:
	aka::gfx::Program* m_shadowProgram;
	aka::gfx::Program* m_shadowPointProgram;
	aka::gfx::Pipeline* m_shadowPipeline;
	aka::gfx::Pipeline* m_shadowPointPipeline;
	//aka::Buffer* m_modelUniformBuffer;
	//aka::Buffer* m_pointLightUniformBuffer;
	//aka::Buffer* m_directionalLightUniformBuffer;
};

};