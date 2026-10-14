#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "device.h"
#include "ethernet.h"

int main() {
	srand(time(NULL));

	Node nodeA = {.id = 1, .mac = {0xAA, 0x11}};
	Node nodeB = {.id = 2, .mac = {0xBB, 0x22}};

	Wire wire = {.endpoint_a = &nodeA, .endpoint_b = &nodeB, .is_busy = 0};

	nodeA.wire = &wire;
	nodeB.wire = &wire;

	while (current_tick < 50) {
		if (current_tick == 5) {
			uint8_t payload[] = "Hello Ethernet!";

			Packet packet = {0};
			packet.data = packet.buffer + sizeof(packet.buffer) - sizeof(payload);
			memcpy(packet.data, payload, sizeof(payload));
			packet.length = sizeof(payload);

			ethernet_send(&nodeA, &packet, nodeB.mac, TYPE_IPV4);
		}


		wire_tick(&wire);

		if (nodeB.rx_packet.length) {
			if (ethernet_receive(&nodeB, &nodeB.rx_packet) == NET_OK)
				printf("[Tick %llu] Node %d processed rx_buffer: %s\n",
				       current_tick, nodeB.id, (const char *) nodeB.rx_packet.data);

			memset(&nodeB.rx_packet, 0, sizeof(Packet));
		}

		current_tick++;
	}

	return 0;
}
