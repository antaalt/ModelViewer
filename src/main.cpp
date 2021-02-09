#include "ModelViewer.h"

struct Settings {
	uint32_t width;
	uint32_t height;
};

void parse(int argc, char* argv[], Settings& settings)
{
	if (argc < 2)
		return;

	for (int i = 1; i < argc; ++i)
	{
		if (strcmp(argv[i], "--help") == 0)
		{
			std::cout << std::endl << "Usage : " << std::endl;
			std::cout << "\t" << argv[0] << " [options]" << std::endl;
			std::cout << "Options are :" << std::endl;
			std::cout << "\t" << "--help                Print this message and exit.\n" << std::endl;
			std::cout << "\t" << "-w | --width  <int>   Render width (1280)." << std::endl;
			std::cout << "\t" << "-h | --height <int>   Render height (720)." << std::endl;
			std::cout << std::endl;
			return;
		}
		else if (strcmp(argv[i], "--width") == 0 || strcmp(argv[i], "-w") == 0)
		{
			if (i == argc - 1)
			{
				aka::Logger::warn("No arguments for width");
				return;
			}
			try {
				settings.width = (uint32_t)std::stoi(argv[++i]);
			} catch (const std::exception&) { aka::Logger::error("Could not parse integer for ", argv[i - 1]); }
		}
		else if (strcmp(argv[i], "--height") == 0 || strcmp(argv[i], "-h") == 0)
		{
			if (i == argc - 1)
			{
				aka::Logger::warn("No arguments for height");
				return;
			}
			try {
				settings.height = (uint32_t)std::stoi(argv[++i]);
			} catch (const std::exception&) { aka::Logger::error("Could not parse integer for ", argv[i - 1]); }
		}
	}
}

int main(int argc, char* argv[])
{
	Settings settings{};
	settings.width = 1280;
	settings.height = 720;

	parse(argc, argv, settings);
	
	viewer::Viewer viewer;
	aka::Config cfg{};
	cfg.app = &viewer;
	cfg.width = settings.width;
	cfg.height = settings.height;
	cfg.name = "Model Viewer";
	aka::Application::run(cfg);

	return 0;
}