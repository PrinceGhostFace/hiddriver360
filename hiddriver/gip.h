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
#define GIP_CMD_IDENTIFY    0x04
#define GIP_CMD_POWER       0x05
#define GIP_CMD_AUTHENTICATE 0x06
#define GIP_CMD_VIRTUAL_KEY 0x07
#define GIP_CMD_RUMBLE      0x09
#define GIP_CMD_LED         0x0A
#define GIP_CMD_INPUT       0x20

#define GIP_OPT_ACK      0x10
#define GIP_OPT_INTERNAL 0x20

#define GIP_MAX_PACKET 64

struct GipInitPacket {
	uint16_t vendorId;   // 0 matches every vendor
	uint16_t productId;  // 0 matches every product of that vendor
	const uint8_t* data;
	uint8_t length;
};

// Returns the next init packet for this controller starting at *cursor, or nullptr when there are no more.
// Byte 2 of every packet is the sequence number and has to be filled in by the sender.
const GipInitPacket* GipNextInitPacket(uint16_t vendorId, uint16_t productId, int* cursor);

// Writes the ack for a received packet that had GIP_OPT_ACK set, returns its length
uint8_t GipBuildAck(const uint8_t* packet, uint8_t* out);

// Updates report from an input (0x20) or virtual key (0x07) packet. Returns false for anything else.
bool GipParsePacket(const uint8_t* data, uint32_t length, ButtonsReport* report);

// Builds a rumble packet, motor speeds are XInput's 0-65535. Returns its length.
uint8_t GipBuildRumble(uint8_t sequence, uint16_t leftMotor, uint16_t rightMotor, uint8_t* out);
