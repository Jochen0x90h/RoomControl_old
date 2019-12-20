#pragma once

#include "config.hpp"
#include <stdint.h>


class Flash {
public:
	// size of one flash page in bytes
	static const int PAGE_SIZE = FLASH_PAGE_SIZE;
		
	// number of pages to use
	static const int PAGE_COUNT = FLASH_PAGE_COUNT;
	
	// use at least 4 so that 32 bit types such as int are 4 byte aligned
	static const int WRITE_ALIGN = FLASH_WRITE_ALIGN < 4 ? 4 : FLASH_WRITE_ALIGN;


	static const uint8_t *getAddress(uint8_t pageIndex);

	static bool isEmpty(uint8_t pageIndex) {
		const uint32_t *page = reinterpret_cast<const uint32_t*>(getAddress(pageIndex));
		for (int i = 0; i < PAGE_SIZE >> 2; ++i) {
			if (page[i] != 0xffffffff)
				return false;
		}
		return true;
	}
	
	static void erase(uint8_t pageIndex);
	
	static constexpr int align(int size) {
		return (size + WRITE_ALIGN - 1) & ~(WRITE_ALIGN - 1);
	}
	
	static const uint8_t *write(const uint8_t *address, const void *data, int size);
};
