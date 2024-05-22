// ApiGateway.cpp
#include <random>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <future>
#include <chrono>
#include <thread>
#include <curl/curl.h>
#include <aws/core/utils/Outcome.h>
#include <aws/apigateway/model/CreateRestApiRequest.h>
#include <aws/apigateway/model/CreateRestApiResult.h>
#include <aws/apigateway/model/CreateResourceRequest.h>
#include <aws/apigateway/model/CreateResourceResult.h>
#include <aws/apigateway/model/PutMethodRequest.h>
#include <aws/apigateway/model/PutMethodResult.h>
#include <aws/apigateway/model/PutIntegrationRequest.h>
#include <aws/apigateway/model/PutIntegrationResult.h>
#include <aws/apigateway/model/CreateDeploymentRequest.h>
#include <aws/apigateway/model/CreateDeploymentResult.h>
#include <aws/apigateway/model/DeleteRestApiRequest.h>
#include <aws/apigateway/model/GetResourcesRequest.h>
#include <aws/apigateway/model/GetResourcesResult.h>
#include <aws/apigateway/model/GetRestApisRequest.h>
#include <aws/apigateway/model/GetRestApisResult.h>
#include <aws/core/auth/AWSCredentials.h>
#include "ApiGateway.hpp"

ApiGateway::ApiGateway(const std::string& site, const std::vector<std::string>& regions,
                       const std::string& accessKeyId, const std::string& accessKeySecret)
    : site(site), regions(regions) {
    Aws::SDKOptions options;
    Aws::InitAPI(options);
}

ApiGateway::~ApiGateway() {
}

void ApiGateway::Shutdown() {
    std::cout << "Initiating Gateway teardowns.." << std::endl;
    while (!manager.IsEmpty()) {
        Gateway gateway = manager.GetNextGateway();
        DeleteGateway(gateway);
        manager.DeleteGateway(gateway);
    }
    Aws::SDKOptions options;
    Aws::ShutdownAPI(options);
}

void ApiGateway::Start(bool force) {
    std::vector<std::future<bool>> futures;
    for (const auto& region : regions) {
        futures.push_back(std::async(std::launch::async, &ApiGateway::InitGateway, this, region, force));
    }

    for (auto& f : futures) {
        f.wait();
    }
}

bool ApiGateway::InitGateway(const std::string& region, bool force) {
    Aws::Client::ClientConfiguration clientConfig;
    clientConfig.region = region;
    Aws::APIGateway::APIGatewayClient apiClient(clientConfig);

    Aws::APIGateway::Model::GetRestApisRequest getApisRequest;
    auto getApisOutcome = apiClient.GetRestApis(getApisRequest);

    if (!getApisOutcome.IsSuccess()) {
        std::cerr << "Failed to get APIs: " << getApisOutcome.GetError().GetMessage() << std::endl;
        return false;
    }

    // Check if API already exists
    std::string existingApiId;
    for (const auto& item : getApisOutcome.GetResult().GetItems()) {
        if (item.GetName() == site + " - IP Rotate API") {
            existingApiId = item.GetId();
            if (!force) {
                std::string endpointUrl = ApiIdToEndpointUrl(existingApiId, region);
                std::cout << "API Gateway for region " << region << " already exists! URL: " << endpointUrl << std::endl;
                manager.AddGateway({ .apiId = existingApiId, .region = region, .endpoint = endpointUrl });
                return true;
            }
        }
    }

    // Create new API if not found or force is true
    Aws::APIGateway::Model::CreateRestApiRequest createApiRequest;
    createApiRequest.SetName(site + " - IP Rotate API");
    createApiRequest.SetDescription("API Gateway");

    auto createApiOutcome = apiClient.CreateRestApi(createApiRequest);
    if (!createApiOutcome.IsSuccess()) {
        std::cerr << "Failed to create API: " << createApiOutcome.GetError().GetMessage() << std::endl;
        return false;
    }

    auto apiId = createApiOutcome.GetResult().GetId();
    std::cout << "New API Created: " << apiId << std::endl;

    // Create a new resource
    Aws::APIGateway::Model::CreateResourceRequest resourceRequest;
    resourceRequest.SetRestApiId(apiId);
    resourceRequest.SetParentId(createApiOutcome.GetResult().GetRootResourceId());
    resourceRequest.SetPathPart("{proxy+}");

    auto resourceOutcome = apiClient.CreateResource(resourceRequest);
    if (!resourceOutcome.IsSuccess()) {
        std::cerr << "Failed to create resource: " << resourceOutcome.GetError().GetMessage() << std::endl;
        return false;
    }

    auto resourceId = resourceOutcome.GetResult().GetId();
    std::cout << "Resource Created: " << resourceId << std::endl;

    // Define a method for the resource
    Aws::APIGateway::Model::PutMethodRequest methodRequest;
    methodRequest.SetRestApiId(apiId);
    methodRequest.SetResourceId(resourceId);
    methodRequest.SetHttpMethod("ANY");
    methodRequest.SetAuthorizationType("NONE");

    // Set the request parameters
    Aws::Map<Aws::String, bool> requestParameters;
    requestParameters["method.request.path.proxy"] = true;
    requestParameters["method.request.header.X-My-X-Forwarded-For"] = true;
    methodRequest.SetRequestParameters(requestParameters);

    auto methodOutcome = apiClient.PutMethod(methodRequest);
    if (!methodOutcome.IsSuccess()) {
        std::cerr << "Failed to define method: " << methodOutcome.GetError().GetMessage() << std::endl;
        return false;
    }

    // Define the integration
    Aws::APIGateway::Model::PutIntegrationRequest integrationRequest;
    integrationRequest.SetRestApiId(apiId);
    integrationRequest.SetResourceId(resourceId);
    integrationRequest.SetType(Aws::APIGateway::Model::IntegrationType::HTTP_PROXY);
    integrationRequest.SetHttpMethod("ANY");
    integrationRequest.SetUri(site);
    integrationRequest.SetIntegrationHttpMethod("ANY");

    // Set the request parameters
    Aws::Map<Aws::String, Aws::String> integrationReqParams;
    integrationReqParams["integration.request.path.proxy"] = "method.request.path.proxy";
    integrationReqParams["integration.request.header.X-Forwarded-For"] = "method.request.header.X-My-X-Forwarded-For";
    integrationRequest.SetRequestParameters(integrationReqParams);

    auto integrationOutcome = apiClient.PutIntegration(integrationRequest);
    if (!integrationOutcome.IsSuccess()) {
        std::cerr << "Failed to set up integration: " << integrationOutcome.GetError().GetMessage() << std::endl;
        return false;
    }

    // Repeat
    apiClient.PutMethod(methodRequest);
    integrationRequest.SetUri(site + "/{proxy}");
    apiClient.PutIntegration(integrationRequest);

    // Deploy the API
    Aws::APIGateway::Model::CreateDeploymentRequest deploymentRequest;
    deploymentRequest.SetRestApiId(apiId);
    deploymentRequest.SetStageName("PROXY");

    auto deploymentOutcome = apiClient.CreateDeployment(deploymentRequest);
    if (!deploymentOutcome.IsSuccess()) {
        std::cerr << "Failed to deploy API: " << deploymentOutcome.GetError().GetMessage() << std::endl;
        return false;
    }

    // Construct the API endpoint URL
    std::string endpointUrl = ApiIdToEndpointUrl(apiId, region);
    std::cout << "API Gateway URL: " << endpointUrl << std::endl;
    manager.AddGateway({ .apiId = apiId, .region = region, .endpoint = endpointUrl });

    std::cout << "API Deployed Successfully" << std::endl;
    return true;
}

