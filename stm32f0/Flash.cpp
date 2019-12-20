#include "Flash.hpp"
#include <libopencm3/stm32/memorymap.h>


// start of pages to use
static const int PAGE_START = 32768 - Flash::PAGE_COUNT * Flash::PAGE_SIZE;

const uint8_t *Flash::getAddress(uint8_t pageIndex) {
	return (uint8_t*)FLASH_BASE + PAGE_START + pageIndex * PAGE_SIZE;
}

void Flash::erase(uint8_t pageIndex) {
}

const uint8_t *Flash::write(const uint8_t *address, const void *data, int size) {
	return address + align(size);
}
