#include "ethernet.h"

#include <string.h>

NetResult ethernet_send(const Node* sender, Packet* packet, const uint8_t dest_mac[SIZE_MAC_ADDRESS],
                        uint16_t ethertype) {
	EthernetHeader header = {0};
	memcpy(header.dest_mac, dest_mac, SIZE_MAC_ADDRESS);
	memcpy(header.src_mac, sender->mac, SIZE_MAC_ADDRESS);
	header.type = ethertype;

	packet->data -= sizeof(header);
	memcpy(packet->data, &header, sizeof(header));
	packet->length += sizeof(header);

	return wire_send(sender->wire, sender, packet->data, packet->length);
}

NetResult ethernet_receive(Node* receiver, Packet* packet) {
	EthernetHeader header = {0};
	memcpy(&header, packet->data, sizeof(header));

	if (memcmp(&header.dest_mac, &receiver->mac, SIZE_MAC_ADDRESS) != 0) return NET_DROP;

	packet->data += sizeof(header);
	packet->length -= sizeof(header);

	return NET_OK;
}