bool ApiGateway::DeleteGateway(const Gateway& gateway) {
    Aws::Client::ClientConfiguration clientConfig;
    clientConfig.region = gateway.region;
    Aws::APIGateway::APIGatewayClient client(clientConfig);

    Aws::APIGateway::Model::DeleteRestApiRequest deleteRequest;
    deleteRequest.SetRestApiId(gateway.apiId);

    const int maxAttempts = 5;
    const std::chrono::seconds retryDelay(30);

    for (int attempt = 1; attempt <= maxAttempts; ++attempt) {
        auto deleteOutcome = client.DeleteRestApi(deleteRequest);
        if (deleteOutcome.IsSuccess()) {
            std::cout << "Successfully deleted API Gateway with ID: " << gateway.apiId << std::endl;
            return true;
        } else {
            std::cerr << "Failed to delete API Gateway (attempt " << attempt << "): " << deleteOutcome.GetError().GetMessage() << std::endl;
            if (attempt < maxAttempts) {
                std::this_thread::sleep_for(retryDelay);
            }
        }
    }
    return false;
}

std::string ApiGateway::Send(const std::string& url) {
    if (manager.IsEmpty()) {
        std::cerr << "No API Gateways available." << std::endl;
        return "";
    }

    Gateway chosenGateway = manager.GetNextGateway();
    std::string urlPath = ExtractUrlPath(url);
    std::string modifiedUrl = "https://" + chosenGateway.endpoint + "/PROXY" + urlPath;

    CURL* curl = curl_easy_init();
    if (curl) {
        std::string readBuffer;
        struct curl_slist* headers = nullptr;

        // Set up CURL options
        curl_easy_setopt(curl, CURLOPT_URL, modifiedUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Gateway Client");

        // Generate and set a random X-Forwarded-For IP address
        std::string ip = GenerateRandomIP();
        headers = curl_slist_append(headers, ("X-My-X-Forwarded-For: " + ip).c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Perform the request
        CURLcode res = curl_easy_perform(curl);
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

       if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            return "";
        } else if (response_code != 200) {
            std::cerr << "HTTP Request failed for " << modifiedUrl << ", Status Code: " + std::to_string(response_code) << std::endl;
            return "";
        } else {
            std::cout << "API Gateway [" << chosenGateway.region << "] " << "REQ @ " << modifiedUrl << " Status: " << response_code << std::endl;
        }

        return readBuffer;
    } else {
        std::cerr << "Failed to initialize curl" << std::endl;
        return "";
    }
}

std::string ApiGateway::GenerateRandomIP() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    return std::to_string(dis(gen)) + "." + std::to_string(dis(gen)) + "." +
           std::to_string(dis(gen)) + "." + std::to_string(dis(gen));
}

std::string ApiGateway::ExtractUrlPath(const std::string& url) {
    size_t domainStart = url.find("://");
    if (domainStart == std::string::npos) {
        return "";
    }
    size_t pathStart = url.find("/", domainStart + 3);
    if (pathStart == std::string::npos) {
        return "";
    }
    return url.substr(pathStart);
}

size_t ApiGateway::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string ApiGateway::ApiIdToEndpointUrl(std::string apiId, std::string region) {
    std::string endpointUrl = apiId + ".execute-api." + region + ".amazonaws.com";
    return endpointUrl;
}
