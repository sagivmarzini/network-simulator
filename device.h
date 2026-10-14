#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_MTU 1500 // Maximum Transmission Unit - the max size of a packet (in bytes) that a network device will accept



#define WIRE_LATENCY_TICKS 10

extern uint64_t current_tick;

typedef struct {
	int id;
	uint8_t rx_buffer[MAX_MTU];
	int rx_len;
	int has_new_data;
} Device;

typedef struct {
	Device* endpoint_a;
	Device* endpoint_b;

	uint8_t in_transit_data[MAX_MTU];
	int in_transit_len;
	bool sent_from_a; // otherwise from endpoint b

	uint64_t delivery_tick; // When does the packet pop out the other side?
	int is_busy;            // Is the wire currently carrying something?
} Wire;

// Function to inject data into the wire
void wire_send(Wire* w, const Device* sender, const uint8_t* data, int len);

// The function that runs every tick
void wire_tick(Wire* w);
