#pragma once

#include "Model.h"

namespace viewer {

class ModelLoader
{
public:
	virtual Model::Ptr load(const Path& path) = 0;
};

};