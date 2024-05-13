// ApiGatewayWrapper.cpp
#include "ApiGateway.hpp"
#include "ApiGatewayWrapper.h"
#include <vector>
#include <string>

extern "C" {

void* ApiGateway_create(const char* site, const char** regions, int num_regions, const char* accessKeyId, const char* accessKeySecret) {
    std::vector<std::string> region_vector(regions, regions + num_regions);
    ApiGateway* api = new ApiGateway(site, region_vector, accessKeyId, accessKeySecret);
    return static_cast<void*>(api);
}

void ApiGateway_destroy(void* api) {
    delete static_cast<ApiGateway*>(api);
}

void ApiGateway_start(void* api, int force) {
    static_cast <ApiGateway*>(api)->Start(force != 0);
}

void ApiGateway_shutdown(void* api) {
    static_cast <ApiGateway*>(api)->Shutdown();
}

const char* ApiGateway_send(void* api, const char* url) {
    std::string response = static_cast<ApiGateway*>(api)->Send(std::string(url));
    char* c_response = new char[response.length() + 1];
    std::strcpy(c_response, response.c_str());
    return c_response;
}

} // extern "C"
