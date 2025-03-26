#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iomanip>
#include "Commands.h"
#include <algorithm>

using namespace std;

#if 0
#define FUNC_ENTRY() \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT() \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

string _ltrim(const std::string &s)
{
  size_t start = s.find_first_not_of(' ');
  return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string &s)
{
  size_t end = s.find_last_not_of(' ');
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string &s)
{
  return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char *cmd_line, char **args)
{
  FUNC_ENTRY()
  int i = 0;
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  for (std::string s; iss >> s;)
  {
    args[i] = (char *)malloc(s.length() + 1);
    memset(args[i], 0, s.length() + 1);
    strcpy(args[i], s.c_str());
    args[++i] = NULL;
  }
  return i;

  FUNC_EXIT()
}

bool _isBackgroundComamnd(const char *cmd_line)
{
  const string str(cmd_line);
  return str[str.find_last_not_of(' ')] == '&';
}

void _removeBackgroundSign(char *cmd_line)
{
  const string str(cmd_line);
  // find last character other than spaces
  unsigned int idx = str.find_last_not_of(' ');
  // if all characters are spaces then return
  if (idx == string::npos)
  {
    return;
  }
  // if the command line does not end with & then return
  if (cmd_line[idx] != '&')
  {
    return;
  }
  // replace the & (background sign) with space and then remove all tailing spaces.
  cmd_line[idx] = ' ';
  // truncate the command line string up to the last non-space character
  cmd_line[str.find_last_not_of(' ', idx) + 1] = 0;
}

// TODO: Add your implementation for classes in Commands.h
JobsList::JobEntry &JobsList::JobEntry::operator=(const JobsList::JobEntry &job_entry)
{
  if (this == &job_entry)
  {
    return *this;
  }
  job_id = job_entry.job_id;
  job_pid = job_entry.job_pid;
  job_cmd = job_entry.job_cmd;
  return *this;
}

Command::Command(const char *cmd_line) : cmd_line(cmd_line)
{
  argv = new char *[COMMAND_MAX_ARGS + 1];
  argc = _parseCommandLine(cmd_line, argv);
}

Command::~Command()
{
  delete[] argv;
}

void ChangePromptCommand::execute()
{
  SmallShell &shell = SmallShell::getInstance();
  string new_prompt = "smash";
  if (argc == 1)
  {
    shell.setMyPrompt(new_prompt);
  }
  if (argc > 1)
  {
    new_prompt = argv[1];
    shell.setMyPrompt(new_prompt);
  }
}

void ShowPidCommand::execute()
{
  SmallShell &shell = SmallShell::getInstance();
  std::cout << "smash pid is " << shell.getSmashPid() << std::endl;
}

void GetCurrDirCommand::execute()
{
  char *buffer = new char[FILENAME_MAX];
  if (!getcwd(buffer, FILENAME_MAX))
  {
    perror("smash error: getcwd failed");
    return;
  }
  std::cout << buffer << std::endl;
  delete[] buffer;
}

void ChangeDirCommand::execute()
{
  if (argc > 2)
  {
    std::cerr << "smash error: cd: too many arguments" << std::endl;
    return;
  }
  if (argc < 2)
  {
    return;
  }
  char *new_previous_path = new char[FILENAME_MAX];
  char *pre_previous_temp = *(SmallShell::getInstance().previous_path);
  if (!strcmp(argv[1], "-"))
  {
    if (*(SmallShell::getInstance().first_cd) == true)
    {
      std::cerr << "smash error: cd: OLDPWD not set" << std::endl;
      return;
    }
    if (!getcwd(new_previous_path, FILENAME_MAX))
    {
      perror("smash error: getcwd failed");
      return;
    }
    if (chdir(pre_previous_temp) < 0)
    {
      perror("smash error: chdir failed");
      return;
    }
  }
  else
  {
    if (!getcwd(new_previous_path, FILENAME_MAX))
    {
      perror("smash error: chdir failed");
      return;
    }
    if (chdir(argv[1]) < 0)
    {
      perror("smash error: chdir failed");
      return;
    }
  }

  delete[] SmallShell::getInstance().previous_path;
  SmallShell::getInstance().previous_path = new char *[FILENAME_MAX];
  *(SmallShell::getInstance().previous_path) = new_previous_path;
  *(SmallShell::getInstance().first_cd) = false;
}

