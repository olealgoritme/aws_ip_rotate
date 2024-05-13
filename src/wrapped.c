// wrapped.c
#include <stdio.h>
#include <stdlib.h>
#include "ApiGatewayWrapper.h"

int main() {

    const char* accessKey = getenv("AWS_ACCESS_KEY_ID");
    const char* secretKey = getenv("AWS_SECRET_ACCESS_KEY");

    if (!accessKey || !secretKey) {
        printf("AWS environment variables not set.\n");
        return EXIT_FAILURE;
    }

    const char *regions[] = {"eu-west-1", "eu-west-2"};
    int num_regions = 2;
    const char *result;

    void* api = ApiGateway_create("https://ifconfig.io", regions, num_regions, accessKey, secretKey);
    ApiGateway_start(api, 0);

    result = ApiGateway_send(api, "https://ifconfig.io/ip");
    printf("Result: %s\n", result);
    result = ApiGateway_send(api, "https://ifconfig.io/ip");
    printf("Result: %s\n", result);
    result = ApiGateway_send(api, "https://ifconfig.io/ip");
    printf("Result: %s\n", result);
    result = ApiGateway_send(api, "https://ifconfig.io/ip");
    printf("Result: %s\n", result);
    free((void *)result);

    ApiGateway_shutdown(api);
    return 0;
}
