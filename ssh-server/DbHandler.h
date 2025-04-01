#pragma once

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <curl/curl.h>
#include "nlohmann/json.hpp"
#include <memory>
#include <functional>
#include "VirtualMachine.h"

// Structure for API responses
struct ApiResponse {
    bool success;
    std::string message;
    int userId = -1;
    bool valid = false;
    std::vector<VirtualMachine> vms;
    VirtualMachine vm;
};

class DbHandler
{
private:
    const std::string baseUrl = "https://remotekvm.online";
    
    // Helper function for making POST requests
    nlohmann::json makePostRequest(const std::string& endpoint, const nlohmann::json& data);
    
    // Helper function for handling curl responses
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s);

public:
    DbHandler();
    ~DbHandler();

    // 1. Validate user and get their VMs
    ApiResponse validateUser(const std::string& username, const std::string& password);
    
    // 2. Validate user and specific VM
    ApiResponse validateVm(const std::string& username, const std::string& password, const std::string& vmName);
    
    // 3. Validate terminal token for specific user and VM
    ApiResponse validateTerminalToken(const std::string& username, int vmId, const std::string& token);

    // 4. Update VM status (running/stopped)
    ApiResponse updateVmStatus(int vmId, const std::string& newStatus);
};