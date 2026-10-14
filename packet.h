#pragma once
#include <stdint.h>
#include <sys/_types/_size_t.h>

typedef struct {
	uint8_t* buffer; // Beginning of allocated memory
	uint8_t* data;   // Beginning of current packet
	size_t length;   // Number of valid bytes from data onward
	size_t capacity; // Total allocated bytes
} Packet;


typedef enum {
	NET_OK,
	NET_DROP,
	NET_ERR_INVALID,
	NET_ERR_NOMEM,
	NET_ERR_UNSUPPORTED,
	NET_ERR_NO_ROUTE
} NetResult;
