#pragma once

#include "../ModelLoader.h"

namespace viewer {

class OBJLoader : public ModelLoader
{
public:
	Model::Ptr load(const Path& path) override;
};

}