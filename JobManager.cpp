#include "JobManager.h"
#include <unistd.h>
#include <wait.h>
#include <assert.h>
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

void JobManager::exec_bg(int job_num)
{
    shared_ptr<Job> job;
    if(job_num == 0)
    {
        //find the last not terminated job in job list
        auto last_not_terminated = job_list_.rbegin();
        for(; last_not_terminated != job_list_.rend(); last_not_terminated++)
        {
            if((*last_not_terminated)->status != Terminated)
                break;
        }
        //all jobs are terminated or no job in job list
        if(last_not_terminated == job_list_.rend())
        {
            cout << "No such job now";
            return;
        }
        job = *last_not_terminated;
    } else
    {
        auto iter = job_list_.begin();
        std::advance(iter, job_num - 1);
        job = *iter;
    }

    if(job->status == Stopped)
    {
        //send continue signal to stopped child process
        put_job_in_background(job, true);
    }
    else if(job->status == Running)
        cerr << "job already in background";
    else
        assert(1);
}

void JobManager::exec_fg(int job_num)
{
    shared_ptr<Job> job;
    if(job_num == 0)
    {
        //find the last not terminated job in job list
        auto last_not_terminated = job_list_.rbegin();
        for(; last_not_terminated != job_list_.rend(); last_not_terminated++)
        {
            if((*last_not_terminated)->status != Terminated)
                break;
        }
        //all jobs are terminated or no job in job list
        if(last_not_terminated == job_list_.rend())
        {
            cout << "No such job now";
            return;
        }
        job = *last_not_terminated;
    } else
    {
        auto iter = job_list_.begin();
        std::advance(iter, job_num - 1);
        job = *iter;
    }

    current_job_ = job;
    cout<< job->name <<endl;
    if(job->status == Stopped)
        put_job_in_foreground(job, true);
    else if(job->status == Running)
        put_job_in_foreground(job, false);
    else
        assert(1);

}

void JobManager::put_job_in_background(shared_ptr<Job> job, bool con)
{
    if(con)
    {
        if(kill(job->pgid, SIGCONT)<0)
            perror("kill(SIGCONT");
    }
    job->status = Running;
    job->foreground = false;
}

void JobManager::put_job_in_foreground(shared_ptr<Job> job, bool con)
{
    //set child process group id in terminal
    tcsetpgrp(STDIN_FILENO, job->pgid);
    if(con)
    {
        if(kill(job->pgid, SIGCONT)<0)
            perror("kill(SIGCONT");
    }
    job->status = Running;
    job->foreground = true;
    wait_for_job(job);
    pid_t  shell_pid = getpid();
    //reset shell process group id in terminal
    tcsetpgrp(STDIN_FILENO, shell_pid);
    tcsetattr (STDIN_FILENO, TCSADRAIN, &terminal_backup);
}


void JobManager::exec_jobs()
{
    //TODO:jobs with parameters
    int job_num = 1;
    for(auto job = job_list_.begin(); job != job_list_.end(); job++, job_num++)
    {
        if((*job)->status != Terminated)
            print_job_status(*job, job_num);
    }
}

void JobManager::exec_exit()
{
    exit(EXIT_SUCCESS);
}

void JobManager::launch_job(shared_ptr<Job> job)
{
    current_job_ = job;
    //internal cmd is used alone
    bool is_internal_cmd = check_internal_cmd(job);
    if(is_internal_cmd)
    {
        if(job->process_list.size() > 1)
            cerr << "internal commands should be used alone";
        exec_internal_cmd(*job->process_list.begin());
        return;
    }
    if(!job->foreground)//if job runs on background, add it to the bg job list
    {
        job_list_.push_back(job);
        cout << "\x1b[31m[" + to_string(job_list_.size()) + "]\t" + job->name + "\x1b[0m" << endl;
    }

    //initialize file descriptor
    int fd[2], infile, outfile, errfile;
    errfile = STDERR_FILENO;
    infile = STDIN_FILENO;
    //launch all processes in current and connect their pipes
    for(auto process = job->process_list.begin(); process != job->process_list.end(); process++)
    {
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
        {
            //only last process in a job can control the terminal
            if(process == std::prev(job->process_list.end()))
                launch_process(*process, job->pgid, infile, outfile, errfile, job->foreground);
            else
                launch_process(*process, job->pgid, infile, outfile, errfile, false);
        }
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
    }
    if (job->foreground)
        put_job_in_foreground(job, false);
    else
        put_job_in_background(job, false);
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
    cerr << process->name << ": command not found" << endl;
    exit(1);
}

void JobManager::exec_pwd()
{
    char dir[1024];
    if (getcwd(dir, sizeof(dir)) != NULL)
        cout << (string(dir)) <<endl;
    else
        perror("getcwd() error");
}

