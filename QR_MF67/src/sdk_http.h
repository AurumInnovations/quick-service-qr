#pragma once

void sdk_http_test();
void sdk_https_test();
void sdk_ota_update(const char *url);

typedef enum {
	SOCK_INDEX0,
	SOCK_INDEX1,
}SOCK_INDEX;

