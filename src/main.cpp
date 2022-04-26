#include "EditorApp.h"

#include <cstdlib>

struct Settings {
	uint32_t width;
	uint32_t height;
	aka::String directory;
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
		else if (strcmp(argv[i], "--directory") == 0 || strcmp(argv[i], "-d") == 0)
		{
			if (i == argc - 1)
			{
				aka::Logger::warn("No arguments for directory");
				return;
			}
			settings.directory = argv[++i];
		}
	}
}

// Pick a graphic API
aka::gfx::GraphicAPI pick()
{
#if   defined(AKA_USE_OPENGL)
	return aka::gfx::GraphicAPI::OpenGL3;
#elif defined(AKA_USE_D3D11)
	return aka::gfx::GraphicAPI::DirectX11;
#elif defined(AKA_USE_VULKAN)
	return aka::gfx::GraphicAPI::Vulkan;
#else
	return aka::gfx::GraphicAPI::None;
#endif
}

int main(int argc, char* argv[])
{
	Settings settings{};
	settings.width = 1280;
	settings.height = 720;
	settings.directory = "./";

	parse(argc, argv, settings);

	app::Editor editor;
	
	aka::Config cfg{};
	cfg.app = &editor;
	cfg.platform.name = "Aka editor";
	cfg.platform.width = settings.width;
	cfg.platform.height = settings.height;
	//cfg.graphic.flags = aka::GraphicFlag::None;
	cfg.graphic.api = pick();
	cfg.argc = argc;
	cfg.argv = argv;
	cfg.directory = settings.directory;
	aka::Application::run(cfg);
	return 0;
}