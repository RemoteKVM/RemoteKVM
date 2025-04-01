#pragma once

#include <string>
#include <vector>
#include <fstream>  // For std::ifstream
#include <stdexcept>  // For std::runtime_error
#include "VirtualMachine.h"
#include "ChannelsHandler.h"
#include "DbHandler.h"

class VmsHandler
{
public:
    VmsHandler(ChannelsHandler* channels_handler, DbHandler* dbHandler, int socket);
    ~VmsHandler();

    void setVms(std::vector<VirtualMachine> vms);
    void setSelectedVm(VirtualMachine selectedVm);
    std::string handleVmSelection();
    
private:
    ChannelsHandler* channels_handler;
    DbHandler* dbHandler;
    int socket;
    
    VirtualMachine selectedVm;
    std::vector<VirtualMachine> vms;

    void activateVmStorage(std::string vmId, int storageSize);

    std::string getUserSelection();
};