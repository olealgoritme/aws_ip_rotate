// ApiGatewayWrapper.h
#ifndef API_GATEWAY_WRAPPER_H
#define API_GATEWAY_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

void* ApiGateway_create(const char* site, const char** regions, int num_regions, const char* accessKeyId, const char* accessKeySecret);
void ApiGateway_destroy(void* api);
void ApiGateway_start(void* api, int force);
void ApiGateway_shutdown(void* api);
const char* ApiGateway_send(void* api, const char* url);

#ifdef __cplusplus
}
#endif

#endif // API_GATEWAY_WRAPPER_H
