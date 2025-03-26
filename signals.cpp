#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"
#include <unistd.h>

using namespace std;

void ctrlCHandler(int sig_num) {
	SmallShell& shell = SmallShell::getInstance();
  std::cout<<"smash: got ctrl-C"<<std::endl;
  pid_t curr_job_pid = shell.getCurrentJobPID();

  if(curr_job_pid != -1){
    if(killpg(shell.getCurrentJobPID(), SIGKILL) == -1){
      perror("smash error: killpg failed");
      return;
    }
    JobsList::JobEntry* curr_job = shell.jobs->getJobByPId(curr_job_pid);
    if(curr_job){
      shell.jobs->removeJobById(curr_job->job_id);
    }
    std::cout<<"smash: process "<<curr_job_pid<<" was killed"<<std::endl;
    shell.setCurrentJobPID(-1);
  }

}

void alarmHandler(int sig_num)
{
    SmallShell& shell = SmallShell::getInstance();
    cout<<"smash: got an alarm"<<endl;

    TimedJobEntry* job_to_kill = shell.timed_jobs->timeout_jobs[0];

    pid_t pid_to_kill = job_to_kill->getPid();

    if(pid_to_kill == shell.getSmashPid())
    {
        return;
    }

    if (killpg(pid_to_kill, SIGKILL) == -1)
    {
        std::perror("smash error: killpg failed");
        return;
    }

    cout<<"smash: timeout "<< job_to_kill->getTimer() << " " << job_to_kill->getCommandLine() << " timed out!" << endl;
    shell.timed_jobs->removeKilledJobs();
    shell.jobs->finishedJobs();

    if(shell.timed_jobs->timeout_jobs.empty())
    {
        return;
    }

    TimedJobEntry* curr_alarmed_job = shell.timed_jobs->timeout_jobs[0];
    time_t curr_alarm = curr_alarmed_job->getEndTime();

    alarm(curr_alarm - time(nullptr));
}

