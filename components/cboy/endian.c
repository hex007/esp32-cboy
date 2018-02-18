#include "endian.h"


uint32_t be32toh( uint32_t x )
{
	return (x << 16) | (x >> 16);
}

uint16_t be16toh( uint32_t x )
{
	return ((x << 8) & 0xFF00) | ((x >> 8) & 0xFF);
}
