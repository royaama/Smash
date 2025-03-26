#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

class Command {
// TODO: Add your data members
public:
    std::string cmd_line;
    char** argv;
    int argc;
    Command(const char* cmd_line);
    virtual ~Command();
    virtual void execute() = 0;
    //virtual void prepare();
    //virtual void cleanup();
    // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
public:
    BuiltInCommand(const char* cmd_line) : Command(cmd_line) {}
    virtual ~BuiltInCommand() {}
};

class JobsList;
class ExternalCommand : public Command {
    JobsList* jobs;
public:
    ExternalCommand(const char* cmd_line,JobsList* jobs): Command(cmd_line), jobs(jobs){}
    virtual ~ExternalCommand() {}
    void execute() override;
};


enum pipeType{stdIn, stdErr};
class PipeCommand : public Command {
private:
    pipeType type;
    std::string cmd1;
    std::string cmd2;
public:
    PipeCommand(const char* cmd_line, pipeType type) : Command(cmd_line), type(type){}
    virtual ~PipeCommand() {}
    void execute() override;
};

class RedirectionCommand : public Command {
    // TODO: Add your data members
private:
    std::string my_cmd;
    std::string my_file;
    int dup_out;
    bool* isAppend;
    void getData(std::string cmd);
public:
    explicit RedirectionCommand(const char* cmd_line): Command(cmd_line), dup_out(-1){
        isAppend = new bool(true);
    }
    virtual ~RedirectionCommand() {}
    void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
// TODO: Add your data members
private:
    char** plastPwd;
public:
    ChangeDirCommand(const char* cmd_line,char** plastPwd) : BuiltInCommand(cmd_line), plastPwd(plastPwd){}
    virtual ~ChangeDirCommand() {}
    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
public:
    GetCurrDirCommand(const char* cmd_line): BuiltInCommand(cmd_line) {}
    virtual ~GetCurrDirCommand() {}
    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
public:
    ShowPidCommand(const char* cmd_line): BuiltInCommand(cmd_line) {}
    virtual ~ShowPidCommand() {}
    void execute() override;
};

class ChangePromptCommand : public BuiltInCommand {
public:
    ChangePromptCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}
    virtual ~ChangePromptCommand() {}
    void execute() override;
};

class JobsList;
class QuitCommand : public BuiltInCommand {
// TODO: Add your data members 
private:
    JobsList* jobs;
public:
    QuitCommand(const char* cmd_line, JobsList* jobs): BuiltInCommand(cmd_line), jobs(jobs){}
    virtual ~QuitCommand() {}
    void execute() override;
};




class JobsList {
public:
    class JobEntry {
        // TODO: Add your data members
    public:
        pid_t job_pid;
        int job_id;
        const Command* job_cmd;
        JobEntry(const pid_t& job_pid1,  const int& job_id2, const Command* job_cmd3) {
            job_pid = job_pid1;
            job_id = job_id2;
            job_cmd = job_cmd3;
        }
        ~JobEntry() = default;
        JobsList::JobEntry& operator=(const JobsList::JobEntry& job_entry);
    };
    // TODO: Add your data members

    JobsList()=default;
    ~JobsList()=default;
    void addJob(Command* cmd, pid_t pid);//we added the pid as an argument
    void printJobsList();
    void killAllJobs();
    void removeFinishedJobs();
    void finishedJobs();
    JobEntry * getJobById(int jobId);
    JobEntry * getJobByPId(int jobPID);
    void removeJobById(int jobId);
    JobEntry * getLastJob(int* lastJobId);
    //JobEntry *getLastStoppedJob(int *jobId);
    bool jobsListIsEmpty() const;
    int getMaximalJobID() const;
    int getNumOfJobs() const;
    std::vector<JobEntry*> getAllJobs() const;
    // TODO: Add extra methods or modify exisitng ones as needed
  
private:
    std::vector<JobEntry*> jobs;
    int max_job_id = 0;
};

class JobsCommand : public BuiltInCommand {
    // TODO: Add your data members
  private:
    JobsList* jobs;
public:
    JobsCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line), jobs(jobs){}
    virtual ~JobsCommand() {}
    void execute() override;
    void add_Job(ExternalCommand* cmd, pid_t pid);//we added the pid as an argument
};