void JobsList::addJob(Command *cmd, pid_t pid)
{
  removeFinishedJobs();
  ++max_job_id;
  JobEntry *to_add = new JobEntry(pid, max_job_id, cmd);
  if (to_add)
  {
    jobs.push_back(to_add);
  }
}

void JobsList::printJobsList()
{
  removeFinishedJobs();
  if (jobs.empty())
  {
    return;
  }
  for (JobEntry *j : jobs)
  {
    if (j)
    {
      std::cout << "[" << j->job_id << "] " << j->job_cmd->cmd_line << std::endl;
    }
  }
}

void JobsList::killAllJobs()
{
  removeFinishedJobs();
  for (JobEntry *j : jobs)
  {
    if (kill(j->job_pid, SIGKILL) == -1)
    {
      perror("smash error: kill failed");
      return;
    }
  }
  if (!jobs.empty())
  {
    jobs.clear();
  }
  max_job_id = 0;
}

void JobsList::removeFinishedJobs()
{
  for (JobEntry *job : jobs)
  {
    int waitResult = waitpid(job->job_pid, nullptr, WNOHANG);
    if (waitResult < 0)
    {
      perror("smash error: waitpid failed");
      return;
    }
    if (waitResult != 0)
    {
      std::vector<JobEntry *>::iterator to_remove = std::find(jobs.begin(), jobs.end(), job);
      jobs.erase(to_remove);
    }
  }

  if (jobs.empty())
  {
    max_job_id = 0;
  }
  else
  {
    max_job_id = jobs[jobs.size() - 1]->job_id;
  }
}

JobsList::JobEntry *JobsList::getJobById(int jobId)
{
  for (JobEntry *j : jobs)
  {
    if (j->job_id == jobId)
    {
      return j;
    }
  }
  return nullptr;
}

JobsList::JobEntry *JobsList::getJobByPId(int jobPID)
{
  for (JobEntry *j : jobs)
  {
    if (j->job_pid == jobPID)
    {
      return j;
    }
  }
  return nullptr;
}

void JobsList::removeJobById(int jobId)
{
  std::vector<JobEntry *>::iterator job_iterator = jobs.begin();
  for (JobEntry *j : jobs)
  {
    if (j->job_id == jobId)
    {
      jobs.erase(job_iterator);
      return;
    }
    job_iterator++;
  }
  if (jobs.size() == 0)
  {
    max_job_id = 0;
  }
  else
  {
    max_job_id = jobs[jobs.size() - 1]->job_id;
  }
}

JobsList::JobEntry *JobsList::getLastJob(int *lastJobId)
{
  return this->getJobById(max_job_id);
}

bool JobsList::jobsListIsEmpty() const
{
  return jobs.empty();
}

int JobsList::getMaximalJobID() const
{
  return max_job_id;
}

int JobsList::getNumOfJobs() const
{
  return jobs.size();
}

std::vector<JobsList::JobEntry *> JobsList::getAllJobs() const
{
  return jobs;
}

void JobsCommand::add_Job(ExternalCommand *cmd, pid_t pid)
{
  jobs->addJob(cmd, pid);
}

void JobsCommand::execute()
{
  SmallShell &shell = SmallShell::getInstance();
  shell.jobs->removeFinishedJobs();
  shell.jobs->printJobsList();
}

