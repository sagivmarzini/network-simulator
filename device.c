#include "device.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint64_t current_tick = 0;

void wire_send(Wire* w, const Device* sender, const uint8_t* data, int len) {
	if (sender != w->endpoint_a && sender != w->endpoint_b) {
		printf("[Tick %llu] Error! Sender is sending data through remote wire.\n", current_tick);
		return;
	}
	if (len > MAX_MTU) {
		printf("[Tick %llu] Error! Data too long for wire.\n", current_tick);
		return;
	}

	if (w->is_busy) {
		printf("[Tick %llu] Collision! Wire is busy.\n", current_tick);
		return; // Packet lost!
	}

	memcpy(w->in_transit_data, data, len);
	w->in_transit_len = len;
	w->delivery_tick = current_tick + WIRE_LATENCY_TICKS;
	w->is_busy = 1;
	w->sent_from_a = sender == w->endpoint_a;

	printf("[Tick %llu] Node %d sending %d bytes. Arrives at tick %llu\n",
	       current_tick, sender->id, len, w->delivery_tick);
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

		Device* receiver_device = w->sent_from_a ? w->endpoint_b : w->endpoint_a;
		memcpy(receiver_device->rx_buffer, w->in_transit_data, w->in_transit_len);
		receiver_device->rx_len = w->in_transit_len;
		receiver_device->has_new_data = 1;

		printf("[Tick %llu] Wire delivered %d bytes to Node %d\n",
		       current_tick, w->in_transit_len, receiver_device->id);

		w->is_busy = 0; // Wire is ready for next packet
	}
}
