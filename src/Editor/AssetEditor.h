#pragma once

#include "Editor.h"

namespace viewer {

class AssetEditor : public EditorWindow
{
public:
	void onRender(aka::World& world) override;
};

};