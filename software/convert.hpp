#pragma once

#include <stdint.h>
#include <optional.hpp>


/**
 * Convert string to integer
 * @return optional int, defined if conversion was successful
 */
optional<int> toInt(char const *str, int length);

/**
 * Convert string to floating point number in the form x.y without support for exponential notation
 * @return optional floating point number, defined if conversion was successful
 */
optional<float> toFloat(char const *str, int length);

/**
 * Convert a 32 bit unsigned integer to string
 * @param str string buffer to convert into
 * @param length length of string buffer
 * @param value value to convert
 * @param digitCount minimum number of digits to convert, pad smaller numbers with leading zeros
 * @return actual length of string
 */
int toString(char *str, int length, uint32_t value, int digitCount = 1);

/**
 * Convert a 32 bit unsigned integer to hex string
 * @param str string buffer to convert into
 * @param length length of string buffer
 * @param value value to convert
 * @param digitCount number of hex digits to convert
 * @return actual length of string
*/
int hexToString(char *str, int length, uint32_t value, int digitCount);

int toString(char *str, int length, float value, int digitCount, int decimalCount);
