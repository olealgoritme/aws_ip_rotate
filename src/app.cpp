// app.cpp
#include <cstdlib>
#include <iostream>
#include <csignal>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>
#include <unistd.h>
#include "ApiGateway.hpp"

std::unique_ptr<ApiGateway> gateway;

static void cleanup() {
    std::cout << "Cleanup... " << std::endl;
    gateway->Shutdown();
}

static void signalHandler(int signal) {
    std::cout << "Received interrupt. \nCleaning up resources..." << std::endl;
    cleanup();
}

int main(int argc, char **argv) {
    std::signal(SIGINT, signalHandler);

    const auto accessKey = std::getenv("AWS_ACCESS_KEY_ID");
    const auto secretKey = std::getenv("AWS_SECRET_ACCESS_KEY");
    if (!accessKey || !secretKey) {
        std::cerr << "AWS environment variables not set." << std::endl;
        return EXIT_FAILURE;
    }

    std::vector<std::string> regions = { "eu-west-1", "eu-west-2" };

    gateway.reset(new ApiGateway("https://ifconfig.io", regions, accessKey, secretKey));
    gateway->Start();

    for (int i = 0; i < 4; ++i) {
        std::string result = gateway->Send("https://ifconfig.io/ip");
        if (!result.empty()) {
            std::cout << "Result: " << result << std::endl;
        }
    }

    gateway->Shutdown();
    return 0;
}
