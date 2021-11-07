#include "gptool.h"

int main(int argc, char *argv[])
{
	fs::path exe(argv[0]);
	fs::current_path(exe.parent_path().parent_path());

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
