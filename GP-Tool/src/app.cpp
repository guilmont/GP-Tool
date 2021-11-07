#include "gptool.h"

int main(int argc, char *argv[])
{
	std::cout << argv[0] << std::endl;
	fs::path exe(argv[0]);
	fs::path curr = exe.parent_path().parent_path();
	
	fs::current_path(curr);



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