void ForegroundCommand::execute()
{
  jobs->removeFinishedJobs();
  int id;
  SmallShell &shell = SmallShell::getInstance();
  // the job with the specified id in argv[1]
  if (argc >= 2)
  {
    if (argv[1][0] == '-')
    {
      for (int i = 1; argv[1][i] != '\0'; i++)
      {
        if (!isdigit(argv[1][i]))
        {
          std::cerr << "smash error: kill: invalid arguments" << std::endl;
          return;
        }
      }
      id = stoi(argv[1]);
      std::cerr << "smash error: fg: job-id " << id << " does not exist" << std::endl;
      return;
    }
    for (char &c : std::string(argv[1]))
    {
      if (!std::isdigit(c))
      {
        std::cerr << "smash error: fg: invalid arguments" << std::endl;
        return;
      }
    }
    id = stoi(argv[1]);
  }

  // the job with maximal job_id to the foreground
  if (argc == 1)
  {
    if (jobs->jobsListIsEmpty())
    {
      std::cerr << "smash error: fg: jobs list is empty" << std::endl;
      return;
    }
    id = jobs->getMaximalJobID();
  }
  if (id == 0)
  {
    std::cerr << "smash error: fg: job-id " << id << " does not exist" << std::endl;
    return;
  }
  /*if(jobs->jobsListIsEmpty() && !this->argv[1]){
    std::cerr<<"smash error: fg: jobs list is empty"<<std::endl;
    return;
  }*/
  JobsList::JobEntry *jobToMove = jobs->getJobById(id);
  if (!jobToMove)
  {
    std::cerr << "smash error: fg: job-id " << id << " does not exist" << std::endl;
    return;
  }
  if (argc > 2)
  {
    std::cerr << "smash error: fg: invalid arguments" << std::endl;
    return;
  }

  pid_t jobToMove_pid = jobToMove->job_pid;
  const Command *jobToMove_cmd = jobToMove->job_cmd;
  std::cout << jobToMove_cmd->cmd_line << " " << jobToMove_pid << std::endl;
  shell.setCurrentCMDLine(std::string(jobToMove_cmd->cmd_line));
  shell.setCurrentJobPID(jobToMove_pid);
  if (waitpid(jobToMove_pid, nullptr, WUNTRACED) == -1)
  {
    perror("smash error: waitpid failed");
    return;
  }
  jobs->removeJobById(jobToMove->job_id);
  shell.setCurrentJobPID(-1);
}

void QuitCommand::execute()
{
  if ((argc > 1) && (strcmp("kill", argv[1]) == 0))
  {
    jobs->removeFinishedJobs();
    std::cout << "smash: sending SIGKILL signal to " << jobs->getNumOfJobs() << " jobs:" << std::endl;
    for (JobsList::JobEntry *j : jobs->getAllJobs())
    {
      std::cout << j->job_pid << ": " << j->job_cmd->cmd_line << std::endl;
    }
    jobs->killAllJobs();
  }
  exit(0);
}

void KillCommand::execute()
{
  JobsList::JobEntry *job;
  /*if(argc>1){
    for(char& c : std::string(argv[2])){
      if(!std::isdigit(c)){
        std::cerr<<"smash error: kill: job-id "<<argv[2]<<" does not exist"<<std::endl;
        return;
      }
    }
  }*/
  int job_id;
  if (argc >= 3)
  {
    // jobid is number
    if (argv[2][0] == '-')
    {
      for (int i = 1; argv[2][i] != '\0'; i++)
      {
        if (!isdigit(argv[2][i]))
        {
          std::cerr << "smash error: kill: invalid arguments" << std::endl;
          return;
        }
      }
      job_id = stoi(argv[2]);
      std::cerr << "smash error: kill: job-id " << job_id << " does not exist" << std::endl;
      return;
    }
    for (char &c : std::string(argv[2]))
    {
      if (!std::isdigit(c))
      {
        std::cerr << "smash error: kill: invalid arguments" << std::endl;
        return;
      }
    }
    job_id = stoi(argv[2]);
    job = jobs->getJobById(job_id);
    if (!job)
    {
      std::cerr << "smash error: kill: job-id " << job_id << " does not exist" << std::endl;
      return;
    }
  }
  if (argc != 3 || argv[1][0] != '-')
  {
    std::cerr << "smash error: kill: invalid arguments" << std::endl;
    return;
  }
  // signum is number
  int i = 1;
  while (argv[1][i] != '\0')
  {
    if (!isdigit(argv[1][i]))
    {
      std::cerr << "smash error: kill: invalid arguments" << std::endl;
      return;
    }
    i++;
  }
  int signal = stoi((argv[1])) * (-1);
  pid_t jobPID = job->job_pid;
  if (kill(jobPID, signal) == -1)
  {
    perror("smash error: kill failed");
    return;
  }
  std::cout << "signal number " << signal << " was sent to pid " << jobPID << std::endl;
}

