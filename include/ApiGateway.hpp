// ApiGateway.hpp
#ifndef API_GATEWAY_H
#define API_GATEWAY_H

#include <string>
#include <vector>
#include <algorithm>
#include <aws/core/Aws.h>
#include <aws/apigateway/APIGatewayClient.h>


typedef struct {
    std::string apiId;
    std::string region;
    std::string endpoint;
} Gateway;

class GatewayManager {

private:
    std::vector<Gateway> gateways;
    size_t lastIndex = 0;

public:
    void AddGateway(const Gateway& gateway) {
        gateways.push_back(gateway);
    }

    void DeleteGateway(const Gateway& gateway) {
        auto it = std::find_if(gateways.begin(), gateways.end(),
                [&gateway](const Gateway& g) {
                    return g.apiId == gateway.apiId && g.region == gateway.region && g.endpoint == gateway.endpoint;
                });

        if (it != gateways.end()) {
            gateways.erase(it);
            if (gateways.empty()) {
                lastIndex = 0;
            } else if (lastIndex >= gateways.size()) {
                lastIndex = 0;
            }
        }
    }

    Gateway GetNextGateway() {
        if (gateways.empty()) {
            throw std::runtime_error("No gateways available.");
        }
        const Gateway& chosenGateway = gateways[lastIndex];
        lastIndex = (lastIndex + 1) % gateways.size();
        return chosenGateway;
    }

    bool IsEmpty() const {
        return gateways.empty();
    }

};

class ApiGateway {

public:
    ApiGateway(const std::string& site, const std::vector<std::string>& regions,
                       const std::string& accessKeyId, const std::string& accessKeySecret);
    ~ApiGateway();

    void Start(bool force = false);
    void Shutdown();
    std::string Send(const std::string& url);
    GatewayManager manager;

private:
    std::string site;
    std::vector<std::string> regions;

    bool InitGateway(const std::string& region, bool force);
    bool DeleteGateway(const Gateway& gateway);
    std::vector<std::string> GetGateways();
    std::string GenerateRandomIP();

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
    static std::string ApiIdToEndpointUrl(std::string apiId, std::string region);
    static std::string ExtractUrlPath(const std::string& url);
};

#endif // API_GATEWAY_H
