#ifndef JOBMANAGER_H
#define JOBMANAGER_H

#include <string>
#include <list>
#include <memory>

using namespace std;

enum Status{
	Stopped,
	Running,
	Done
};

typedef struct{
	int pid;
	string name;
 	char** argv;
	int argc;
} Process;

typedef struct{
	int gpid = 0;
	string name;
	bool foreground;
	list<Process> process_list;
	Status status;
} Job;


class JobManager {
private:
	list<Job> job_list_;
	shared_ptr<Job*> last_job;
public:
	JobManager() {}
	void launch_job(Job& job);
	void launch_process(Process& process, pid_t gpid, int infile, int outfile, int errfile);
	void exec_bg(string& cmd);
	void exec_fg(string& cmd);
	void exec_jobs(string& cmd);
	void exec_exit(string& cmd);
	void exec_cd(string& cmd);
};


#endif
