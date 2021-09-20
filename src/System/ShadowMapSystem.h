#pragma once

#include <Aka/Aka.h>

#include "RenderSystem.h"

namespace viewer {

class ShadowMapSystem : 
	public aka::System,
	public aka::EventListener<aka::ProgramReloadedEvent>
{
public:
	void onCreate(aka::World& world) override;
	void onDestroy(aka::World& world) override;

	void onRender(aka::World& world) override;

	void onReceive(const aka::ProgramReloadedEvent& e) override;
private:
	aka::Framebuffer::Ptr m_shadowFramebuffer;
	aka::Material::Ptr m_shadowMaterial;
	aka::Material::Ptr m_shadowPointMaterial;
	aka::Buffer::Ptr m_modelUniformBuffer;
	aka::Buffer::Ptr m_pointLightUniformBuffer;
	aka::Buffer::Ptr m_directionalLightUniformBuffer;
};

};