#pragma once

#include <Aka/Aka.h>

namespace app {

class SceneSystem :
	public aka::System
{
public:
	void onCreate(aka::World& world) override;
	void onDestroy(aka::World& world) override;

	void onUpdate(aka::World& world, aka::Time deltaTime) override;
};

};