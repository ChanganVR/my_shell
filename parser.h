#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "JobManager.h"
#include <list>
#include <string>

using namespace std;

class ParseError {
public:
	ParseError(string message):error_message(message){}
	string error_message;
};

class Parser {
private:
	list<string> cmd_list_;
	JobManager job_manager_;
public:
	Parser(JobManager & job_manager):job_manager_(job_manager){}
	void parse(string& cmd);
	void trim_front(string &cmd);
	string get_token(string& cmd);
	string get_argv(string &cmd, string &exec_name, char **&argv, int &argc);
};
#endif