void ExternalCommand::execute()
{
  SmallShell &shell = SmallShell::getInstance();
  pid_t pid = fork();
  bool bg_flag = false;

  if (_isBackgroundComamnd(cmd_line.c_str()))
  {
    bg_flag = true;
  }
  if (pid == -1)
  {
    std::perror("smash error: fork failed");
    return;
  }

  if (pid == 0)
  { // child_mode
    if (setpgrp() == -1)
    {
      std::perror("smash error: setpgrp failed");
      return;
    }
    char external_cmd[COMMAND_ARGS_MAX_LENGTH + 1];
    strcpy(external_cmd, cmd_line.c_str());
    if ((cmd_line.find('*') != std::string::npos) || (cmd_line.find('?') != std::string::npos))
    { // COMPLEX

      if (bg_flag)
      {
        _removeBackgroundSign(external_cmd);
      }
      const string temp(external_cmd);
      strcpy(external_cmd, _trim(temp).c_str());
     
    //  char *args[] = {(char *)"/bin/bash", (char *)"-c", external_cmd, nullptr};
      if (execlp("/bin/bash","bin/bash","-c",external_cmd,nullptr) == -1)
      { 
        std::perror("smash error: execlp failed");
        return;
      }
    }
    else
    { // SIMPLE

      if (bg_flag)
      {
        _removeBackgroundSign(external_cmd);
      }
      const string temp(external_cmd);
      strcpy(external_cmd, _trim(temp).c_str());
      char *args[COMMAND_ARGS_MAX_LENGTH + 2] = {nullptr};

      _parseCommandLine(external_cmd, args);
      if (execvp(args[0], args) == -1)
      {
        std::perror("smash error: execvp failed");
        if (kill(pid, SIGKILL) == -1)
        {
          std::perror("smash error: kill failed");
        }
        return;
      }
    }
  }
  else
  { // parent_mode

    if (!bg_flag)
    {
      shell.setCurrentJobPID(pid);
      shell.setCurrentCMDLine(std::string(cmd_line));
      if (waitpid(pid, nullptr, WUNTRACED) == -1)
      {
        perror("smash error: waitpid failed");
        return;
      }
    }
    else
    {
      ExternalCommand *bg_command = new ExternalCommand(this->cmd_line.c_str(), jobs);
      if (bg_command)
      {
        shell.jobs->addJob(bg_command, pid);
        shell.setCurrentJobPID(-1);
      }
    }
  }
}

void RedirectionCommand::getData(std::string cmd)
{
  if (cmd.empty())
  {
    return;
  }

  if (cmd.find(">>") != std::string::npos)
  {
    *isAppend = true;
  }
  else
  {
    *isAppend = false;
  }

  int redirect_sign_start = cmd.find(">");
  std::string cmd_input = cmd.substr(0, redirect_sign_start);
  if (*isAppend)
  {
    redirect_sign_start += 2;
  }
  else
  {
    redirect_sign_start += 1;
  }
  std::string path_input = cmd.substr(redirect_sign_start, cmd.size() - redirect_sign_start + 1);
  my_cmd = _trim(cmd_input);
  my_file = _trim(path_input);
}

void RedirectionCommand::execute()
{
  SmallShell &smash = SmallShell::getInstance();
  char comm[cmd_line.length() + 1];
  strcpy(comm, cmd_line.c_str());
  _removeBackgroundSign(comm);
  char *splitedCommand[COMMAND_MAX_ARGS + 1];
  _parseCommandLine(comm, splitedCommand);
  string cmd, file_name;
  getData(cmd_line);//we get the command data here
  unsigned int count = 0;
  while (comm[count] != '>')
  {
    cmd.push_back(comm[count]);
    count++;
  }
  cmd = _trim(cmd);
  if (*isAppend == true)
  {
    count++;
  }
  count++;
  while (count < strlen(comm))
  {
    file_name.push_back(comm[count]);
    count++;
  }
  file_name = _trim(file_name);

  int fd;
  if (*isAppend == true)
  {
    fd = open(file_name.c_str(), O_APPEND | O_RDWR | O_CREAT, 0666);
  }
  else
  {
    fd = open(file_name.c_str(), O_TRUNC | O_RDWR | O_CREAT, 0666);
  }
  if (fd == -1)
  {
    perror("smash error: open failed");
    return;
  }

  int saved = dup(1);
  dup2(fd, 1);
  if (close(fd) == -1)
  {
    perror("smash error: close failed");
    return;
  }
  char to_exe[strlen(comm) + 1];
  strcpy(to_exe, cmd.c_str());
  Command *command = smash.CreateCommand(to_exe);
  command->execute();
  dup2(saved, 1); 
  close(saved);
  return;
}


