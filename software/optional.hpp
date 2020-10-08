#pragma once

#include <cstddef>


template <typename T>
class optional {
public:
  bool valid;
  T value;

  optional(std::nullptr_t p = nullptr) : valid(false) {}

  optional(T value) : valid(true), value(value) {
  }

  T const &operator *() const {
     return this->value;
  }

  T &operator *() {
     return this->value;
  }

  T const *operator ->() const {
     return &this->value;
  }

  T *operator ->() {
     return &this->value;
  }

  optional<T> &operator =(std::nullptr_t p) {
     this->valid = false;
     return *this;
  }

  optional<T> &operator =(T value) {
     this->valid = true;
     this->value = value;
     return *this;
  }

  operator bool () const {
     return this->valid;
  }
};
