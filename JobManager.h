#ifndef JOBMANAGER_H
#define JOBMANAGER_H

#include <string>
#include <list>
#include <memory>

using namespace std;

enum Status{
	Stopped,
	Running,
   	Terminated//either from the "running" state by completing its execution or by explicitly being killed
};

#define WRITE_END 1
#define READ_END 0
#define DEBUG_MODE

typedef struct{
	int pid;
	string name;
	bool completed = false;
 	char** argv;
	int argc;
} Process;

typedef struct{
	int pgid = 0;
	string name;
	bool foreground;
	list<shared_ptr<Process>> process_list;
	Status status;
} Job;


class JobManager {
private:
	list<shared_ptr<Job>> job_list_;
public:
	JobManager() {}
	void launch_job(shared_ptr<Job> job);
	void launch_process(shared_ptr<Process> process, pid_t pgid, int infile, int outfile, int errfile, bool foreground);
    bool check_internal_cmd(shared_ptr<Process> process);
	void check_job_status();//To see whether some job is terminated
    void wait_for_job(shared_ptr<Job> job);
	bool update_job_status(pid_t pid, int status);
	void put_job_in_background(shared_ptr<Job> job, bool con);
	void put_job_in_foreground(shared_ptr<Job> job, bool con);
	void exec_bg();
	void exec_fg();
	void exec_jobs();
	void exec_exit();
	void exec_cd(string & dir);
	void exec_pwd();
};


#endif
