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
	void launch_process(Process &process, pid_t pgid, int infile, int outfile, int errfile, bool foreground);
    bool check_internal_cmd(Process &process);
	void check_job_status();//To see whether some job is terminated
    void wait_for_job(Job &job);
	bool update_job_status(pid_t pid, int status);
	void put_job_in_background(Job& job, bool con);
	void put_job_in_foreground(Job& job, bool con);
	void exec_bg(Process &process);
	void exec_fg(Process &process);
	void exec_jobs(Process &process);
	void exec_exit();
	void exec_cd(Process &process);
	void exec_pwd();
};


#endif
