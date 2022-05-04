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
	const aka::gfx::Program* m_shadowProgram;
	const aka::gfx::Program* m_shadowPointProgram;
	const aka::gfx::Pipeline* m_shadowPipeline;
	const aka::gfx::Pipeline* m_shadowPointPipeline;
};

};