/**
 * Autogenerated by Thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#pragma once

#include "thrift/compiler/test/fixtures/mcpp2-compare/gen-cpp2/module_types.h"
#include <thrift/lib/cpp2/protocol/Protocol.h>
namespace cpp2 {

struct module_constants {

  static constexpr int8_t const aByte_ = static_cast<int8_t>(1);

  static constexpr int8_t aByte() {
    return aByte_;
  }

  static constexpr int16_t const a16BitInt_ = static_cast<int16_t>(12);

  static constexpr int16_t a16BitInt() {
    return a16BitInt_;
  }

  static constexpr int32_t const a32BitInt_ = 123;

  static constexpr int32_t a32BitInt() {
    return a32BitInt_;
  }

  static constexpr int64_t const a64BitInt_ = 1234LL;

  static constexpr int64_t a64BitInt() {
    return a64BitInt_;
  }

  static constexpr float const aFloat_ = 0.1;

  static constexpr float aFloat() {
    return aFloat_;
  }

  static constexpr double const aDouble_ = 0.12;

  static constexpr double aDouble() {
    return aDouble_;
  }

  // consider using folly::StringPiece instead of std::string whenever possible
  // to referencing this statically allocated string constant, in order to
  // prevent unnecessary allocations

  static constexpr char const * const aString_ = "Joe Doe";

  static constexpr char const * aString() {
    return aString_;
  }

};

} // cpp2
