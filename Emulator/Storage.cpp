#include "util.hpp"
#include "Flash.hpp"
#include "Storage.hpp"


// Storage::ArrayData

void Storage::ArrayData::enlarge(int count) {
	this->count += count;
	
	// find range of elements of following arrays to move
	const void **begin = this->elements + this->count;
	const void **it = begin;
	ArrayData *arrayData = this;
	while ((arrayData = arrayData->next) != nullptr) {
		arrayData->elements = it;
		it += arrayData->count;
	}
	
	// move elements of following arrays
	while (it != begin) {
		--it;
		it[0] = it[-count];
	}
	
	this->storage->elementCount += count;
}

void Storage::ArrayData::shrink(int count) {
	this->count -= count;

	// find range of elements of following arrays to move
	const void **end = this->elements + this->count;
	const void **it = end;
	ArrayData *arrayData = this;
	while ((arrayData = arrayData->next) != nullptr) {
		arrayData->elements = end;
		end += arrayData->count;
	}
	
	// move elements of following arrays
	while (it != end) {
		it[0] = it[count];
		++it;
	}
	
	this->storage->elementCount -= count;
}
/*
void Storage::ArrayData::write(int op, int index, int value) {
	ArrayData *arrayData = this;
	Storage *storage = this->storage;
	
	// calc size of elements (in 32 bit units)
	int size = 1; // for header
	for (int i = 0; i < value; ++i) {
		size += (arrayData->byteSize(arrayData->elements[index + i]) + 3) >> 2;
	}
	
	// check if there is enough space
	//int size = 1 + (op == OVERWRITE ? arrayData->elementSize * value : 0);
	if (storage->it <= storage->end - size) {
		// write header to flash
		Header header = {arrayData->index, uint8_t(index), uint8_t(value), uint8_t(op)};
		Flash::write(storage->it, reinterpret_cast<uint32_t*>(&header), 1);
		++storage->it;
		
		// write data to flash
		if (op == OVERWRITE) {
			for (int i = 0; i < value; ++i) {
				int elementSize = (arrayData->byteSize(arrayData->elements[index + i]) + 3) >> 2;
				Flash::write(storage->it, arrayData->elements[index + i], elementSize);
				arrayData->elements[index + i] = storage->it;
				storage->it += elementSize;
			}
		}
	} else {
		storage->switchFlashRegions();
	}
}
*/
void Storage::ArrayData::write(int index, const void *element) {
	Storage *storage = this->storage;
	
	// automatically enlarge by one if write at end
	if (index == this->count) {
		enlarge(1);
	} else {
		// update used memory size
		storage->elementsSize -= this->byteSize(this->elements[index]);
	}
		
	// update used memory size
	int elementSize = this->byteSize(element);
	storage->elementsSize += elementSize;

	// check if there is enough space
	if (storage->it + elementSize <= storage->end) {
		// write header to flash
		Header header = {this->index, uint8_t(index), uint8_t(1), Op::OVERWRITE};
		storage->it = Flash::write(storage->it, &header, HEADER_SIZE);
		
		// set element in flash
		this->elements[index] = storage->it;

		// write element to flash
		storage->it = Flash::write(storage->it, element, elementSize);
	} else {
		// set element, gets copied to flash in switchFlashRegions()
		this->elements[index] = element;
		
		// defragment flash
		storage->switchFlashRegions();
	}
}

void Storage::ArrayData::erase(int index) {
	Storage *storage = this->storage;
	
	// update used memory size
	storage->elementsSize -= this->byteSize(this->elements[index]);

	// erase element
	for (int i = index; i < this->count - 1; ++i) {
		this->elements[i] = this->elements[i + 1];
	}

	// shrink allocated size of this array by one
	shrink(1);

	// check if there is enough space for a header
	if (storage->it + HEADER_SIZE <= storage->end) {
		// write header to flash
		Header header = {this->index, uint8_t(index), uint8_t(1), Op::ERASE};
		storage->it = Flash::write(storage->it, &header, HEADER_SIZE);
	} else {
		// defragment flash
		storage->switchFlashRegions();
	}
}

void Storage::ArrayData::move(int index, int newIndex) {
	Storage *storage = this->storage;

	// move element
	const void **e = this->elements;
	const void *element = e[index];
	if (index < newIndex) {
		for (int i = index; i < newIndex; ++i) {
			e[i] = e[i + 1];
		}
	} else {
		for (int i = index; i > newIndex; --i) {
			e[i] = e[i - 1];
		}
	}
	e[newIndex] = element;

	// check if there is enough space for a header
	if (storage->it + HEADER_SIZE <= storage->end) {
		// write header to flash
		Header header = {this->index, uint8_t(index), uint8_t(newIndex), Op::MOVE};
		storage->it = Flash::write(storage->it, &header, HEADER_SIZE);
	} else {
		// defragment flash
		storage->switchFlashRegions();
	}
}


