#include "gptool.h"

int main(int argc, char *argv[])
{
	GPTool *app = new GPTool();

	if (argc > 1)
	{
		fs::path arq(argv[1]);
		app->openMovie(arq);
	}

	app->run();
	delete app;

	return EXIT_SUCCESS;
}
