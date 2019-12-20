#pragma once

#include "util.hpp"
#include "Flash.hpp"
#include <stdint.h>


class Storage {
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

	/**
	 * Return true if there is space available for the requested size
	 */
	bool hasSpace(int elementCount, int byteSize) const;

	void switchFlashRegions();

protected:

	enum class Op : uint8_t {
		OVERWRITE,
		ERASE,
		MOVE
	};
	
	struct Header {
		// index of array to modify
		uint8_t arrayIndex;
				
		// element index to modify
		uint8_t index;
		
		// operation dependent value
		// OVERWRITE, ERASE: element count
		// MOVE: destination index
		uint8_t value;

		// operation (is last so that it can be used to check if the header was fully written to flash)
		Op op;
	};
	static_assert(sizeof(Header) == 4);
	static const int HEADER_SIZE = sizeof(Header);
	
	struct ArrayData {
		Storage *storage;

		// next array in a linked list
		ArrayData *next;
		
		// index of array
		uint8_t index;
		
		// element size divided by 4
		//uint8_t elementSize;
		
		// number of elements in array
		uint8_t count;
		
		// function to determine the size of an element
		int (*byteSize)(const void *element);
		
		// elements
		const void **elements;

		void enlarge(int count);
		void shrink(int count);
		void write(int index, const void *element);
		void erase(int index);
		void move(int oldIndex, int newIndex);
	};

public:

	template <typename E>
	struct Iterator {
		const E *const *p;
		void operator ++() {++this->p;}
		const E &operator *() const {return **this->p;}
		bool operator !=(Iterator it) {return it.p != this->p;}
	};

	template <typename E>
	class Array {
	public:
		using ELEMENT = E;

		int size() const {
			return this->data.count;
		}
				
		const ELEMENT &operator[](int index) const {
			assert(index >= 0 && index < this->data.count);
			const ELEMENT **e = reinterpret_cast<const ELEMENT**>(this->data.elements);
			return *e[index];
		}

		void write(int index, const ELEMENT &element) {
			assert(index >= 0 && index <= this->data.count);
			this->data.write(index, &element);
		}

		void erase(int index) {
			assert(index >= 0 && index < this->data.count);
			this->data.erase(index);
		}
		
		/**
		 * Move an element to an old index to a new index and move all elements in between to fill the old index and free
		 * the new index
		 */
		void move(int index, int newIndex) {
			assert(index >= 0 && index < this->data.count);
			assert(newIndex >= 0 && newIndex < this->data.count);
			this->data.move(index, newIndex);
		}
		
		Iterator<ELEMENT> begin() const {
			const ELEMENT **elements = reinterpret_cast<const ELEMENT**>(this->data.elements);
			return {elements};
			
		}
		Iterator<ELEMENT> end() const {
			const ELEMENT **elements = reinterpret_cast<const ELEMENT**>(this->data.elements);
			return {elements + this->data.count};
		}

		// only for emulator to set initial configuration
		template <int N>
		void assign(const ELEMENT (&elements)[N]) {
			assert(this->data.count == 0);
			Storage *storage = this->data.storage;

			// enlarge array
			this->data.enlarge(N);
			
			// write header
			Header header = {this->data.index, uint8_t(0), uint8_t(N), Op::OVERWRITE};
			storage->it = Flash::write(storage->it, &header, 1);
			
			// write elements to flash
			for (int i = 0; i < N; ++i) {
				const void *element = &elements[i];
				
				// update used memory size
				int elementSize = this->data.byteSize(element);
				storage->elementsSize += elementSize;

				// set element in flash
				this->data.elements[i] = storage->it;

				// write element to flash
				storage->it = Flash::write(storage->it, element, elementSize);
			}
		}

		ArrayData data;
	};
		
protected:
	
	template <typename E>
	static int byteSize(const void *element) {
		return reinterpret_cast<const E*>(element)->byteSize();
	}

	template <typename T>
	void add(T &array) {
		static_assert(sizeof(typename T::ELEMENT) < 1024);
		
		array.data.storage = this;
		array.data.next = this->first;
		this->first = &array.data;
		array.data.index = this->arrayCount++;
		array.data.count = 0;
		array.data.byteSize = &byteSize<typename T::ELEMENT>;
		array.data.elements = this->elements;
	}

	template <typename T, typename... Arrays>
	void add(T &array, Arrays&... arrays) {
		add(array);
		add(arrays...);
	}

	void init();
	
	// configuration
	uint8_t pageStart;
	uint8_t pageCount;
	uint8_t arrayCount = 0;
	
	// flash pointers
	const uint8_t *it;
	const uint8_t *end;
	
	// list of arrays
	ArrayData *first = nullptr;
	
	// space for array elements of all arrays
	const void *elements[256];
	
	// total number of elements
	int elementCount = 0;
	
	// accumulated size of all elements in 4 byte units
	int elementsSize = 0;
};