void PipeCommand::execute(){
  size_t pipe_index = string(cmd_line).find('|');

    if (type == stdErr)
    {
      pipe_index = string(this->cmd_line).find("|&");
    }

    string cmd_splitted1 = string(cmd_line);
    string cmd_splitted2 = string(cmd_line);

    cmd1 = cmd_splitted1.substr(0, pipe_index);
    pipe_index++;
    if (type == stdErr)
    {
        pipe_index++;
    }

    string str_temp = cmd_splitted2.substr(pipe_index, cmd_splitted1.length() - pipe_index);
    str_temp = _trim(str_temp);
    str_temp += " ";
    cmd2 = str_temp.substr(0, str_temp.find_last_of(' '));
    SmallShell& shell = SmallShell::getInstance();
    int fd_pipe[2];

    if (pipe(fd_pipe) == -1)
    {
        std::perror("smash error: pipe failed");
        return;
    }

    pid_t firstChild_pid = fork();
    if (firstChild_pid == -1)
    {
        std::perror("smash error: fork failed");
        return;
    }

    if (firstChild_pid == 0)
    {
      if (setpgrp() == -1)
      {
          std::perror("smash error: setpgrp failed");
          return;
      }

      if (type == stdIn)
      {
        if (dup2(fd_pipe[1], 1) == -1) {
            std::perror("smash error: dup2 failed");
            return;
        }

        if (close(fd_pipe[0]) == -1) {
            std::perror("smash error: close failed");
            return;
        }

        if (close(fd_pipe[1]) == -1) {
            std::perror("smash error: close failed");
            return;
        }
      }
      else
      {
        if (dup2(fd_pipe[1], 2) == -1)
        {
            std::perror("smash error: dup2 failed");
            return;
        }

        if (close(fd_pipe[0]) == -1)
        {
            std::perror("smash error: close failed");
            return;
        }

        if (close(fd_pipe[1]) == -1)
        {
            std::perror("smash error: close failed");
            return;
        }
      }

      Command* pipe_command = shell.CreateCommand(this->cmd1.c_str());
      pipe_command->execute();
      exit(0);
    }

    pid_t secondChild_pid = fork();

    if (secondChild_pid == -1)
    {
        std::perror("smash error: fork failed");
        return;
    }

    if (secondChild_pid == 0)
    {
      if (setpgrp() == -1)
      {
          std::perror("smash error: setpgrp failed");
          return;
      }

      if (dup2(fd_pipe[0], 0) == -1)
      {
          std::perror("smash error: dup2 failed");
          return;
      }

      if (close(fd_pipe[0]) == -1)
      {
          std::perror("smash error: close failed");
          return;
      }

      if (close(fd_pipe[1]) == -1)
      {
          std::perror("smash error: close failed");
          return;
      }

      Command* my_command = shell.CreateCommand(cmd2.c_str());
      my_command->execute();
      exit(0);
  }

  if (close(fd_pipe[0]) == -1) {
      std::perror("smash error: close failed");
      return;
  }

  if (close(fd_pipe[1]) == -1) {
    std::perror("smash error: close failed");
    return;
  }

  if (waitpid(firstChild_pid, nullptr, WUNTRACED) == -1)
  {
    std::perror("smash error: waitpid failed");
    return;
  }

  if (waitpid(secondChild_pid, nullptr, WUNTRACED) == -1)
  {
    std::perror("smash error: waitpid failed");
    return;
  }
}


void ChmodCommand::execute()
{
  if (argc != 3)
  {
    std::cerr << "smash error: chmod: invalid arguments" << std::endl;
    return;
  }

  int int_ver_mode = stoi(argv[1]);
  if (int_ver_mode > 9999 || int_ver_mode < 0)
  {
    std::cerr << "smash error: chmod: invalid arguments" << std::endl;
    return;
  }
  else if (int_ver_mode % 10 > 7 || (int_ver_mode / 10) % 10 > 7 ||
           (int_ver_mode / 100) % 10 > 7 || (int_ver_mode / 1000) % 10 > 7)
  {
    std::cerr << "smash error: chmod: invalid arguments" << std::endl;
    return;
  }
  mode_t new_mode = (int_ver_mode % 10) + 8 * ((int_ver_mode / 10) % 10) + 64 * ((int_ver_mode / 100) % 10) + 512 * ((int_ver_mode / 1000) % 10);

  if (chmod(argv[2], new_mode) != 0)
  {
    std::cerr << "smash error: chmod: invalid arguments" << std::endl;
    return;
  }
}

