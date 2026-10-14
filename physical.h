#pragma once
#include <stdbool.h>
#include <stdint.h>

#include "packet.h"

#define MAX_MTU             1528  // max bytes a device will accept in one frame
#define SIZE_MAC_ADDRESS            6
#define WIRE_LATENCY_TICKS  10
#define MAX_PORTS           8     // upper bound per device (host uses 1, switch uses N)
#define MAC_TABLE_SIZE      64

extern uint64_t current_tick;

typedef struct Device Device;
typedef struct Port Port;
typedef struct Wire Wire;

// The only thing a wire ever attaches to. Hosts have one, switches have many.
struct Port {
	Device* owner;
	int index;  // position inside owner->ports
	Wire* wire; // NULL if unplugged
};

// Common base for anything that can be plugged into a wire. Behaviour is supplied through ops (a tiny vtable).
typedef struct {
	// A frame arrived on port `port_index`.
	void (*on_receive)(Device* self, int port_index, const uint8_t* data, int len);

	// Optional: called once per tick (aging, timers...). May be NULL.
	void (*on_tick)(Device* self);
} DeviceOps;

struct Device {
	int id;
	const DeviceOps* ops;
	Port ports[MAX_PORTS];
	int port_count;
};

void device_init(Device* d, int id, const DeviceOps* ops, int port_count);

// Send out of a specific port (no-op / error if unplugged).
NetResult device_send(Device* d, int port_index, const uint8_t* data, int len);


// Connects exactly two ports. Full duplex: one lane per direction, so A->B and B->A don't block each other.
typedef struct {
	uint8_t data[MAX_MTU];
	int len;
	uint64_t delivery_tick;
	bool busy;
} Lane;

struct Wire {
	Port* end[2];
	Lane lane[2]; // lane[i] carries traffic that was sent FROM end[i]
};

// Plug both ends in. Fails if either port is already connected.
bool wire_connect(Wire* w, Port* a, Port* b);

NetResult wire_send(Wire* w, const Port* from, const uint8_t* data, int len);

// Run every tick: delivers finished lanes to the far port's device.
void wire_tick(Wire* w);

// A Device with one port and a MAC address. `base` must stay the first member so (Device*) <-> (Host*) casts are valid.
typedef struct {
	Device base;
	uint8_t mac[SIZE_MAC_ADDRESS];
	Packet rx_packet;
} Host;

void host_init(Host* h, int id, const uint8_t mac[SIZE_MAC_ADDRESS]);

// A Device with N ports and a learning MAC table
typedef struct {
	uint8_t mac[SIZE_MAC_ADDRESS];
	int port_index;
	uint64_t last_seen_tick;
	bool valid;
} MacTableEntry;

typedef struct {
	Device base; // must be first
	MacTableEntry mac_table[MAC_TABLE_SIZE];
} Switch;

void switch_init(Switch* s, int id, int port_count);
