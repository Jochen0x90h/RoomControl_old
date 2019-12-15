#include "Flash.hpp"
#include <algorithm>
#include <assert.h>


uint8_t Flash::data[PAGE_COUNT * PAGE_SIZE];

const uint8_t *Flash::getAddress(uint8_t pageIndex) {
	assert(pageIndex >= PAGE_OFFSET && pageIndex <= PAGE_OFFSET + PAGE_COUNT);
	return data + (pageIndex - PAGE_OFFSET) * PAGE_SIZE;
}

void Flash::erase(uint8_t pageIndex) {
	uint8_t *page = (uint8_t*)getAddress(pageIndex);
	std::fill(page, page + PAGE_SIZE, 0xff);
}
