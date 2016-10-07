#include "parser.h"
#include <iostream>
#include <signal.h>
#include <unistd.h>
#include <termios.h>

using namespace std;

struct termios terminal_backup;

int main()
{
	//initial the shell
    //ignore the signal interrupting shell
    signal (SIGINT, SIG_IGN);
    signal (SIGQUIT, SIG_IGN);
    signal (SIGTSTP, SIG_IGN);
    signal (SIGTTIN, SIG_IGN);
    signal (SIGTTOU, SIG_IGN);
    signal (SIGCHLD, SIG_IGN);

	int shell_pid = getpid();
    cout << shell_pid <<endl;
	if(setpgid(shell_pid, shell_pid) < 0)
	{
		perror("set shell in its own process group");
	}
	//get control of the terminal
	if(tcsetpgrp(STDIN_FILENO, shell_pid) < 0)
	{
		cout << "tcsetpgrp(shell) fail" <<endl;
		perror("Setting shell foreground process group fails");
	}
	tcgetattr(STDIN_FILENO, &terminal_backup);
	//check whether interactive

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
