#pragma once

#include <stdint.h>


class Storage {
	template <typename E, int C>
	friend class Array;

public:

	/**
	 * Constructor
	 * @param pageStart first flash page to use
	 * @pageCount number of flash pages to use
	 * @arrays arrays that use this flash storage
	 */
	template <typename... Arrays>
	Storage(uint8_t pageStart, uint8_t pageCount, Arrays&... arrays) {
		pageCount >>= 1;
		this->pageStart = pageStart;
		this->pageCount = pageCount;

		add(arrays...);
		
		init();
	}
	
	enum Op {
		OVERWRITE,
		//INSERT,
		ERASE,
		MOVE
	};
	
	struct Header {
		uint8_t arrayIndex;
		uint8_t op;
		uint8_t index;
		uint8_t value;
	};

protected:

	struct ArrayData {
		ArrayData *next;
		uint8_t index;
		uint8_t elementSize; // divided by 4
		uint8_t maxCount;
		uint8_t count;
		uint32_t **elements;
	};

	template <typename T>
	void add(T &array) {
		array.storage = this;
		array.data.next = this->first;
		this->first = &array.data;
		array.data.index = this->count++;
		array.data.elementSize = (sizeof(typename T::ELEMENT) + 3) / 4;
		array.data.maxCount = T::MAX_COUNT;
		array.data.count = 0;
		array.data.elements = (uint32_t**)array.elements;
	}

	template <typename T, typename... Arrays>
	void add(T &array, Arrays&... arrays) {
		add(array);
		add(arrays...);
	}

	void init();
	
	void write(ArrayData *arrayData, int op, int index, int value);

	uint8_t pageStart;
	uint8_t pageCount;
	uint8_t count = 0;
	uint32_t *it;
	uint32_t *end;
	ArrayData *first = nullptr;
};


template <typename E, int C>
class Array {
public:
	using ELEMENT = E;
	static const int MAX_COUNT = C;

	int size() const {
		return this->data.count;
	}
	
	const ELEMENT &get(int index) const {
		return *this->elements[index];
	}
	
	void write(int index, const ELEMENT &element) {
		if (index == this->data.count && index < MAX_COUNT)
			this->data.count = index + 1;
		if (index < this->data.count) {
			// write element
			this->elements[index] = &element;
			this->storage->write(&this->data, Storage::OVERWRITE, index, 1);
		}
	}
/*
	void write(int index, const ELEMENT *elements, int count) {
		if (index <= this->array.count) {
			// write elements
			for (int i = 0; i < count; ++i) {
				this->elements[index + i] = elements[i];
			}
			this->storage.write(&this->array, Storage::OVERWRITE, index, count);
			this->array.count = max(this->array.count, index + count);
		}
	}
*/
	template <int N>
	void assign(const ELEMENT (&elements)[N]) {
		// write elements
		for (int i = 0; i < N; ++i) {
			this->elements[i] = &elements[i];
		}
		this->storage->write(&this->data, Storage::OVERWRITE, 0, N);
		if (this->data.count > N)
			this->storage->write(&this->data, Storage::ERASE, this->data.count, this->data.count - N);
		this->data.count = N;
	}

	void erase(int index) {
		if (index < this->data.count) {
			// erase element
			for (int i = index; i < this->data.count - 1; ++i) {
				this->elements[i] = this->elements[i + 1];
			}
			--this->data.count;

			// update flash storage
			this->storage->write(&this->data, Storage::ERASE, index, 1);
		}
	}
	
	void move(int index, int newIndex) {
		if (index < this->data.count && newIndex < this->data.count && index != newIndex) {
			// move element
			ELEMENT *e = this->elements[index];
			if (index < newIndex) {
				for (int i = index; i < newIndex; ++i) {
					this->elements[i] = this->elements[i + 1];
				}
			} else {
				for (int i = index; i > newIndex; --i) {
					this->elements[i] = this->elements[i - 1];
				}
			}
			this->elements[newIndex] = e;
			
			// update flash storage
			this->storage->write(&this->array, Storage::MOVE, index, newIndex);
		}
	}
	
	Storage *storage;
	Storage::ArrayData data;
	const ELEMENT *elements[MAX_COUNT];
};