void JobsList::finishedJobs() {
    SmallShell& shell = SmallShell::getInstance();
    pid_t pid_smash = getpid();

    if (pid_smash < 0) {
        std::perror("smash error: getpid failed");
        return;
    }

    if (pid_smash != shell.getSmashPid()) {
        return;
    }

    for (unsigned int i = 0; i < jobs.size(); i++)
    {
        JobsList::JobEntry* job = jobs[i];
        int waitStatus = waitpid(job->job_pid, nullptr, WNOHANG);
        if (waitStatus != 0) {
            jobs.erase(jobs.begin() + i);
        }
    }

    if (jobs.empty()) {
        max_job_id = 0;
    }
    else {
        max_job_id = jobs.back()->job_pid;
    }
}


void TimedJobs::removeKilledJobs()
{
    SmallShell& shell = SmallShell::getInstance();
    pid_t pid_smash = getpid();

    if (pid_smash != shell.getSmashPid())
    {
      return;
    }

    for (unsigned int i = 0; i < shell.timed_jobs->timeout_jobs.size(); ++i)
    {
      TimedJobEntry* timed_job = shell.timed_jobs->timeout_jobs[i];
      time_t curr_time = time(nullptr);

      if (timed_job->getEndTime() <= curr_time)
      {
        delete shell.timed_jobs->timeout_jobs[i];
        shell.timed_jobs->timeout_jobs.erase(shell.timed_jobs->timeout_jobs.begin() + i);
      }
    }
    shell.jobs->finishedJobs();
}

void TimedJobs::modifyJobByID(pid_t job_pid)
{
    int timed_jobs_number = timeout_jobs.size();
    if (timed_jobs_number >= 1) {

        timeout_jobs[timed_jobs_number - 1]->setPid(job_pid);
    }
    else
    {
        timeout_jobs[timed_jobs_number]->setPid(job_pid);
    }

    std::sort(timeout_jobs.begin(), timeout_jobs.end(), timeoutEntryIsBigger);
}

void TimeoutCommand::getDataTimeOutCommand(const char* cmd)
{
    if (!cmd) {
        return;
    }

    if (argc < 3)
    {
        error_type = TOO_FEW_ARGS;
        return;
    }

    std::string timeoutDuration = string(argv[1]);

    for (unsigned int index = 0; index < timeoutDuration.size(); index++)
    {
        if (timeoutDuration[index] < '0' || timeoutDuration[index] > '9')
        {
            error_type = TIMEOUT_NUMBER_IS_NOT_INTEGER;
            return;
        }
    }

    timer = atoi(argv[1]);
    string cmd_temp;

    for (int i = 2; i < argc; i++)
    {
        cmd_temp += argv[i];
        cmd_temp += " ";
    }

    command = _trim(cmd_temp);
}

void TimeoutCommand::execute()
{

    this->getDataTimeOutCommand(this->cmd_line.c_str());

    if (error_type == TIMEOUT_NUMBER_IS_NOT_INTEGER || error_type == TOO_FEW_ARGS)
    {
        std::cerr<<"smash error: timeout: invalid arguments"<<std::endl;
        return;
    }

    SmallShell& shell = SmallShell::getInstance();

    TimedJobEntry* current_timed_out = new TimedJobEntry(command, -1, time(nullptr), timer, false);
    shell.timed_jobs->getTimeoutJobs().push_back(current_timed_out);
    shell.setAlarmedJobs(true);
    char fg_cmd[COMMAND_ARGS_MAX_LENGTH];
    strcpy(fg_cmd, command.c_str());
    _removeBackgroundSign(fg_cmd);
    bool bg_flag = _isBackgroundComamnd(this->command.c_str());
    pid_t pid = fork();


    if (pid < 0)
    {
        std::perror("smash error: fork failed");
        return;
    }
    else if (pid == 0)
    {
        setpgrp();
        shell.executeCommand(fg_cmd);
        while (wait(nullptr) != -1) {}
        exit(0);
    }
    else
    {
      if (shell.getSmashPid() == getpid())
      {
        shell.setAlarmedJobs(true);
        shell.timed_jobs->modifyJobByID(pid);

        time_t current_time = time(nullptr);
        TimedJobEntry* alarm_entry = shell.timed_jobs->timeout_jobs[0];
        time_t end_time = alarm_entry->getEndTime();

        unsigned int seconds = end_time - current_time;
        alarm(seconds);

        if (bg_flag)
        {
          TimeoutCommand* copy_command = new TimeoutCommand(*this);
          shell.jobs->addJob(copy_command, pid);
        }
        else
        {
          if (waitpid(pid, nullptr, WUNTRACED) == -1)
          {
            std::perror("smash error: waitpid failed");
            return;
          }
        }
        shell.setAlarmedJobs(false);
      }
    }
    shell.setAlarmedJobs(false);
}




