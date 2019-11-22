#include "Flash.hpp"
#include <algorithm>
#include <assert.h>


uint32_t Flash::data[PAGE_COUNT * (PAGE_SIZE >> 2)];

uint32_t *Flash::getAddress(uint8_t pageIndex) {
	assert(pageIndex >= PAGE_OFFSET && pageIndex <= PAGE_OFFSET + PAGE_COUNT);
	return data + (pageIndex - PAGE_OFFSET) * (PAGE_SIZE >> 2);
}

void Flash::erase(uint8_t pageIndex) {
	uint32_t *page = getAddress(pageIndex);
	std::fill(page, page + (PAGE_SIZE >> 2), 0xffffffff);
}
