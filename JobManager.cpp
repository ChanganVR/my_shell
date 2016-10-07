#include "JobManager.h"
#include <unistd.h>
#include <wait.h>
#include <assert.h>
#include <signal.h>
#include <termios.h>
#include <iostream>

extern struct termios terminal_backup;

void JobManager::exec_cd(string & dir)
{
    if(chdir(dir.c_str()) < 0)
    {
        perror("cd");
    }
#ifdef DEBUG_MODE
   char cwd[1024];
   if (getcwd(cwd, sizeof(cwd)) != NULL)
       cout << ("current directory:" + string(cwd)) <<endl;
   else
       perror("getcwd() error");
#endif
}

void JobManager::exec_bg()
{
    if(job_list_.size() == 0)
        cerr << "No such job now";
    else
    {
        shared_ptr<Job> job = *std::prev(job_list_.end());
        if(job->status == Stopped)
        {
            //send continue signal
            put_job_in_background(job, true);
        }
        else if(job->status == Running)
            cerr << "job already in background";
        else
            assert(1);
    }
}

void JobManager::exec_fg()
{
    if(job_list_.size() == 0)
        cerr << "No such job now";
    else
    {
        shared_ptr<Job> job = *std::prev(job_list_.end());
        if(job->status == Stopped)
            put_job_in_foreground(job, true);
        else if(job->status == Running)
            put_job_in_foreground(job, false);
        else
            assert(1);
    }
}

void JobManager::put_job_in_background(shared_ptr<Job> job, bool con)
{
    if(con)
    {
        if(kill(job->pgid, SIGCONT)<0)
            perror("kill(SIGCONT");
    }
}

void JobManager::put_job_in_foreground(shared_ptr<Job> job, bool con)
{
    tcsetpgrp(STDIN_FILENO, job->pgid);
    if(con)
    {
        if(kill(job->pgid, SIGCONT)<0)
            perror("kill(SIGCONT");
    }
    wait_for_job(job);
    pid_t  shell_pid = getpid();
    tcsetpgrp(STDIN_FILENO, shell_pid);
    tcsetattr (STDIN_FILENO, TCSADRAIN, &terminal_backup);
}


void JobManager::exec_jobs()
{
    //TODO:jobs with parameters
    int job_count = 1;
    for(auto job = job_list_.begin(); job != job_list_.end(); job++, job_count++)
    {
        string status_info;
        switch ((*job)->status)
        {
            case Stopped: status_info = "Stopped";
                break;
            case Running: status_info = "Stopped";
                break;
            case Terminated: status_info = "Terminated";
                break;
        }
        cout << "[" + to_string(job_count) + "]  " + status_info + "   " + (*job)->name <<endl;
    }
}

void JobManager::exec_exit()
{
    exit(EXIT_SUCCESS);
}

void JobManager::launch_job(shared_ptr<Job> job)
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
    for(auto process = job->process_list.begin(); process != job->process_list.end(); process++)
    {
        //internal command, don't fork(), infile for next process is still stdin
        if(check_internal_cmd((*process)))
            continue;
        //external command
        if (process == std::prev(job->process_list.end()))
            outfile = STDOUT_FILENO;
        else {
            if (pipe(fd) == -1) {
                perror("pipe");
                exit(1);
            } else
                outfile = fd[WRITE_END];
        }

        pid_t pid = fork();
        if (pid == 0)//child process
            launch_process(*process, job->pgid, infile, outfile, errfile, job->foreground);
        else if (pid < 0)
        {
            perror("fork");
            exit(1);
        } else//parent process
        {
            (*process)->pid = pid;
            if (job->pgid == 0)
                job->pgid = pid;
            setpgid(pid, job->pgid);
            if (infile != STDIN_FILENO)
                close(infile);
            if (outfile != STDOUT_FILENO)
                close(outfile);
            //set input for next process
            infile = fd[READ_END];
        }
        if (job->foreground)
            put_job_in_foreground(job, false);
        else
            put_job_in_background(job, false);
    }
}

void JobManager::launch_process(shared_ptr<Process> process, pid_t pgid, int infile, int outfile, int errfile,
                                bool foreground)
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
    execvp(process->name.c_str(), process->argv);
}

void JobManager::exec_pwd()
{
    char dir[1024];
    if (getcwd(dir, sizeof(dir)) != NULL)
        cout << ("current directory:" + string(dir)) <<endl;
    else
        perror("getcwd() error");
}

bool JobManager::check_internal_cmd(shared_ptr<Process> process)
{
    string dir(process->argv[0]);
    //TODO:check whether there is extra argv
    if((process)->name == "cd")
        exec_cd(dir);
    else if((process)->name == "exit")
        exec_exit();
    else if((process)->name == "jobs")
        exec_jobs();
    else if((process)->name == "bg")
        exec_bg();
    else if((process)->name == "fg")
        exec_fg();
    else if((process)->name == "pwd")
        exec_pwd();
    else
        return false;
    return true;
}

void JobManager::wait_for_job(shared_ptr<Job> job)
{
    int status;
    pid_t  pid;
    while(1)
    {
        //wait status change(stopped, terminated) of all child process, save status and return pid
        pid = waitpid(WAIT_ANY, &status, WUNTRACED);
        //stopping or terminating fg process is same as doing that with job, but need to update the information
        update_job_status(pid, status);
        //when current job stopped or terminated, return
        if(job->status == Stopped || job->status == Terminated)
            break;
    }
}

bool JobManager::update_job_status(pid_t pid, int status)
{
    for(auto job = job_list_.begin(); job != job_list_.end(); job++)
    {
        for (auto process = (*job)->process_list.begin(); process != (*job)->process_list.end(); process++)
        {
            if ((*process)->pid == pid)
            {
                if (WIFEXITED(status))
                {
                    //process normally terminated
                    (*process)->completed = true;
                    //if process is the last element of job, then job is terminated
                    if((*job)->process_list.size() != 0 && std::prev((*job)->process_list.end()) == process)
                        (*job)->status = Terminated;
                    else
                        (*job)->status = Running;
                    return true;
                }
                if (WIFSIGNALED(status))
                {
                    //terminated by a signal
                    (*process)->completed = false;
                    (*job)->status = Terminated;
                    cerr << (*job)->name + " is terminated by signal";
                    return true;
                }
                if (WIFSTOPPED(status))
                {
                    (*process)->completed = false;
                    (*job)->status = Stopped;
                    return true;
                }
            }
        }
    }
    //no process id is matched
    return false;
}

void JobManager::check_job_status()
{
    int status;
    pid_t pid;
    while(1)
    {
        //return if process stopped or terminated, or no process
        pid = waitpid (WAIT_ANY, &status, WUNTRACED|WNOHANG);
        if(!update_job_status(pid, status))//No job is updated
            break;
    }
}


