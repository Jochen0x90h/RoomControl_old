#include "Flash.hpp"
#include <algorithm>


uint32_t Flash::data[PAGE_COUNT * (PAGE_SIZE >> 2)];

void Flash::erase(uint8_t pageIndex) {
	uint32_t *page = getAddress(pageIndex);
	std::fill(page, page + (PAGE_SIZE >> 2), 0xffffffff);
}