// Storage

void Storage::init() {
	// detect current pages
	const uint8_t *p = Flash::getAddress(this->pageStart);
	int pageStart = this->pageStart;
	int pageCount = this->pageCount;
	int pageStart2 = pageStart + pageCount;
	
	// check if op of first header is valid
	if (p[3] != 0xff) {
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
	const uint8_t *it = this->it;
	while (it < this->end && it[3] != 0xff) {
		// get header
		const Header *header = reinterpret_cast<const Header*>(it);
		it += Flash::align(HEADER_SIZE);
		
		// get array by index
		ArrayData *arrayData = this->first;
		int arrayIndex = this->arrayCount - 1;
		while (arrayIndex > header->arrayIndex) {
			--arrayIndex;
			arrayData = arrayData->next;
		};
		
		// execute operation
		Op op = Op(header->op);
		int index = header->index;
		if (op == Op::OVERWRITE) {
			// overwrite array elements, may enlarge the array
			int count = header->value;
			
			// check if we have to enlarge the array
			int newCount = index + count;
			if (newCount > arrayData->count) {
				arrayData->enlarge(newCount - arrayData->count);
			}
			
			// set elements
			for (int i = 0; i < count; ++i) {
				int elementSize = arrayData->byteSize(it);
				arrayData->elements[index + i] = it;
				it += Flash::align(elementSize);
			}
		} else if (op == Op::ERASE) {
			// erase array elements
			int count = header->value;
			for (int i = index; i < arrayData->count - count; ++i) {
				arrayData->elements[i] = arrayData->elements[i + count];
			}
			arrayData->shrink(count);
		} else if (op == Op::MOVE) {
			// move element
			int newIndex = header->value;
			const void **e = arrayData->elements;
			const void *element = e[index];
			if (index < newIndex) {
				for (int i = index; i < newIndex; ++i) {
					e[i] = e[i + 1];
				}
			} else {
				for (int i = index; i > newIndex; --i) {
					e[i] = e[i - 1];
				}
			}
			e[newIndex] = element;
		}
	}
	this->it = it;
	
	// accumulated size of all elements
	ArrayData *arrayData = this->first;
	while (arrayData != nullptr) {
		for (int i = 0; i < arrayData->count; ++i) {
			this->elementsSize += arrayData->byteSize(arrayData->elements[i]);
		}
		arrayData = arrayData->next;
	};
}

bool Storage::hasSpace(int elementCount, int byteSize) const {
	return this->elementCount + elementCount <= ::size(this->elements)
		&& (this->elementCount + 1) * (Flash::align(HEADER_SIZE) + Flash::WRITE_ALIGN) + this->elementsSize + byteSize < this->pageCount * Flash::PAGE_SIZE;
}

void Storage::switchFlashRegions() {
	// switch flash regions
	const uint8_t *p = Flash::getAddress(this->pageStart);
	uint8_t pageStart2 = this->pageStart + this->pageCount;
	if (p[3] != 0xff) {
		// second region becomes current
		this->it = Flash::getAddress(pageStart2);
		this->end = Flash::getAddress(pageStart2 + this->pageCount);
	} else {
		// first region becomes current
		this->it = p;
		this->end = Flash::getAddress(pageStart2);
	}
	
	// write arrays to new flash region
	this->elementCount = 0;
	this->elementsSize = 0;
	ArrayData *arrayData = this->first;
	const uint8_t *it = this->it;
	while (arrayData != nullptr) {
		// write header for whole array (except first header)
		if (arrayData != this->first) {
			Header header = {arrayData->index, 0, arrayData->count, Op::OVERWRITE};
			it = Flash::write(it, &header, HEADER_SIZE);
		} else {
			it += Flash::align(HEADER_SIZE);
		}
		
		// write array data
		for (int i = 0; i < arrayData->count; ++i) {
			int elementSize = arrayData->byteSize(arrayData->elements[i]);
			arrayData->elements[i] = it;
			it = Flash::write(it, arrayData->elements[i], elementSize);
			
			// update element count and accumulated size
			++this->elementCount;
			this->elementsSize = elementSize;
		}

		// next array
		arrayData = arrayData->next;
	}
	
	// write first header last to make new flash region valid
	Header header = {this->first->index, 0, this->first->count, Op::OVERWRITE};
	Flash::write(this->it, &header, HEADER_SIZE);
	
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
