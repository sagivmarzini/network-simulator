#include "ethernet.h"

#include <string.h>
#include <zlib.h>

NetResult ethernet_send(const Host* sender, Packet* packet, const uint8_t dest_mac[SIZE_MAC_ADDRESS],
                        uint16_t ethertype) {
	EthernetHeader header = {0};

	// Ethernet header
	memcpy(header.dest_mac, dest_mac, SIZE_MAC_ADDRESS);
	memcpy(header.src_mac, sender->mac, SIZE_MAC_ADDRESS);
	header.type = ethertype;

	// Payload
	packet->data -= sizeof(header);
	memcpy(packet->data, &header, sizeof(header));
	packet->length += sizeof(header);

	// CRC checksum
	uLong crc = crc32(0L, Z_NULL, 0);
	crc = crc32(crc, packet->data, packet->length - SIZE_FCS);
	memcpy(packet->data + packet->length - SIZE_FCS, &crc, SIZE_FCS);

	return device_send((Device *) sender, 0, packet->data, packet->length);
}

static bool isCrcCorrect(Packet* packet) {
	uLong received_crc = 0;
	memcpy(&received_crc, packet->data + packet->length - SIZE_FCS, SIZE_FCS);

	uLong crc = crc32(0L, Z_NULL, 0);
	crc = crc32(crc, packet->data, packet->length - SIZE_FCS);

	return received_crc == crc;
}

NetResult ethernet_receive(const Host* receiver, Packet* packet) {
	EthernetHeader header = {0};
	memcpy(&header, packet->data, sizeof(header));

	// Check if dest MAC is to this host
	if (memcmp(&header.dest_mac, &receiver->mac, SIZE_MAC_ADDRESS) != 0) return NET_DROP;
	// Check if the frame is corrupted
	if (!isCrcCorrect(packet)) return NET_ERR_INVALID;

	packet->data += sizeof(header);
	packet->length -= sizeof(header);

	return NET_OK;
}
