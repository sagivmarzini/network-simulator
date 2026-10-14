#pragma once
#include "device.h"
#include "packet.h"

#define TYPE_IPV4 0x0800

typedef struct __attribute__((packed)) {
	uint8_t dest_mac[SIZE_MAC_ADDRESS], src_mac[SIZE_MAC_ADDRESS];
	uint16_t type;
} EthernetHeader;

NetResult ethernet_send(const Node* sender, Packet* packet, const uint8_t dest_mac[SIZE_MAC_ADDRESS],
                        uint16_t ethertype);

NetResult ethernet_receive(Node* receiver, Packet* packet);
