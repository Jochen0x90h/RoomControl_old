#pragma once

#include <stdint.h>


class Flash {
public:
	static const int PAGE_SIZE = 1024;
	static const int PAGE_OFFSET = 16;
	static const int PAGE_COUNT = 16;
	static const int WRITE_ALIGN = 2;

	static uint32_t data[PAGE_COUNT * (PAGE_SIZE >> 2)];

	static uint32_t *getAddress(uint8_t pageIndex);

	static bool isEmpty(uint8_t pageIndex) {
		uint32_t *page = getAddress(pageIndex);
		for (int i = 0; i < PAGE_SIZE >> 2; ++i) {
			if (page[i] != 0xffffffff)
				return false;
		}
		return true;
	}
	
	static void erase(uint8_t pageIndex);
	
	static void write(uint32_t *address, uint32_t *data, int count) {
		for (int i = 0; i < count; ++i) {
			address[i] = data[i];
		}
	}
};