SmallShell::SmallShell() : my_prompt("smash")
{
  smash_pid = getpid();
  previous_path = new char *[FILENAME_MAX];
  *previous_path = new char[FILENAME_MAX];
  currentJob_pid = -1;
  jobs = new JobsList();
  timed_jobs = new TimedJobs();
  first_cd = new bool(true);
}

SmallShell::~SmallShell()
{
  delete[] *previous_path;
  delete[] previous_path;
}

/**
 * Creates and returns a pointer to Command class which matches the given command line (cmd_line)
 */
Command *SmallShell::CreateCommand(const char *cmd_line)
{
  if (!cmd_line)
  {
    return nullptr;
  }

  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

  if (cmd_s.find(">") != std::string::npos)
  {
    return new RedirectionCommand(cmd_line);
  }
  else if(cmd_s.find("|") != std::string::npos){
    return new PipeCommand(cmd_line, stdIn);
  }
  else if(cmd_s.find("|&") != std::string::npos){
    return new PipeCommand(cmd_line, stdErr);
  }
  else if (firstWord.compare("chprompt") == 0 || firstWord.compare("chprompt&") == 0)
  {
    if (_isBackgroundComamnd(cmd_line))
    {
      _removeBackgroundSign((char *)cmd_line);
    }
    return new ChangePromptCommand(cmd_line);
  }
  else if (firstWord.compare("showpid") == 0 || firstWord.compare("showpid&") == 0)
  {
    return new ShowPidCommand(cmd_line);
  }
  else if (firstWord.compare("pwd") == 0 || firstWord.compare("pwd&") == 0)
  {
    return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWord.compare("cd") == 0 || firstWord.compare("cd&") == 0)
  {
    if (_isBackgroundComamnd(cmd_line))
    {
      _removeBackgroundSign((char *)cmd_line);
    }
    return new ChangeDirCommand(cmd_line, this->previous_path);
  }
  else if (firstWord.compare("jobs") == 0 || firstWord.compare("jobs&") == 0)
  {
    return new JobsCommand(cmd_line, this->jobs);
  }
  else if (firstWord.compare("fg") == 0 || firstWord.compare("fg&") == 0)
  {
    if (_isBackgroundComamnd(cmd_line))
    {
      _removeBackgroundSign((char *)cmd_line);
    }
    return new ForegroundCommand(cmd_line, this->jobs);
  }
  else if (firstWord.compare("quit") == 0 || firstWord.compare("quit&") == 0)
  {
    if (_isBackgroundComamnd(cmd_line))
    {
      _removeBackgroundSign((char *)cmd_line);
    }
    return new QuitCommand(cmd_line, this->jobs);
  }
  else if (firstWord.compare("kill") == 0 || firstWord.compare("kill&") == 0)
  {
    if (_isBackgroundComamnd(cmd_line))
    {
      _removeBackgroundSign((char *)cmd_line);
    }
    return new KillCommand(cmd_line, this->jobs);
  }
  else if (firstWord.compare("chmod") == 0 || firstWord.compare("chmod&") == 0)
  {
    if (_isBackgroundComamnd(cmd_line))
    {
      _removeBackgroundSign((char *)cmd_line);
    }
    return new ChmodCommand(cmd_line);
  }
  else if (firstWord.compare("timeout") == 0 )
  {
    return new TimeoutCommand(cmd_line);
  }
  else
  { // external command
    return new ExternalCommand(cmd_line, this->jobs);
  }

  return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line)
{
  // TODO: Add your implementation here
  this->jobs->removeFinishedJobs();
  Command *cmd = CreateCommand(cmd_line);
  if (cmd)
  {
    this->curr_cmd_line = cmd_line;
    cmd->execute();
    delete cmd;
  }
  this->currentJob_pid = -1;
}

// Please note that you must fork smash process for some commands (e.g., external commands....)
