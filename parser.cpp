#include "parser.h"
#include <cstring>
#include <iostream>
#include "JobManager.h"


void Parser::parse(string& cmd)
{
    //preprocess the cmd
	for (auto iter = cmd.begin(); iter != cmd.end();iter++)
	{
		if((*iter)== '&' || (*iter) == '|')
		{
			if(iter!=cmd.begin())
			{
				iter = cmd.insert(iter, ' ');
				iter++;
			}
			if(iter !=cmd.end())
				iter = cmd.insert(iter+1, ' ');
		}
	}
	if (cmd == "--help" || cmd == "help")
	{
		cout << "Usage: enter you command" << endl;
		return;
	}
	else
	{
		Job job;
		job.name = cmd;
		while(1)
		{
			Process process;
			process.name = get_token(cmd);
			if(job.name.empty())
				break;
			string next = get_argv(cmd, job.name, process.argv, process.argc);
			job.process_list.push_back(process);
			if(next == "&")
			{
				job.foreground = false;
				break;
			}
			else if(next == "|")
				continue;
			else if(next.empty())
			{
				job.foreground = true;
				break;
			}
			else
			 	throw(new ParseError("Error:There is some error in your command!"));
		}
		job_manager_.launch_job(job);
	}

}


string Parser::get_argv(string &cmd, string &exec_name, char **&argv, int &argc)
{
	string next = get_token(cmd);
	list<string> arg_list;
    //the executable name should be the first parameter
	arg_list.push_back(exec_name);
	if(next.empty() || next == "&" || next == "|")
	{
		argc = 1;
		argv = new char*[argc];
		argv[0] = new char[exec_name.size() + 1];
		std::strcpy(argv[0], exec_name.c_str());
		return next;
	}
	else
	{
		while( !next.empty() && next != "&" && next != "|")
		{
			arg_list.push_back(next);
			next = get_token(cmd);
		}
		argc = arg_list.size();
		argv = new char*[argc];
		int i = 0;
		for (auto str = arg_list.begin(); str != arg_list.end(); str++, i++)
		{
			argv[i] = new char[(*str).size() + 1];
			std::strcpy(argv[i], (*str).c_str());
		}
		return next;
	}

}

std::string Parser::get_token(string& cmd)
{
	trim_front(cmd);
	string token;
	int first_separator = cmd.find_first_of(" \t");
	if(first_separator == string::npos)
	{
		token = cmd;
		cmd.clear();
	}
	else
	{
		token = cmd.substr(0, first_separator);
		cmd = cmd.substr(first_separator);
	}
	return token;
}

void Parser::trim_front(string &cmd)
{
	for (auto iter = cmd.begin(); iter != cmd.end();)
	{
		if (*iter != ' ' && *iter !='\t')
			break;
		else
			iter = cmd.erase(iter);
	}

}
