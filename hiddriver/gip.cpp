#include "gip.h"
#include <string.h>

// Init packets, sent in order after the controller has been added (Linux xpad: xboxone_init_packets)
static const uint8_t hori_ack_id[] = { GIP_CMD_ACK, GIP_OPT_INTERNAL, 0x00, 0x09, 0x00, GIP_CMD_IDENTIFY, GIP_OPT_INTERNAL, 0x3A, 0x00, 0x00, 0x00, 0x80, 0x00 };
static const uint8_t power_on[] = { GIP_CMD_POWER, GIP_OPT_INTERNAL, 0x00, 0x01, 0x00 };
static const uint8_t s_init[] = { GIP_CMD_POWER, GIP_OPT_INTERNAL, 0x00, 0x0F, 0x06 };
static const uint8_t extra_input_init[] = { 0x4D, 0x10, 0x00, 0x02, 0x07, 0x00 };
static const uint8_t pdp_led_on[] = { GIP_CMD_LED, GIP_OPT_INTERNAL, 0x00, 0x03, 0x00, 0x01, 0x14 };
static const uint8_t pdp_auth[] = { GIP_CMD_AUTHENTICATE, GIP_OPT_INTERNAL, 0x00, 0x02, 0x01, 0x00 };
static const uint8_t rumble_begin_init[] = { GIP_CMD_RUMBLE, 0x00, 0x00, 0x09, 0x00, 0x0F, 0x00, 0x00, 0x1D, 0x1D, 0xFF, 0x00, 0x00 };
static const uint8_t rumble_end_init[] = { GIP_CMD_RUMBLE, 0x00, 0x00, 0x09, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

#define INIT_PACKET(vid, pid, packet) { vid, pid, packet, sizeof(packet) }
static const GipInitPacket initPackets[] = {
	INIT_PACKET(0x0E6F, 0x0165, hori_ack_id),
	INIT_PACKET(0x0F0D, 0x0067, hori_ack_id),
	INIT_PACKET(0x0000, 0x0000, power_on),
	INIT_PACKET(0x045E, 0x02EA, s_init),      // Xbox One S
	INIT_PACKET(0x045E, 0x0B00, s_init),      // Elite Series 2
	INIT_PACKET(0x045E, 0x0B00, extra_input_init),
	INIT_PACKET(0x0E6F, 0x0000, pdp_led_on),  // PDP
	INIT_PACKET(0x20D6, 0xA01A, pdp_led_on),
	INIT_PACKET(0x0E6F, 0x0000, pdp_auth),
	INIT_PACKET(0x20D6, 0xA01A, pdp_auth),
	INIT_PACKET(0x24C6, 0x541A, rumble_begin_init), // PowerA
	INIT_PACKET(0x24C6, 0x542A, rumble_begin_init),
	INIT_PACKET(0x24C6, 0x543A, rumble_begin_init),
	INIT_PACKET(0x24C6, 0x541A, rumble_end_init),
	INIT_PACKET(0x24C6, 0x542A, rumble_end_init),
	INIT_PACKET(0x24C6, 0x543A, rumble_end_init),
};

const GipInitPacket* GipNextInitPacket(uint16_t vendorId, uint16_t productId, int* cursor) {
	while (*cursor < (int)(sizeof(initPackets) / sizeof(initPackets[0]))) {
		const GipInitPacket* packet = &initPackets[(*cursor)++];
		if (packet->vendorId && packet->vendorId != vendorId)
			continue;
		if (packet->productId && packet->productId != productId)
			continue;
		return packet;
	}
	return nullptr;
}

uint8_t GipBuildAck(const uint8_t* packet, uint8_t* out) {
	const uint8_t ack[] = { GIP_CMD_ACK, GIP_OPT_INTERNAL, packet[2], 0x09, 0x00, packet[0], GIP_OPT_INTERNAL, packet[3], 0x00, 0x00, 0x00, 0x00, 0x00 };
	memcpy(out, ack, sizeof(ack));
	return sizeof(ack);
}

static int16_t ReadInt16(const uint8_t* p) {
	return (int16_t)(p[0] | (p[1] << 8));
}

bool GipParsePacket(const uint8_t* data, uint32_t length, ButtonsReport* report) {
	if (length < 4)
		return false;

	if (data[0] == GIP_CMD_VIRTUAL_KEY && length >= 5) {
		report->xbox = data[4] & 0x01;
		return true;
	}

	if (data[0] != GIP_CMD_INPUT || length < 18)
		return false;

	report->start = (data[4] >> 2) & 1;  // menu
	report->back = (data[4] >> 3) & 1;   // view
	report->a_button = (data[4] >> 4) & 1;
	report->b_button = (data[4] >> 5) & 1;
	report->x_button = (data[4] >> 6) & 1;
	report->y_button = (data[4] >> 7) & 1;

	report->has_hat_switch = false;
	report->dpad_up = data[5] & 1;
	report->dpad_down = (data[5] >> 1) & 1;
	report->dpad_left = (data[5] >> 2) & 1;
	report->dpad_right = (data[5] >> 3) & 1;
	report->l1 = (data[5] >> 4) & 1;
	report->r1 = (data[5] >> 5) & 1;
	report->l3 = (data[5] >> 6) & 1;
	report->r3 = (data[5] >> 7) & 1;

	// Triggers are 10 bit, XInput wants 0-255 (rx/ry carry the analog triggers)
	report->rx = (data[6] | (data[7] << 8)) >> 2;
	report->ry = (data[8] | (data[9] << 8)) >> 2;

	// Sticks already use XInput's convention, up is positive
	report->x = ReadInt16(&data[10]);
	report->y = ReadInt16(&data[12]);
	report->z = ReadInt16(&data[14]);
	report->rz = ReadInt16(&data[16]);
	return true;
}

uint8_t GipBuildRumble(uint8_t sequence, uint16_t leftMotor, uint16_t rightMotor, uint8_t* out) {
	// Actuator strength goes from 0 to 100
	uint8_t strong = (uint8_t)((leftMotor * 100u) / 65535u);
	uint8_t weak = (uint8_t)((rightMotor * 100u) / 65535u);
	const uint8_t rumble[] = { GIP_CMD_RUMBLE, 0x00, sequence, 0x09, 0x00, 0x0F, 0x00, 0x00, strong, weak, 0xFF, 0x00, 0xFF };
	memcpy(out, rumble, sizeof(rumble));
	return sizeof(rumble);
}
