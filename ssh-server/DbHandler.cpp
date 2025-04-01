#include "DbHandler.h"
#include <iostream>
#include <sstream>

DbHandler::DbHandler()
{
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

DbHandler::~DbHandler()
{
    curl_global_cleanup();
}

size_t DbHandler::WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s)
{
    size_t totalSize = size * nmemb;
    s->append((char*)contents, totalSize);
    return totalSize;
}

nlohmann::json DbHandler::makePostRequest(const std::string& endpoint, const nlohmann::json& data)
{
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if(curl) {
        std::string url = baseUrl + endpoint;
        std::string dataString = data.dump();

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, dataString.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            curl_easy_cleanup(curl);
            throw std::runtime_error("HTTP request failed with error: " + std::string(curl_easy_strerror(res)));
        }

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }

    try {
        return nlohmann::json::parse(readBuffer);
    } catch (nlohmann::json::parse_error& e) {
        std::cerr << "JSON parsing failed: " << e.what() << std::endl;
        throw std::runtime_error("Failed to parse API response: " + readBuffer);
    }
}

ApiResponse DbHandler::validateUser(const std::string& username, const std::string& password)
{
    ApiResponse response;
    
    try {
        nlohmann::json requestData = {
            {"username", username},
            {"password", password}
        };
        
        auto jsonResponse = makePostRequest("/api-ssh-server/validate-user", requestData);
        
        response.success = jsonResponse["success"];
        
        if (jsonResponse.contains("message")) {
            response.message = jsonResponse["message"];
        }
        
        if (response.success) {
            response.userId = jsonResponse["userId"];
            
            // Parse VMs
            if (jsonResponse.contains("vms") && jsonResponse["vms"].is_array()) {
                for (const auto& vm : jsonResponse["vms"]) {
                    VirtualMachine vmObj;
                    vmObj.id = vm["id"];
                    vmObj.name = vm["vm_name"];
                    vmObj.status = vm["status"];
                    vmObj.storageSize = vm["disk_size"];
                    response.vms.push_back(vmObj);
                }
            }
        }
    } catch (const std::exception& e) {
        response.success = false;
        response.message = std::string("Error: ") + e.what();
    }
    
    return response;
}

ApiResponse DbHandler::validateVm(const std::string& username, const std::string& password, const std::string& vmName)
{
    ApiResponse response;
    
    try {
        nlohmann::json requestData = {
            {"username", username},
            {"password", password},
            {"vmName", vmName}
        };
        
        auto jsonResponse = makePostRequest("/api-ssh-server/validate-vm", requestData);
        
        response.success = jsonResponse["success"];
        
        if (jsonResponse.contains("message")) {
            response.message = jsonResponse["message"];
        }
        
        if (response.success) {
            response.userId = jsonResponse["userId"];
            
            // Populate the VM object
            response.vm.id = jsonResponse["vmId"];
            response.vm.name = jsonResponse["vmName"];
            response.vm.status = jsonResponse["status"];
            response.vm.storageSize = jsonResponse["diskSize"];
        }
    } catch (const std::exception& e) {
        response.success = false;
        response.message = std::string("Error: ") + e.what();
    }
    
    return response;
}

ApiResponse DbHandler::validateTerminalToken(const std::string& username, int vmId, const std::string& token)
{
    ApiResponse response;
    
    try {
        nlohmann::json requestData = {
            {"username", username},
            {"vmId", vmId},
            {"token", token}
        };
        
        auto jsonResponse = makePostRequest("/api-ssh-server/validate-terminal-token", requestData);
        
        response.success = jsonResponse["success"];
        
        if (jsonResponse.contains("message")) {
            response.message = jsonResponse["message"];
        }
        
        if (jsonResponse.contains("valid")) {
            response.valid = jsonResponse["valid"];
        }
        
        if (response.success) {
            response.userId = jsonResponse["userId"];
            
            // Populate the VM object
            response.vm.id = jsonResponse["vmId"];
            response.vm.name = jsonResponse["vmName"];
            response.vm.status = jsonResponse["status"];
            response.vm.storageSize = jsonResponse["diskSize"];
        }
    } catch (const std::exception& e) {
        response.success = false;
        response.message = std::string("Error: ") + e.what();
    }
    
    return response;
}

ApiResponse DbHandler::updateVmStatus(int vmId, const std::string& newStatus)
{
    ApiResponse response;
    
    try {
        // Validate status value
        if (newStatus != "running" && newStatus != "stopped") {
            response.success = false;
            response.message = "Invalid status value. Must be 'running' or 'stopped'.";
            return response;
        }
        std::ifstream keyFile("SSH_SERVER_SECRET_KEY.key");
        if (!keyFile.is_open()) {
            throw std::runtime_error("Failed to open SSH_SERVER_SECRET_KEY.key file");
        }
        
        std::string secretKey;
        std::getline(keyFile, secretKey);
        keyFile.close();
        
        if (secretKey.empty()) {
            throw std::runtime_error("SSH_SERVER_SECRET_KEY.key file is empty");
        }

        nlohmann::json requestData = {
            {"vmId", vmId},
            {"newStatus", newStatus},
            {"secretKey", secretKey}
        };
        

        auto jsonResponse = makePostRequest("/api-ssh-server/update-vm-status", requestData);
        response.success = jsonResponse["success"];
        
        if (jsonResponse.contains("message")) {
            response.message = jsonResponse["message"];
        }
    } catch (const std::exception& e) {
        response.success = false;
        response.message = std::string("Error: ") + e.what();
    }
    
    return response;
}