#pragma once

#include "Editor.h"

namespace viewer {

class SceneEditor : public EditorWindow
{
public:
	SceneEditor();
	void onCreate(aka::World& world) override;
	void onDestroy(aka::World& world) override;
	void onUpdate(aka::World& world) override;
	void onRender(aka::World& world) override;
private:
	entt::entity m_currentEntity;
	uint32_t m_gizmoOperation;
};

};