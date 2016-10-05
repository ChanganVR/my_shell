#include "parser.h"
#include <iostream>

using namespace std;

int main()
{
	//initial the shell
	//check wheher interactive

	JobManager job_manager;
	Parser parser(job_manager);
	bool running = true;
	while (running)
	{
		string cmd;
		cout<<"Enter your command" << endl;
		getline(cin, cmd);
		try {
			parser.parse(cmd);
		}
		catch (ParseError& error) {
			cout << error.error_message << endl;
			continue;
		}
		catch (int i) {
			if (i == 1)
				running = false;
		}
	}
	return 0;
}
