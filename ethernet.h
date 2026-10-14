#pragma once
#include "device.h"
#include "packet.h"

#define SIZE_MAC_ADDRESS 6
#define MAX_SIZE_ETHERNET_DATA 1500

NetResult ethernet_send(Device* device, Packet* packet, const uint8_t dest_mac[6], uint16_t ethertype);

NetResult ethernet_receive(Device* device, Packet* packet);
