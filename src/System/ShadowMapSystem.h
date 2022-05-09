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
	aka::gfx::ProgramHandle m_shadowProgram;
	aka::gfx::ProgramHandle m_shadowPointProgram;
	aka::gfx::GraphicPipelineHandle m_shadowPipeline;
	aka::gfx::GraphicPipelineHandle m_shadowPointPipeline;
};

};