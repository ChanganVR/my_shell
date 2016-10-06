#include "JobManager.h"
#include <unistd.h>
#include <iostream>
#include <signal.h>

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
    errfile = STDERR_FILENO;
    job_list_.push_back(job);
    infile = STDIN_FILENO;
    for(auto process = job.process_list.begin(); process != job.process_list.end(); process++)
    {
        if(process == std::prev(job.process_list.end()))
            outfile = STDOUT_FILENO;
        else
        {
            if (pipe(fd) == -1)
            {
                perror("pipe");
                exit(1);
            } else
                outfile = fd[WRITE_END];
        }

        pid_t  pid = fork();
        if(pid == 0)//child process
            launch_process(*process, job.pgid, infile, outfile, errfile, job.foreground);
        else if (pid < 0)
        {
            perror("fork");
            exit(1);
        }
        else//parent process
        {
            (*process).pid = pid;
            if(job.pgid == 0)
                setpgid(pid, (*process).pid);
            if(infile != STDIN_FILENO)
                close(infile);
            if(outfile != STDOUT_FILENO)
                close(outfile);
            //set input for next process
            infile = fd[READ_END];
        }
        if(job.foreground && process == job.process_list.begin())
            tcsetpgrp(STDIN_FILENO, job.pgid);
    }

}

void JobManager::launch_process(Process &process, pid_t pgid, int infile, int outfile, int errfile, bool foreground)
{
    //reset signals
    signal (SIGINT, SIG_DFL);
    signal (SIGQUIT, SIG_DFL);
    signal (SIGTSTP, SIG_DFL);
    signal (SIGTTIN, SIG_DFL);
    signal (SIGTTOU, SIG_DFL);
    signal (SIGCHLD, SIG_DFL);

    //set pgid if current process is in a pipe
    pid_t pid = getpid();
    if(pgid == 0)
        setpgid(pid, pgid);
    //since child process inherits from parent shell process, so STDIN_FILENO is terminal input
    if(foreground)
    {
        if(tcsetpgrp(STDIN_FILENO, pid) < 0)
        {
            perror("Setting child process foreground process group fails");
            //exit(1);
        }
    }

    //set stdin, stdout and stderr
    if(infile != STDIN_FILENO)
    {
        dup2(infile, STDIN_FILENO);
        close(infile);
    }
    if(outfile != STDOUT_FILENO)
    {
        dup2(outfile, STDOUT_FILENO);
        close(outfile);
    }
    if(errfile != STDERR_FILENO)
    {
        dup2(errfile, STDERR_FILENO);
        close(errfile);
    }

    //execute program
    execvp(process.name.c_str(), process.argv);
}
