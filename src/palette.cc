#include "palette.hh"
namespace vhvc::palette {
namespace {
extern const uint8_t ntscpalette[8*64*3];
}
uint32_t colors[8*64] = {0};
void set_colors(const uint8_t *pal) {
	for (int i = 0; i < 8*64; i++) {
		int r = *pal++;
		int g = *pal++;
		int b = *pal++;
		colors[i] = r<<16 | g<<8 | b;
	}
}
void set_default_colors() {
	set_colors(ntscpalette);
}
namespace {
const uint8_t ntscpalette[8*64*3] = {
	0x52, 0x52, 0x52, 0x01, 0x1A, 0x51, 0x0F, 0x0F, 0x65, 0x23, 0x06, 0x63,
	0x36, 0x03, 0x4B, 0x40, 0x04, 0x26, 0x3F, 0x09, 0x04, 0x32, 0x13, 0x00,
	0x1F, 0x20, 0x00, 0x0B, 0x2A, 0x00, 0x00, 0x2F, 0x00, 0x00, 0x2E, 0x0A,
	0x00, 0x26, 0x2D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xA0, 0xA0, 0xA0, 0x1E, 0x4A, 0x9D, 0x38, 0x37, 0xBC, 0x58, 0x28, 0xB8,
	0x75, 0x21, 0x94, 0x84, 0x23, 0x5C, 0x82, 0x2E, 0x24, 0x6F, 0x3F, 0x00,
	0x51, 0x52, 0x00, 0x31, 0x63, 0x00, 0x1A, 0x6B, 0x05, 0x0E, 0x69, 0x2E,
	0x10, 0x5C, 0x68, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xFE, 0xFF, 0xFF, 0x69, 0x9E, 0xFC, 0x89, 0x87, 0xFF, 0xAE, 0x76, 0xFF,
	0xCE, 0x6D, 0xF1, 0xE0, 0x70, 0xB2, 0xDE, 0x7C, 0x70, 0xC8, 0x91, 0x3E,
	0xA6, 0xA7, 0x25, 0x81, 0xBA, 0x28, 0x63, 0xC4, 0x46, 0x54, 0xC1, 0x7D,
	0x56, 0xB3, 0xC0, 0x3C, 0x3C, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xFE, 0xFF, 0xFF, 0xBE, 0xD6, 0xFD, 0xCC, 0xCC, 0xFF, 0xDD, 0xC4, 0xFF,
	0xEA, 0xC0, 0xF9, 0xF2, 0xC1, 0xDF, 0xF1, 0xC7, 0xC2, 0xE8, 0xD0, 0xAA,
	0xD9, 0xDA, 0x9D, 0xC9, 0xE2, 0x9E, 0xBC, 0xE6, 0xAE, 0xB4, 0xE5, 0xC7,
	0xB5, 0xDF, 0xE4, 0xA9, 0xA9, 0xA9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x4B, 0x34, 0x32, 0x00, 0x07, 0x2E, 0x0B, 0x01, 0x41, 0x1D, 0x00, 0x42,
	0x30, 0x00, 0x31, 0x3B, 0x00, 0x16, 0x3D, 0x03, 0x00, 0x2F, 0x09, 0x00,
	0x1C, 0x10, 0x00, 0x09, 0x16, 0x00, 0x00, 0x18, 0x00, 0x00, 0x15, 0x00,
	0x00, 0x0D, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x94, 0x71, 0x6C, 0x17, 0x27, 0x67, 0x30, 0x1C, 0x84, 0x4F, 0x13, 0x85,
	0x6B, 0x11, 0x6B, 0x7C, 0x15, 0x41, 0x7E, 0x21, 0x16, 0x6A, 0x2C, 0x00,
	0x4C, 0x38, 0x00, 0x2D, 0x42, 0x00, 0x16, 0x45, 0x00, 0x0A, 0x40, 0x10,
	0x08, 0x32, 0x39, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xEE, 0xBC, 0xB5, 0x5D, 0x67, 0xB0, 0x7C, 0x59, 0xCF, 0xA0, 0x4F, 0xD1,
	0xC0, 0x4B, 0xB4, 0xD3, 0x51, 0x84, 0xD6, 0x60, 0x51, 0xBF, 0x6D, 0x25,
	0x9D, 0x7B, 0x0F, 0x79, 0x87, 0x0E, 0x5C, 0x8A, 0x22, 0x4B, 0x84, 0x49,
	0x49, 0x75, 0x7B, 0x36, 0x23, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xEE, 0xBC, 0xB5, 0xAF, 0x98, 0xB3, 0xBD, 0x91, 0xC0, 0xCD, 0x8D, 0xC0,
	0xDB, 0x8B, 0xB5, 0xE3, 0x8E, 0xA1, 0xE4, 0x94, 0x8A, 0xDA, 0x9A, 0x75,
	0xCC, 0xA1, 0x69, 0xBC, 0xA5, 0x69, 0xAF, 0xA7, 0x74, 0xA7, 0xA4, 0x86,
	0xA6, 0x9E, 0x9D, 0x9D, 0x78, 0x73, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x2E, 0x45, 0x27, 0x00, 0x13, 0x2C, 0x00, 0x07, 0x3A, 0x0A, 0x00, 0x35,
	0x15, 0x00, 0x20, 0x1F, 0x00, 0x08, 0x21, 0x03, 0x00, 0x19, 0x0D, 0x00,
	0x0C, 0x19, 0x00, 0x01, 0x25, 0x00, 0x00, 0x2C, 0x00, 0x00, 0x28, 0x00,
	0x00, 0x1F, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x67, 0x8B, 0x5C, 0x08, 0x3D, 0x64, 0x18, 0x2A, 0x79, 0x2D, 0x1A, 0x72,
	0x40, 0x12, 0x51, 0x50, 0x16, 0x29, 0x52, 0x22, 0x04, 0x46, 0x34, 0x00,
	0x31, 0x48, 0x00, 0x1C, 0x59, 0x00, 0x0C, 0x64, 0x00, 0x02, 0x5E, 0x13,
	0x01, 0x50, 0x3D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xAE, 0xE1, 0x9D, 0x3C, 0x89, 0xA6, 0x52, 0x72, 0xBD, 0x6C, 0x5F, 0xB6,
	0x81, 0x54, 0x92, 0x94, 0x5A, 0x64, 0x96, 0x68, 0x34, 0x89, 0x7E, 0x12,
	0x71, 0x95, 0x04, 0x56, 0xA9, 0x08, 0x42, 0xB6, 0x21, 0x33, 0xAF, 0x48,
	0x31, 0x9F, 0x7A, 0x1E, 0x31, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xAE, 0xE1, 0x9D, 0x7D, 0xBB, 0xA1, 0x86, 0xB1, 0xAA, 0x92, 0xA9, 0xA7,
	0x9B, 0xA4, 0x99, 0xA3, 0xA7, 0x85, 0xA4, 0xAD, 0x70, 0x9E, 0xB7, 0x5F,
	0x94, 0xC1, 0x56, 0x89, 0xCA, 0x59, 0x7F, 0xCF, 0x66, 0x78, 0xCC, 0x79,
	0x77, 0xC5, 0x8F, 0x6E, 0x93, 0x62, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x2D, 0x2E, 0x1E, 0x00, 0x04, 0x21, 0x00, 0x00, 0x2F, 0x0A, 0x00, 0x2D,
	0x15, 0x00, 0x1D, 0x1F, 0x00, 0x06, 0x20, 0x00, 0x00, 0x18, 0x05, 0x00,
	0x0B, 0x0C, 0x00, 0x00, 0x13, 0x00, 0x00, 0x16, 0x00, 0x00, 0x13, 0x00,
	0x00, 0x0B, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x66, 0x66, 0x4D, 0x07, 0x23, 0x52, 0x18, 0x17, 0x67, 0x2C, 0x0E, 0x64,
	0x3F, 0x09, 0x4C, 0x4F, 0x0D, 0x25, 0x51, 0x18, 0x02, 0x44, 0x24, 0x00,
	0x30, 0x31, 0x00, 0x1B, 0x3C, 0x00, 0x0B, 0x42, 0x00, 0x01, 0x3C, 0x09,
	0x00, 0x2F, 0x2F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xAC, 0xAC, 0x89, 0x3B, 0x5F, 0x8E, 0x51, 0x50, 0xA5, 0x6A, 0x44, 0xA2,
	0x80, 0x3E, 0x87, 0x93, 0x44, 0x5A, 0x95, 0x52, 0x2C, 0x86, 0x60, 0x0D,
	0x6E, 0x70, 0x01, 0x55, 0x7D, 0x02, 0x40, 0x83, 0x12, 0x31, 0x7D, 0x36,
	0x2F, 0x6E, 0x66, 0x1D, 0x1D, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xAC, 0xAC, 0x89, 0x7B, 0x8B, 0x8B, 0x85, 0x85, 0x94, 0x90, 0x7F, 0x93,
	0x9A, 0x7D, 0x88, 0xA1, 0x7F, 0x75, 0xA2, 0x85, 0x60, 0x9C, 0x8C, 0x51,
	0x92, 0x93, 0x48, 0x86, 0x98, 0x49, 0x7D, 0x9B, 0x53, 0x76, 0x98, 0x65,
	0x75, 0x92, 0x7A, 0x6C, 0x6C, 0x53, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x37, 0x37, 0x59, 0x00, 0x0F, 0x53, 0x09, 0x08, 0x68, 0x17, 0x00, 0x63,
	0x24, 0x00, 0x4B, 0x2A, 0x00, 0x28, 0x26, 0x00, 0x07, 0x1A, 0x03, 0x00,
	0x09, 0x0A, 0x00, 0x00, 0x13, 0x00, 0x00, 0x1A, 0x00, 0x00, 0x1B, 0x0E,
	0x00, 0x17, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01,
	0x75, 0x75, 0xAB, 0x13, 0x37, 0xA1, 0x2C, 0x2A, 0xC0, 0x43, 0x1B, 0xB8,
	0x57, 0x13, 0x94, 0x61, 0x12, 0x5F, 0x5B, 0x17, 0x2A, 0x48, 0x21, 0x06,
	0x2C, 0x2D, 0x00, 0x17, 0x3D, 0x00, 0x08, 0x48, 0x0D, 0x03, 0x49, 0x37,
	0x06, 0x43, 0x6E, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01,
	0xC2, 0xC1, 0xFF, 0x50, 0x7B, 0xFF, 0x6E, 0x6C, 0xFF, 0x89, 0x5A, 0xFF,
	0xA0, 0x4F, 0xF5, 0xAB, 0x4D, 0xBA, 0xA5, 0x54, 0x7B, 0x8F, 0x61, 0x4A,
	0x6F, 0x6F, 0x30, 0x55, 0x83, 0x36, 0x40, 0x8E, 0x55, 0x38, 0x90, 0x8A,
	0x3D, 0x89, 0xCA, 0x25, 0x25, 0x42, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01,
	0xC2, 0xC1, 0xFF, 0x91, 0xA4, 0xFF, 0x9F, 0x9D, 0xFF, 0xAA, 0x95, 0xFF,
	0xB4, 0x90, 0xFF, 0xB9, 0x8F, 0xEA, 0xB6, 0x92, 0xCF, 0xAD, 0x98, 0xB8,
	0x9F, 0x9F, 0xAB, 0x93, 0xA7, 0xAE, 0x8A, 0xAC, 0xBD, 0x86, 0xAD, 0xD6,
	0x88, 0xAA, 0xF1, 0x7D, 0x7C, 0xB4, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01,
	0x32, 0x27, 0x35, 0x00, 0x04, 0x2F, 0x05, 0x00, 0x43, 0x12, 0x00, 0x41,
	0x1E, 0x00, 0x30, 0x25, 0x00, 0x17, 0x24, 0x00, 0x01, 0x18, 0x00, 0x00,
	0x08, 0x06, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x0E, 0x00,
	0x00, 0x0A, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x6D, 0x5B, 0x73, 0x0D, 0x21, 0x69, 0x25, 0x16, 0x86, 0x3B, 0x0D, 0x84,
	0x4F, 0x09, 0x6A, 0x5A, 0x0A, 0x43, 0x58, 0x10, 0x1B, 0x45, 0x1A, 0x00,
	0x2A, 0x26, 0x00, 0x15, 0x30, 0x00, 0x06, 0x36, 0x00, 0x00, 0x34, 0x14,
	0x01, 0x2C, 0x3B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xB6, 0x9D, 0xBE, 0x45, 0x5A, 0xB4, 0x63, 0x4C, 0xD4, 0x7D, 0x40, 0xD1,
	0x94, 0x3B, 0xB5, 0xA0, 0x3C, 0x88, 0x9F, 0x45, 0x59, 0x89, 0x51, 0x2C,
	0x69, 0x5F, 0x15, 0x50, 0x6C, 0x16, 0x3C, 0x72, 0x2B, 0x31, 0x70, 0x51,
	0x33, 0x67, 0x7F, 0x21, 0x18, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xB6, 0x9D, 0xBE, 0x85, 0x81, 0xBA, 0x93, 0x7A, 0xC7, 0x9E, 0x75, 0xC6,
	0xA8, 0x72, 0xBA, 0xAD, 0x73, 0xA8, 0xAC, 0x77, 0x93, 0xA3, 0x7D, 0x7E,
	0x95, 0x83, 0x71, 0x8A, 0x88, 0x72, 0x81, 0x8B, 0x7D, 0x7C, 0x8A, 0x8F,
	0x7C, 0x86, 0xA3, 0x73, 0x62, 0x7A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x24, 0x2F, 0x30, 0x00, 0x0A, 0x2F, 0x00, 0x03, 0x3D, 0x07, 0x00, 0x39,
	0x12, 0x00, 0x23, 0x19, 0x00, 0x0C, 0x18, 0x00, 0x00, 0x10, 0x01, 0x00,
	0x05, 0x08, 0x00, 0x00, 0x11, 0x00, 0x00, 0x17, 0x00, 0x00, 0x16, 0x01,
	0x00, 0x11, 0x17, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x57, 0x68, 0x6A, 0x04, 0x2D, 0x68, 0x14, 0x20, 0x7E, 0x28, 0x12, 0x77,
	0x3B, 0x0A, 0x56, 0x45, 0x0B, 0x30, 0x44, 0x12, 0x0C, 0x37, 0x1D, 0x00,
	0x23, 0x29, 0x00, 0x0F, 0x39, 0x00, 0x02, 0x44, 0x01, 0x00, 0x42, 0x1C,
	0x00, 0x39, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x97, 0xAF, 0xB2, 0x33, 0x6B, 0xB0, 0x48, 0x5C, 0xC8, 0x61, 0x4A, 0xC0,
	0x77, 0x3F, 0x9B, 0x83, 0x41, 0x70, 0x81, 0x4A, 0x43, 0x72, 0x58, 0x21,
	0x5B, 0x67, 0x11, 0x42, 0x7A, 0x16, 0x2F, 0x86, 0x31, 0x25, 0x84, 0x57,
	0x27, 0x7A, 0x86, 0x16, 0x1E, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x97, 0xAF, 0xB2, 0x6B, 0x92, 0xB1, 0x75, 0x8C, 0xBB, 0x80, 0x83, 0xB8,
	0x89, 0x7F, 0xA9, 0x8E, 0x7F, 0x96, 0x8E, 0x83, 0x82, 0x88, 0x8A, 0x72,
	0x7E, 0x91, 0x69, 0x73, 0x99, 0x6C, 0x6A, 0x9E, 0x7A, 0x65, 0x9D, 0x8B,
	0x65, 0x99, 0xA0, 0x5D, 0x6E, 0x71, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x25, 0x25, 0x25, 0x00, 0x03, 0x23, 0x00, 0x00, 0x31, 0x07, 0x00, 0x2F,
	0x12, 0x00, 0x20, 0x19, 0x00, 0x09, 0x18, 0x00, 0x00, 0x10, 0x00, 0x00,
	0x05, 0x05, 0x00, 0x00, 0x0B, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x0E, 0x00,
	0x00, 0x09, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x58, 0x58, 0x58, 0x05, 0x1F, 0x56, 0x14, 0x13, 0x6B, 0x28, 0x0A, 0x68,
	0x3B, 0x06, 0x50, 0x45, 0x07, 0x2B, 0x44, 0x0E, 0x08, 0x37, 0x18, 0x00,
	0x24, 0x24, 0x00, 0x10, 0x2F, 0x00, 0x02, 0x35, 0x00, 0x00, 0x33, 0x0E,
	0x00, 0x2B, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x98, 0x98, 0x98, 0x33, 0x56, 0x96, 0x48, 0x47, 0xAC, 0x61, 0x3C, 0xAA,
	0x77, 0x36, 0x8F, 0x83, 0x38, 0x64, 0x81, 0x40, 0x38, 0x73, 0x4E, 0x18,
	0x5C, 0x5D, 0x09, 0x43, 0x69, 0x0A, 0x30, 0x70, 0x1D, 0x26, 0x6E, 0x40,
	0x27, 0x64, 0x6D, 0x16, 0x16, 0x16, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x98, 0x98, 0x98, 0x6C, 0x7C, 0x97, 0x76, 0x75, 0xA0, 0x81, 0x70, 0x9F,
	0x8A, 0x6D, 0x94, 0x8F, 0x6E, 0x82, 0x8E, 0x72, 0x6E, 0x88, 0x78, 0x5E,
	0x7E, 0x7F, 0x56, 0x73, 0x84, 0x57, 0x6A, 0x87, 0x61, 0x65, 0x86, 0x72,
	0x66, 0x82, 0x86, 0x5E, 0x5E, 0x5E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
}
}
