#pragma once

#include <Aka/Aka.h>

namespace app {

class EditorWindow 
{
public:
	virtual ~EditorWindow() {}
	virtual void onCreate(aka::World& world) {};
	virtual void onDestroy(aka::World& world) {};
	virtual void onUpdate(aka::World& world, aka::Time deltaTime) {};
	virtual void onRender(aka::World& world) {};
};

}