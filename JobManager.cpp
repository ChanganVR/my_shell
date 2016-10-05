#include "JobManager.h"
#include <unistd.h>
#include <iostream>
#include <signal.h>
#include <unistd.h>


void JobManager::exec_cd(string & cmd)
{
    int ret = chdir(cmd.c_str());
}

void JobManager::exec_bg(string & cmd)
{
}

void JobManager::exec_fg(string & cmd)
{
}

void JobManager::exec_jobs(string & cmd)
{
}

void JobManager::exec_exit(string & cmd)
{
}

void JobManager::launch_job(Job& job)
{
    //parser test
//    for (auto iter =job.process_list.begin();iter!=job.process_list.end();iter++)
//    {
//        std::cout << (*iter).name << std::endl;
//        for (int i =0;i<(*iter).argc;i++)
//        {
//            std::cout << (*iter).argv[i] << std::endl;
//        }
//    }

    //fd[0] for reading, fd[1] for writing
    int fd[2], infile, outfile, errfile;
    errfile = stderr;
    job_list_.push_back(job);
    for(auto process = job.process_list.begin(); process != job.process_list.end(); process++)
    {
        if(process == job.process_list.begin())
            infile = stdin;
        else
        {
            if(pipe(fd) == -1)
            {
                perror("pipe");
                exit(1);
            }
            infile = fd[1];
        }
        if(process == job.process_list.end())
            outfile = stdout;
        else
            outfile = fd[0];

        pid_t  pid = fork();
        if(pid == 0)
            launch_process(*process, job.gpid, infile, outfile, errfile);
        else
        {

        }
    }

}

void JobManager::launch_process(Process &process, pid_t gpid, int infile, int outfile, int errfile)
{
    //reset signals
    signal (SIGINT, SIG_DFL);
    signal (SIGQUIT, SIG_DFL);
    signal (SIGTSTP, SIG_DFL);
    signal (SIGTTIN, SIG_DFL);
    signal (SIGTTOU, SIG_DFL);
    signal (SIGCHLD, SIG_DFL);

    pid_t pid = getpid();
    if(gpid == 0)
        setgpid(pid, gpid);
}
