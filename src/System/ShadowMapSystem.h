#pragma once

#include <Aka/Aka.h>

#include "RenderSystem.h"

namespace viewer {

class ShadowMapSystem : 
	public aka::System,
	public aka::EventListener<ShaderHotReloadEvent>
{
public:
	void onCreate(aka::World& world) override;
	void onDestroy(aka::World& world) override;

	void onRender(aka::World& world) override;

	void onReceive(const ShaderHotReloadEvent& e) override;
private:
	void createShaders();
private:
	aka::Framebuffer::Ptr m_shadowFramebuffer;
	aka::ShaderMaterial::Ptr m_shadowMaterial;
	aka::ShaderMaterial::Ptr m_shadowPointMaterial;
	aka::Buffer::Ptr m_modelUniformBuffer;
	aka::Buffer::Ptr m_pointLightUniformBuffer;
	aka::Buffer::Ptr m_directionalLightUniformBuffer;
};

};