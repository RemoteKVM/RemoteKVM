#include "SubprocessHandler.h"
#include <string.h>

SubprocessHandler::SubprocessHandler(std::string vmId) : vmId(vmId) {
    setup_pipes();
    create_subprocess();
};

SubprocessHandler::~SubprocessHandler() {
    try
    {
        poweroff_subprocess();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    close(in_pipe[0]);
    close(in_pipe[1]);
    close(out_pipe[0]);
    close(out_pipe[1]);
    
    close_subprocess(pid);
}

pid_t SubprocessHandler::create_subprocess() {
    pid = fork();
    if (pid == 0) { // Child process
        close(in_pipe[1]);
        close(out_pipe[0]);
        
        dup2(in_pipe[0], STDIN_FILENO);
        dup2(out_pipe[1], STDOUT_FILENO);

        // Construct the path to the VM-specific ext4 file
        std::string ext4Path = "./hypervisor/vmStorage/" + vmId + ".ext4";
        
        execlp("sudo", "sudo", "./hypervisor/main", "./hypervisor/bzImage", ext4Path.c_str(), "./hypervisor/rootfs.cpio", NULL);
        perror("execlp failed");
        exit(1);
    }
    else if (pid > 0) { // Parent
        std::cout << "Parent = Subprocess created with PID: " << pid << std::endl;
        close(in_pipe[0]);
        close(out_pipe[1]);
        return pid;
    }
    else {
        perror("fork failed");
        exit(1);
    }
}

void SubprocessHandler::setup_pipes() {
    pipe(in_pipe);  // Parent writes, subprocess reads
    pipe(out_pipe); // Subprocess writes, parent reads
}

void SubprocessHandler::close_subprocess(pid_t pid) {
    kill(pid, SIGKILL);
    waitpid(pid, NULL, 0);
}

void SubprocessHandler::poweroff_subprocess() {
    // we send a halt command to the hypervisor
    write(in_pipe[1], SYSTEM_HALT_COMMAND, sizeof(SYSTEM_HALT_COMMAND));
    // we wait for the hypervisor to finish. we wait untill the "reboot: System halted" message is printed.
    size_t total_bytes_read = 0;
    char buffer[1024];
    while (true) {
        size_t bytes_read = read(out_pipe[0], buffer + total_bytes_read, sizeof(buffer) - 1);
        total_bytes_read += bytes_read;
        if (bytes_read > 0) {
            buffer[total_bytes_read] = '\0';
            if (strstr(buffer, SYSTEM_HALT_MESSAGE) != NULL) {
                return;
            }
        }
    }
}
