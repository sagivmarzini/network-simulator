#pragma once
#include <stdint.h>

#define MAX_FRAME_SIZE 1518 // Ethernet frame, including FCS

typedef struct {
	uint8_t buffer[MAX_FRAME_SIZE]; // Beginning of allocated memory
	uint8_t* data;                  // Beginning of current packet
	int length;                     // Number of valid bytes from data onward
} Packet;


typedef enum {
	NET_OK,
	NET_DROP,
	NET_ERR_INVALID,     // Invalid arguments or malformed packet
	NET_ERR_NOMEM,       // Memory allocation failed
	NET_ERR_UNSUPPORTED, // Protocol or operation isn't supported by this implementation
	NET_ERR_NO_ROUTE     // No valid path to the destination was found
} NetResult;
