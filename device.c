#include "device.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint64_t current_tick = 0;

NetResult wire_send(Wire* w, const Node* sender, const uint8_t* data, int len) {
	if (sender != w->endpoint_a && sender != w->endpoint_b) {
		return NET_ERR_NO_ROUTE;
	}
	if (len > MAX_MTU) {
		return NET_ERR_INVALID;
	}

	if (w->is_busy) {
		return NET_DROP;
	}

	memcpy(w->in_transit_data, data, len);
	w->in_transit_len = len;
	w->delivery_tick = current_tick + WIRE_LATENCY_TICKS;
	w->is_busy = 1;
	w->sent_from_a = sender == w->endpoint_a;

	return NET_OK;
}

void wire_tick(Wire* w) {
	if (!w->is_busy) return;

	// Has the packet reached its destination?
	if (current_tick == w->delivery_tick) {
		// 10% chance to drop the packet entirely
		if (rand() % 100 < 10) {
			printf("[Tick %llu] WIRE DROPPED THE PACKET!\n", current_tick);
			w->is_busy = 0;
			return;
		}

		// 5% chance to flip a random bit
		if (rand() % 100 < 5) {
			int random_byte = rand() % w->in_transit_len;
			w->in_transit_data[random_byte] ^= 0xFF;
			printf("[Tick %llu] WIRE CORRUPTED THE PACKET!\n", current_tick);
		}

		Node* receiver_device = w->sent_from_a ? w->endpoint_b : w->endpoint_a;

		Packet* packet = &receiver_device->rx_packet;
		packet->data = packet->buffer + sizeof(packet->buffer) - w->in_transit_len;
		memcpy(packet->data, w->in_transit_data, w->in_transit_len);
		packet->length = w->in_transit_len;

		printf("[Tick %llu] Wire delivered %d bytes to Node %d\n",
		       current_tick, w->in_transit_len, receiver_device->id);

		w->is_busy = 0; // Wire is ready for next packet
	}
}
