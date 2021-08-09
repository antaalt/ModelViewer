#pragma once

#include "Editor.h"

namespace viewer {

class InfoEditor : public EditorWindow
{
public:
	void onRender(aka::World& world) override;
};

};