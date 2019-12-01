#pragma once

#include <stdint.h>


class Flash {
public:
	static const int PAGE_SIZE = 1024;
	static const int PAGE_OFFSET = 16;
	static const int PAGE_COUNT = 16;
	
	// use at least 4 so that 32 bit types are 4 byte aligned
	static const int WRITE_ALIGN = 4;

	static uint8_t data[PAGE_COUNT * PAGE_SIZE];

	static const uint8_t *getAddress(uint8_t pageIndex);

	static bool isEmpty(uint8_t pageIndex) {
		const uint8_t *page = getAddress(pageIndex);
		for (int i = 0; i < PAGE_SIZE; ++i) {
			if (page[i] != 0xff)
				return false;
		}
		return true;
	}
	
	static void erase(uint8_t pageIndex);
	
	static constexpr int align(int size) {
		return (size + WRITE_ALIGN - 1) & ~(WRITE_ALIGN - 1);
	}
	
	static const uint8_t *write(const uint8_t *address, const void *data, int size) {
		for (int i = 0; i < size; ++i) {
			const_cast<uint8_t*>(address)[i] = reinterpret_cast<const uint8_t*>(data)[i];
		}
		return address + align(size);
	}
};
