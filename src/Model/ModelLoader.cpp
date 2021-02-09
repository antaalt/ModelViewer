#include "ModelLoader.h"

#include <fstream>

namespace viewer {

std::string readString(const aka::Path& path)
{
	std::ifstream ifs(path.str());
	if (!ifs)
		throw std::runtime_error("Could not load file " + path.str());
	return std::string((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
}

};