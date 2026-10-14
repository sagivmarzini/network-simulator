#include "physical.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAC_AGING_TICKS 1000  // entries older than this are ignored and reusable

// Ethernet frame starts with: dest_mac[6], src_mac[6], ...
#define FRAME_DEST_OFFSET 0
#define FRAME_SRC_OFFSET  SIZE_MAC_ADDRESS
#define FRAME_MIN_LEN     (2 * SIZE_MAC_ADDRESS)

uint64_t current_tick = 0;

void device_init(Device* d, int id, const DeviceOps* ops, int port_count) {
	memset(d, 0, sizeof(*d));
	d->id = id;
	d->ops = ops;
	d->port_count = port_count;
	for (int i = 0; i < port_count; i++) {
		d->ports[i].owner = d;
		d->ports[i].index = i;
	}
}

NetResult device_send(Device* d, int port_index, const uint8_t* data, int len) {
	if (port_index < 0 || port_index >= d->port_count) return NET_ERR_INVALID;

	Port* p = &d->ports[port_index];
	if (!p->wire) return NET_ERR_NO_ROUTE;

	return wire_send(p->wire, p, data, len);
}

bool wire_connect(Wire* w, Port* a, Port* b) {
	if (a->wire || b->wire) return false;

	memset(w, 0, sizeof(*w));
	w->end[0] = a;
	w->end[1] = b;
	a->wire = w;
	b->wire = w;
	return true;
}

NetResult wire_send(Wire* w, const Port* from, const uint8_t* data, int len) {
	int side;
	if (from == w->end[0]) side = 0;
	else if (from == w->end[1]) side = 1;
	else return NET_ERR_NO_ROUTE;

	if (len > MAX_MTU) return NET_ERR_INVALID;

	Lane* lane = &w->lane[side];
	if (lane->busy) return NET_DROP;

	memcpy(lane->data, data, len);
	lane->len = len;
	lane->delivery_tick = current_tick + WIRE_LATENCY_TICKS;
	lane->busy = true;

	return NET_OK;
}

static void lane_tick(Wire* w, int side) {
	Lane* lane = &w->lane[side];
	if (!lane->busy || current_tick < lane->delivery_tick) return;

	// Free the lane first, so the receiver can answer immediately if it wants.
	lane->busy = false;

	// 10% chance to drop the packet entirely
	if (rand() % 100 < 10) {
		printf("[Tick %llu] WIRE DROPPED THE PACKET!\n",
		       (unsigned long long) current_tick);
		return;
	}

	// 5% chance to corrupt a random byte
	if (rand() % 100 < 5) {
		int random_byte = rand() % lane->len;
		lane->data[random_byte] ^= 0xFF;
		printf("[Tick %llu] WIRE CORRUPTED THE PACKET!\n",
		       (unsigned long long) current_tick);
	}

	Port* dst = w->end[1 - side];
	printf("[Tick %llu] Wire delivered %d bytes to Device %d (port %d)\n",
	       (unsigned long long) current_tick, lane->len, dst->owner->id, dst->index);

	dst->owner->ops->on_receive(dst->owner, dst->index, lane->data, lane->len);
}

void wire_tick(Wire* w) {
	lane_tick(w, 0);
	lane_tick(w, 1);
}

static void host_on_receive(Device* self, int port_index, const uint8_t* data, int len) {
	(void) port_index; // hosts only have one port
	Host* h = (Host *) self;

	Packet* packet = &h->rx_packet;
	packet->data = packet->buffer + sizeof(packet->buffer) - len;
	memcpy(packet->data, data, len);
	packet->length = len;
}

static const DeviceOps host_ops = {
	.on_receive = host_on_receive,
	.on_tick = NULL,
};

void host_init(Host* h, int id, const uint8_t mac[SIZE_MAC_ADDRESS]) {
	memset(h, 0, sizeof(*h));
	device_init(&h->base, id, &host_ops, 1);
	memcpy(h->mac, mac, SIZE_MAC_ADDRESS);
}

static bool mac_is_multicast(const uint8_t* mac) {
	return mac[0] & 0x01; // broadcast ff:ff:... is also multicast
}

static bool entry_is_fresh(const MacTableEntry* e) {
	return e->valid && current_tick - e->last_seen_tick < MAC_AGING_TICKS;
}

static MacTableEntry* mac_table_find(Switch* s, const uint8_t* mac) {
	for (int i = 0; i < MAC_TABLE_SIZE; i++) {
		MacTableEntry* e = &s->mac_table[i];
		if (entry_is_fresh(e) && memcmp(e->mac, mac, SIZE_MAC_ADDRESS) == 0) return e;
	}
	return NULL;
}

static void mac_table_learn(Switch* s, const uint8_t* mac, int port_index) {
	MacTableEntry* slot = mac_table_find(s, mac);

	if (!slot) {
		// Take a free/stale slot, otherwise evict the oldest entry.
		MacTableEntry* oldest = &s->mac_table[0];
		for (int i = 0; i < MAC_TABLE_SIZE; i++) {
			MacTableEntry* e = &s->mac_table[i];
			if (!entry_is_fresh(e)) {
				slot = e;
				break;
			}
			if (e->last_seen_tick < oldest->last_seen_tick) oldest = e;
		}
		if (!slot) slot = oldest;
		memcpy(slot->mac, mac, SIZE_MAC_ADDRESS);
		slot->valid = true;
	}

	slot->port_index = port_index;
	slot->last_seen_tick = current_tick;
}

static void switch_flood(Switch* s, int ingress, const uint8_t* data, int len) {
	for (int i = 0; i < s->base.port_count; i++) {
		if (i == ingress || !s->base.ports[i].wire) continue;
		device_send(&s->base, i, data, len);
	}
}

static void switch_on_receive(Device* self, int port_index, const uint8_t* data, int len) {
	Switch* s = (Switch *) self;
	if (len < FRAME_MIN_LEN) return;

	const uint8_t* dest = data + FRAME_DEST_OFFSET;
	const uint8_t* src = data + FRAME_SRC_OFFSET;

	// 1. Learn where the sender lives.
	if (!mac_is_multicast(src)) mac_table_learn(s, src, port_index);

	// 2. Forward.
	if (mac_is_multicast(dest)) {
		switch_flood(s, port_index, data, len);
		return;
	}

	MacTableEntry* e = mac_table_find(s, dest);
	if (!e) {
		switch_flood(s, port_index, data, len); // unknown destination
	} else if (e->port_index != port_index) {
		device_send(&s->base, e->port_index, data, len);
	}
	// else: destination is on the ingress port, so filter (drop) it.
}

static const DeviceOps switch_ops = {
	.on_receive = switch_on_receive,
	.on_tick = NULL,
};

void switch_init(Switch* s, int id, int port_count) {
	memset(s, 0, sizeof(*s));
	if (port_count > MAX_PORTS) port_count = MAX_PORTS;
	device_init(&s->base, id, &switch_ops, port_count);
}
