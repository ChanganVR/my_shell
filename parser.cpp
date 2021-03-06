#include "parser.h"
#include <cstring>
#include <iostream>

void Parser::parse(string& cmd)
{
    if(cmd.empty())
        return;
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
		shared_ptr<Job> job(new Job);
		job->name = cmd;
		while(1)
		{
			shared_ptr<Process> process(new Process);
			process->name = get_token(cmd);
			if(job->name.empty())
				break;
			string next = get_argv(cmd, process->name, process->argv, process->argc);
			job->process_list.push_back(process);
			if(next == "&")
			{
				job->foreground = false;
                job->name.erase(job->name.begin() + job->name.find('&'));
				trim_end(job->name);
				break;
			}
			else if(next == "|")
				continue;
			else if(next.empty())
			{
				job->foreground = true;
				break;
			}
			else
			 	throw(new ParseError("Error:There is some error in your command!"));
		}
        //check whether there is some job status change and update the job list
		job_manager_.check_job_status();
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
		argv = new char*[argc + 1];
		argv[0] = new char[exec_name.size() + 1];
		std::strcpy(argv[0], exec_name.c_str());
        argv[1] = NULL;
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
		argv = new char*[argc + 1];
		int i = 0;
		for (auto str = arg_list.begin(); str != arg_list.end(); str++, i++)
		{
			argv[i] = new char[(*str).size() + 1];
			std::strcpy(argv[i], (*str).c_str());
		}
		argv[i] = NULL;
		return next;
	}

}

//get token in a string using separator blank
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

//wipe off the blanks in the head of a string
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

//wipe off the blanks at the end of a string
void Parser::trim_end(string &cmd)
{
	if(cmd.size() == 0)
		return;
	else
		for(auto iter = std::prev(cmd.end());iter >= cmd.begin(); iter--)
		{
			if(*iter == ' ')
				iter =cmd.erase(iter);
			else
				break;
		}
}
