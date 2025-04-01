#pragma once

#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <thread>

#define SYSTEM_HALT_COMMAND "halt\n"
#define SYSTEM_HALT_MESSAGE "reboot: System halted"

class SubprocessHandler
{
public:
    SubprocessHandler(std::string vmId);
    ~SubprocessHandler();
    pid_t create_subprocess();
    void setup_pipes();
    void close_subprocess(pid_t pid);
    void poweroff_subprocess();

    int in_pipe[2];
    int out_pipe[2];

private:
    pid_t pid;
    std::string vmId;

};