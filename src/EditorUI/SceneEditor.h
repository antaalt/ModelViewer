#pragma once

#include "EditorWindow.h"

namespace app {

class SceneEditor : public EditorWindow
{
public:
	SceneEditor();
	void onCreate(aka::World& world) override;
	void onDestroy(aka::World& world) override;
	void onUpdate(aka::World& world, aka::Time deltaTime) override;
	void onRender(aka::World& world, aka::gfx::Frame* frame) override;
private:
	void drawWireFrame(const aka::mat4f& model, const aka::mat4f& view, const aka::mat4f& projection, const aka::SubMesh& submesh);
private:
	entt::entity m_currentEntity;
	uint32_t m_gizmoOperation;
	char m_entityName[256];
	const aka::gfx::Program* m_wireframeProgram;
	aka::gfx::DescriptorSetHandle m_wireframeDescriptorSet;
	const aka::gfx::Buffer* m_wireFrameUniformBuffer;
};

};