#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "device.h"

int main() {
	srand(time(NULL));

	Device nodeA = {.id = 1, .has_new_data = 0};
	Device nodeB = {.id = 2, .has_new_data = 0};

	Wire wire = {.endpoint_a = &nodeA, .endpoint_b = &nodeB, .is_busy = 0};

	nodeA.wire = &wire;
	nodeB.wire = &wire;

	while (current_tick < 50) {
		if (current_tick == 5) {
			uint8_t payload[] = "Hello Layer 1!";
			wire_send(nodeA.wire, &nodeA, payload, sizeof(payload));
		}

		wire_tick(&wire);

		if (nodeB.has_new_data) {
			printf("[Tick %llu] Node %d processed rx_buffer: %s\n",
			       current_tick, nodeB.id, nodeB.rx_buffer);
			nodeB.has_new_data = 0;
		}

		current_tick++;
	}

	return 0;
}
