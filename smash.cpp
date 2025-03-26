#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "Commands.h"
#include "signals.h"

int main(int argc, char* argv[]) {
    if(signal(SIGINT , ctrlCHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }

    struct sigaction action;
    action.sa_handler = &alarmHandler;
    action.sa_flags = SA_RESTART;

    if(sigaction(SIGALRM, &action, nullptr) < 0)
    {
        perror("smash error: failed to set SIGALARM handler");
    }

    SmallShell& smash = SmallShell::getInstance();
 
    while(true) {
        std::cout <<smash.getMyPrompt()<<"> ";
        std::string cmd_line;
        std::getline(std::cin, cmd_line);
        smash.executeCommand(cmd_line.c_str());
    }
    return 0;
}