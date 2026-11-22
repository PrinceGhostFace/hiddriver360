#pragma once
#include <stdint.h>
#include "usb.h"

// Xbox One / Series controllers don't use HID, they speak GIP (Gaming Input Protocol) on a vendor specific
// interface. Packet layouts and init sequences follow the Linux xpad driver.
#define GIP_INTERFACE_CLASS    0xFF
#define GIP_INTERFACE_SUBCLASS 0x47
#define GIP_INTERFACE_PROTOCOL 0xD0

#define GIP_CMD_ACK         0x01
#define GIP_CMD_ANNOUNCE    0x02
#define GIP_CMD_STATUS      0x03
#define GIP_CMD_IDENTIFY    0x04
#define GIP_CMD_POWER       0x05
#define GIP_CMD_AUTHENTICATE 0x06
#define GIP_CMD_VIRTUAL_KEY 0x07
#define GIP_CMD_RUMBLE      0x09
#define GIP_CMD_LED         0x0A
#define GIP_CMD_INPUT       0x20

#define GIP_OPT_ACK         0x10
#define GIP_OPT_INTERNAL    0x20
#define GIP_OPT_CHUNK_START 0x40
#define GIP_OPT_CHUNK       0x80

#define GIP_MAX_PACKET 64

struct GipHeader {
	uint8_t command;
	uint8_t options;
	uint8_t sequence;
	uint32_t length;        // payload length
	uint32_t chunkOffset;   // only with GIP_OPT_CHUNK, the first chunk carries the total length here instead
	uint32_t headerLength;
};

struct GipInitPacket {
	uint16_t vendorId;   // 0 matches every vendor
	uint16_t productId;  // 0 matches every product of that vendor
	const uint8_t* data;
	uint8_t length;
};

// Byte 2 of every packet below is the sequence number and has to be filled in by the sender.

// Returns the next init packet for this controller starting at *cursor, or nullptr when there are no more
const GipInitPacket* GipNextInitPacket(uint16_t vendorId, uint16_t productId, int* cursor);

// Asks an announced controller to describe itself, it keeps announcing until it gets this
extern const uint8_t gipIdentifyRequest[4];

// Returns the next packet to send once the controller finished describing itself
const GipInitPacket* GipNextStartupPacket(int* cursor);

// Decodes the header of the packet at data (the lengths are 7 bit varints). Returns false when the packet doesn't
// fit in length bytes.
bool GipDecodeHeader(const uint8_t* data, uint32_t length, GipHeader* header);

// Writes the ack for a received packet that had GIP_OPT_ACK set, returns its length. chunkTotal keeps the total
// length of a chunked transfer between calls.
uint8_t GipBuildAck(const GipHeader* header, uint32_t* chunkTotal, uint8_t* out);

// Updates report from an input (0x20) or virtual key (0x07) packet. Returns false for anything else.
bool GipParsePacket(const uint8_t* data, uint32_t length, ButtonsReport* report);

// Builds a rumble packet, motor speeds are XInput's 0-65535. Returns its length.
uint8_t GipBuildRumble(uint8_t sequence, uint16_t leftMotor, uint16_t rightMotor, uint8_t* out);
