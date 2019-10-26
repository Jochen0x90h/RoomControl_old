#include "util.hpp"
#include "Flash.hpp"
#include "Storage.hpp"


void Storage::init() {
	// detect current pages
	uint32_t *p = Flash::getAddress(pageStart);
	int pageStart = this->pageStart;
	int pageCount = this->pageCount;
	int pageStart2 = pageStart + pageCount;
	if (*p != 0xffffffff) {
		// first flash region is current
		this->it = p;
		this->end = Flash::getAddress(pageStart2);
		
		// ensure that second flash region is empty
		for (int i = 0; i < pageCount; ++i) {
			if (!Flash::isEmpty(pageStart2 + i))
				Flash::erase(pageStart2 + i);
		}
	} else {
		// second flash region is current
		this->it = Flash::getAddress(pageStart2);
		this->end = Flash::getAddress(pageStart2 + pageCount);

		// ensure that first flash region is empty
		for (int i = 0; i < pageCount; ++i) {
			if (!Flash::isEmpty(pageStart + i))
				Flash::erase(pageStart + i);
		}
	}

	// initialize arrays
	uint32_t *it = this->it;
	while (it < this->end && *it != 0xffffffff) {
		Header *header = (Header*)it;
		++it;
		
		// get array
		ArrayData *arrayData = this->first;
		int count = this->count;
		for (int i = count - 1; i > header->arrayIndex; --i)
			arrayData = arrayData->next;
		
		// execute operation
		int op = header->op;
		int index = header->index;
		if (op == OVERWRITE) {
			int count = header->value;
			for (int i = 0; i < count; ++i) {
				arrayData->elements[i + index] = it;
				it += arrayData->elementSize;
			}
			arrayData->count = max(arrayData->count, index + count);
		} /*else if (op == INSERT) {
			int count = header->value;
			for (int i = arrayData->count - 1; i >= index; --i) {
				arrayData->elements[i + count] = arrayData->elements[i];
			}
			for (int i = 0; i < count; ++i) {
				arrayData->elements[i + index] = it;
				it += arrayData->elementSize;
			}
			array->count += count;
		} */else if (op == ERASE) {
			int count = header->value;
			for (int i = index; i < arrayData->count - count; ++i) {
				arrayData->elements[i] = arrayData->elements[i + count];
			}
		}
	}
		
	this->it = it;
}
	
void Storage::write(ArrayData *arrayData, int op, int index, int value) {
	// check if there is enough space
	int size = 1 + (op == OVERWRITE ? arrayData->elementSize * value : 0);
	if (this->it <= this->end - size) {
		// write update to flash
		Header header = {arrayData->index, uint8_t(op), uint8_t(index), uint8_t(value)};
		Flash::write(this->it, (uint32_t*)&header, 1);
		++this->it;
		if (op == OVERWRITE) {
			for (int i = 0; i < value; ++i) {
				Flash::write(this->it, arrayData->elements[index + i], arrayData->elementSize);
				arrayData->elements[index + i] = this->it;
				this->it += arrayData->elementSize;
			}
		}
	} else {
		// switch flash regions
		uint32_t *p = Flash::getAddress(this->pageStart);
		uint8_t pageStart2 = this->pageStart + this->pageCount;
		if (*p != 0xffffffff) {
			// second region becomes current
			this->it = Flash::getAddress(pageStart2);
			this->end = Flash::getAddress(pageStart2 + this->pageCount);
		} else {
			// first region becomes current
			this->it = p;
			this->end = Flash::getAddress(pageStart2);
		}
		
		// write arrays to new flash region
		arrayData = this->first;
		uint32_t *it = this->it;
		while (arrayData != nullptr) {
			if (arrayData != this->first) {
				Header header = {arrayData->index, OVERWRITE, 0, arrayData->count};
				Flash::write(it, (uint32_t*)&header, 1);
			}
			++it;
			
			for (int i = 0; i < arrayData->count; ++i) {
				Flash::write(this->it, arrayData->elements[i], arrayData->elementSize);
				arrayData->elements[i] = this->it;
				it += arrayData->elementSize;
			}

			arrayData = arrayData->next;
		}
		
		// write first header last to make new flash region valid
		Header header = {this->first->index, OVERWRITE, 0, this->first->count};
		Flash::write(this->it, (uint32_t*)&header, 1);
		
		// erase old flash region
		if (this->it != p) {
			// erase first region
			for (int i = 0; i < this->pageCount; ++i) {
				Flash::erase(this->pageStart + i);
			}
		} else {
			// erase second region
			for (int i = 0; i < this->pageCount; ++i) {
				Flash::erase(pageStart2 + i);
			}
		}

		this->it = it;
	}
}