class KillCommand : public BuiltInCommand {
    // TODO: Add your data members
    JobsList* jobs;
public:
    KillCommand(const char* cmd_line, JobsList* jobs): BuiltInCommand(cmd_line), jobs(jobs){}
    virtual ~KillCommand() {}
    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
  JobsList* jobs;
    // TODO: Add your data members
public:
    ForegroundCommand(const char* cmd_line, JobsList* jobs):BuiltInCommand(cmd_line), jobs(jobs){}
    virtual ~ForegroundCommand() {}
    void execute() override;
};

class ChmodCommand : public BuiltInCommand {
public:
    ChmodCommand(const char* cmd_line) : BuiltInCommand(cmd_line){}
    virtual ~ChmodCommand() {}
    void execute() override;
};


class TimedJobEntry {
private:
    std::string timed_cmd;
    pid_t pid;
    time_t start_time;
    time_t end_time;
    int timer;
    bool is_foreground;
    bool is_killed;

public:
    TimedJobEntry(std::string cmd, pid_t pid, time_t start_time, int timer, bool is_fg) : timed_cmd(cmd), pid(pid), start_time(start_time),
                                                        end_time(start_time + timer), timer(timer), is_foreground(is_fg), is_killed(false) {}
    ~TimedJobEntry() = default;
    time_t getEndTime() {
        return end_time; }
    pid_t getPid() {
        return pid; }
    void setPid(pid_t id) { 
        pid = id; }
    int getTimer() { 
        return timer; }
    std::string getCommandLine() { 
        return timed_cmd; }
};

class TimedJobs {
public:
    std::vector<TimedJobEntry*> timeout_jobs;

    TimedJobs() = default;
    ~TimedJobs() = default;

    static bool timeoutEntryIsBigger(TimedJobEntry* t1, TimedJobEntry* t2)
    {
        if ((t1->getEndTime()) >= (t2->getEndTime())) {
            return false;
        }
        return true;
    }
    std::vector<TimedJobEntry*>& getTimeoutJobs(){
        return timeout_jobs;
    }
    void removeKilledJobs();
    void modifyJobByID(pid_t job_pid);
};

enum TimeOutErrorType {SUCCESS, TOO_FEW_ARGS, TIMEOUT_NUMBER_IS_NOT_INTEGER};

class TimeoutCommand : public BuiltInCommand {
    /* Bonus */
    int timer;
    TimeOutErrorType error_type;
    std::string command;

    void getDataTimeOutCommand(const char* cmd);
public:
    explicit TimeoutCommand(const char* cmd_line): BuiltInCommand(cmd_line), timer(0), error_type(SUCCESS){}
    virtual ~TimeoutCommand() {}
    void execute() override;
};



class SmallShell {
private:
    // TODO: Add your data members
    std::string my_prompt;
    //std::string previous_path;
    std::string curr_cmd_line;
    //std::string full_path;
    pid_t smash_pid;
    pid_t currentJob_pid;
    bool isAlarmedHandling;//for timeout command
    SmallShell();
public:
    Command *CreateCommand(const char* cmd_line);
    SmallShell(SmallShell const&)      = delete; // disable copy ctor
    void operator=(SmallShell const&)  = delete; // disable = operator
    static SmallShell& getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }
    ~SmallShell();
    void executeCommand(const char* cmd_line);
    // TODO: add extra methods as needed
    std::string getMyPrompt() const{
      return my_prompt;
    }
    void setMyPrompt(std::string& new_prompt){
        my_prompt = new_prompt;
    }
    pid_t getSmashPid() const{
        return smash_pid;
    }
    void setCurrentCMDLine(std::string new_cmd_line){
      curr_cmd_line = new_cmd_line;
    }
    void setCurrentJobPID(pid_t new_job_pid){
      currentJob_pid = new_job_pid;
    }
    pid_t getCurrentJobPID() const{
      return currentJob_pid;
    }
    /* std::string getFullPath() const{
       return full_path;
     }
     void setFullPath(std::string new_path){
       full_path = new_path;
     }
    */ char* getPreviousPath() const{
       return *previous_path;
     }
     void setPreviousPath(char** old){
       previous_path = old;
     }
    char** previous_path;//use in cd command
    JobsList* jobs;
    bool* first_cd;
    //for timeout command
    TimedJobs* timed_jobs;
    bool isAlarmedJobs() { return isAlarmedHandling; }
    void setAlarmedJobs(bool is_alarmed) {isAlarmedHandling= is_alarmed; }

};

#endif //SMASH_COMMAND_H_
