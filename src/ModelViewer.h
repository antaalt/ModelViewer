#pragma once

#include <Aka/Aka.h>

#include "Model/ModelLoader.h"
#include "Editor/Editor.h"

namespace viewer {

class Viewer : public aka::Application
{
	void loadShader();
public:
	void onCreate() override;
	void onDestroy() override;
	void onUpdate(Time::Unit deltaTime) override;
	void onRender() override;
private:
	// Debug info
	bool m_debug;
	std::vector<EditorWindow*> m_editors;

	// shadow map pass
	aka::Framebuffer::Ptr m_shadowFramebuffer;
	aka::ShaderMaterial::Ptr m_shadowMaterial;
	aka::ShaderMaterial::Ptr m_shadowPointMaterial;

	// gbuffers pass
	aka::Texture::Ptr m_position;
	aka::Texture::Ptr m_albedo;
	aka::Texture::Ptr m_normal;
	aka::Texture::Ptr m_depth;
	aka::Texture::Ptr m_roughness;
	aka::Framebuffer::Ptr m_gbuffer;
	aka::ShaderMaterial::Ptr m_gbufferMaterial;

	// compose pass
	aka::Mesh::Ptr m_quad;
	aka::ShaderMaterial::Ptr m_lightingMaterial;

	// Skybox
	aka::Mesh::Ptr m_cube;
	aka::Texture::Ptr m_skybox;
	aka::ShaderMaterial::Ptr m_skyboxMaterial;

	// Forward pass
	aka::ShaderMaterial::Ptr m_material;

	// Scene
	aka::aabbox<> m_bounds;
	aka::World m_world;
	aka::Entity m_sun;
	aka::Entity m_camera;
	aka::CameraPerspective m_projection;

	// FXAA pass
	aka::Texture::Ptr m_storageDepth;
	aka::Texture::Ptr m_storage;
	aka::Framebuffer::Ptr m_storageFramebuffer;
	aka::ShaderMaterial::Ptr m_fxaaMaterial;

};

};