#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "physical.h"
#include "ethernet.h"

#define NUM_HOSTS 3
#define NUM_WIRES 3
#define SIM_TICKS 130

typedef struct {
	uint64_t tick;
	Host* from;
	Host* to;
	const char* text;
} Script;

static void send_text(Host* from, const Host* to, const char* text) {
	size_t len = strlen(text) + 1;

	Packet packet = {0};
	packet.data = packet.buffer + sizeof(packet.buffer) - len - SIZE_FCS;
	memcpy(packet.data, text, len);
	packet.length = len + SIZE_FCS;

	printf("[Tick %llu] >>> Host %d sends \"%s\" to Host %d\n",
	       (unsigned long long) current_tick, from->base.id, text, to->base.id);

	ethernet_send(from, &packet, to->mac, TYPE_IPV4);
}

static void poll_host(Host* h) {
	if (!h->rx_packet.length) return;

	NetResult r = ethernet_receive(h, &h->rx_packet);
	if (r == NET_OK) {
		printf("[Tick %llu] <<< Host %d got: \"%s\"\n",
		       (unsigned long long) current_tick, h->base.id,
		       (const char *) h->rx_packet.data);
	} else if (r == NET_DROP) {
		printf("[Tick %llu] ... Host %d ignored a frame (not addressed to it)\n",
		       (unsigned long long) current_tick, h->base.id);
	} else {
		printf("[Tick %llu] ... Host %d rejected a frame\n",
		       (unsigned long long) current_tick, h->base.id);
	}

	memset(&h->rx_packet, 0, sizeof(Packet));
}

int main(void) {
	srand(time(NULL));

	Host a, b, c;
	Switch sw;
	Wire w[NUM_WIRES];

	host_init(&a, 1, (uint8_t[]){0xAA, 0x11, 0, 0, 0, 0});
	host_init(&b, 2, (uint8_t[]){0xBB, 0x22, 0, 0, 0, 0});
	host_init(&c, 3, (uint8_t[]){0xCC, 0x33, 0, 0, 0, 0});
	switch_init(&sw, 10, 3);

	wire_connect(&w[0], &a.base.ports[0], &sw.base.ports[0]);
	wire_connect(&w[1], &b.base.ports[0], &sw.base.ports[1]);
	wire_connect(&w[2], &c.base.ports[0], &sw.base.ports[2]);

	Host* hosts[NUM_HOSTS] = {&a, &b, &c};

	// The story:
	//  1. A -> B: the switch has never seen B, so it floods. C hears it too and ignores it.
	//  2. B -> A: the switch learned A in step 1, so this goes straight to A.
	//  3. A -> B: the switch learned B in step 2, so only B receives it. C stays silent.
	Script script[] = {
		{5, &a, &b, "Hello B, it's A!"},
		{45, &b, &a, "Hey A, B here."},
		{85, &a, &b, "Now it's just us two."},
	};
	size_t next = 0;

	printf("=== Switch demo: A(AA:11) B(BB:22) C(CC:33) ===\n\n");

	while (current_tick < SIM_TICKS) {
		if (next < sizeof(script) / sizeof(script[0]) && script[next].tick == current_tick) {
			send_text(script[next].from, script[next].to, script[next].text);
			next++;
		}

		for (int i = 0; i < NUM_WIRES; i++) wire_tick(&w[i]);
		for (int i = 0; i < NUM_HOSTS; i++) poll_host(hosts[i]);

		current_tick++;
	}

	return 0;
}
