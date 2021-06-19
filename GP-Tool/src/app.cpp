#include "gptool.h"

int main(void)
{
	GPTool *app = new GPTool();
	app->run();
	delete app;

	return EXIT_SUCCESS;
}
