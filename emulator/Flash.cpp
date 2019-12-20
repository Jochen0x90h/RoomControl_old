#include "Flash.hpp"
#include <algorithm>
#include <assert.h>


static uint8_t data[Flash::PAGE_COUNT * Flash::PAGE_SIZE];

const uint8_t *Flash::getAddress(uint8_t pageIndex) {
	assert(pageIndex <= PAGE_COUNT);
	return data + pageIndex * PAGE_SIZE;
}

void Flash::erase(uint8_t pageIndex) {
	uint8_t *page = (uint8_t*)getAddress(pageIndex);
	std::fill(page, page + PAGE_SIZE, 0xff);
}

const uint8_t *Flash::write(const uint8_t *address, const void *data, int size) {
	for (int i = 0; i < size; ++i) {
		const_cast<uint8_t*>(address)[i] = reinterpret_cast<const uint8_t*>(data)[i];
	}
	return address + align(size);
}
