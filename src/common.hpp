#pragma once

#ifndef SCPP_COMMON
#define SCPP_COMMON

#include <string>

/**
 * Converts a value to hex.
 * 
 * @param w Value to be converted.
 * @param hex_len Length of the final hex string.
 * 
 * @return std::string The hex string.
 */
template <typename I>
std::string n2hexstr(I w, int hex_len = sizeof(I) << 1) {
	int i, j;
	static const char digits[17] = "0123456789abcdef";

	std::string rc(hex_len, '0');
	for (i = 0, j = (hex_len - 1) * 4; i < hex_len; ++i, j -= 4)
		rc[i] = digits[(w >> j) & 0x0f];

	return rc;
}

#endif