bool JobManager::exec_internal_cmd(shared_ptr<Process> process)
{
    //TODO:check whether there is extra argv
    if((process)->name == "cd")
    {
        if(process->argv[1] == NULL)
        {
            cerr << "path is not specified" << endl;
            return false;
        }
        else
        {
            string dir(process->argv[1]);
            exec_cd(dir);
        }
    }
    else if((process)->name == "exit")
        exec_exit();
    else if((process)->name == "jobs")
        exec_jobs();
    else if((process)->name == "bg")
    {
        int job_num;
        if(process->argv[1] == NULL)
            job_num = 0;
        else
            stoi(string(process->argv[1]));
        exec_bg(job_num);
    }
    else if((process)->name == "fg")
    {
        int job_num;
        if(process->argv[1] == NULL)
            job_num = 0;
        else
            stoi(string(process->argv[1]));
        exec_fg(job_num);
    }
    else if((process)->name == "pwd")
        exec_pwd();
    else
        false;
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
        int foreground_process = update_job_status(pid, status);
        //wait until current job stopped or terminated or one process in current job completed
        if(job->status == Stopped || job->status == Terminated || foreground_process)
            break;
    }
}

//return value: -1 means no status update, 0 means successful update, 1 means foreground process finish
int JobManager::update_job_status(pid_t pid, int status)
{
    int job_num = 1;
    //check whether current job is in the background job list
    bool current_in_background = false;
    for(auto job = job_list_.begin(); job != job_list_.end(); job++, job_num++)
    {
        if((*job)->pgid == current_job_->pgid)
            current_in_background = true;
    }
    //no child process to report, specified by WNOHANG
    if(pid == 0)
        return -1;
    else if(pid == -1)
    {
        perror("waitpid");
        return -1;
    }
    else if(!current_in_background && WIFSTOPPED(status))
    {
        job_list_.push_back(current_job_);
        current_job_->foreground = false;
        current_job_->status = Stopped;
        return 0;
    }
    job_num = 1;
    for(auto job = job_list_.begin(); job != job_list_.end(); job++, job_num++)
    {
        for (auto process = (*job)->process_list.begin(); process != (*job)->process_list.end(); process++)
        {
            if ((*process)->pid == pid)
            {
                if (WIFEXITED(status))
                {
                    //process normally terminated
                    (*process)->completed = true;
                    //if process is the last process of job, then job is terminated
                    if((*job)->process_list.size() != 0 && std::prev((*job)->process_list.end()) == process)
                    {
                        (*job)->status = Done;
                        print_job_status(*job, job_num);
                        (*job)->status = Terminated;
                    }
                    else
                        (*job)->status = Running;
                    return (*job)->pgid;
                }
                if (WIFSIGNALED(status))
                {
                    //terminated by a signal
                    (*process)->completed = false;
                    (*job)->status = Terminated;
                    print_job_status(*job, job_num);
//                    cerr << (*job)->name + " is terminated by signal";
                    return 0;
                }
                if (WIFSTOPPED(status))
                {
                    (*process)->completed = false;
                    (*job)->status = Stopped;
                    print_job_status(*job, job_num);
                    return 0;
                }
            }
        }
    }
    return 1;//foreground process, not in the list
}

void JobManager::check_job_status()
{
    int status;
    pid_t pid;
    while(1)
    {
        //return if process stopped or terminated, or no process
        pid = waitpid (WAIT_ANY, &status, WNOHANG|WUNTRACED);
        if(pid == -1)
            pid = 0; //the way WNOHANG returns
        if(update_job_status(pid, status) == -1)//No job is updated
            break;
    }
}

bool JobManager::check_internal_cmd(shared_ptr<Job> job)
{
    //check pure internal command, that is one job only has one internal command
    auto process = job->process_list.begin();
    if(job->process_list.size() == 1 && ((*process)->name == "cd" || (*process)->name == "exit" ||
        (*process)->name == "bg" || (*process)->name == "fg" || (*process)->name == "jobs" || (*process)->name == "pwd"))
        return true;
    else
        return false;
}

void JobManager::print_job_status(shared_ptr<Job> job, int job_num)
{
    //don't print when process is in foreground
    if(job->foreground && job->status != Stopped)
        return;
    string status_info;
    switch (job->status)
    {
        case Stopped: status_info = "Stopped";
            break;
        case Running: status_info = "Running";
            break;
        case Terminated: status_info = "Terminated";
            break;
        case Done: status_info = "Done";
            break;
    }
    string ending;
    if(job->foreground == false)
        ending = "&";
    cout << "\x1b[31m[" + to_string(job_num) + "]\t" + status_info + "\t\t\t" + job->name + ending + "\x1b[0m"<<endl;
}

