#pragma once
#include "physical.h"
#include "packet.h"

#define TYPE_IPV4 0x0800
#define SIZE_FCS 4 // size in bytes of the checksum field of the ethernet frame

typedef struct __attribute__((packed)) {
	uint8_t dest_mac[SIZE_MAC_ADDRESS], src_mac[SIZE_MAC_ADDRESS];
	uint16_t type;
} EthernetHeader;

NetResult ethernet_send(const Host* sender, Packet* packet, const uint8_t dest_mac[SIZE_MAC_ADDRESS],
                        uint16_t ethertype);

NetResult ethernet_receive(const Host* receiver, Packet* packet);
