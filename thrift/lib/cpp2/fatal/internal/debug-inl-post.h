/*
 * Copyright 2016 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef THRIFT_FATAL_DEBUG_POST_INL_H_
#define THRIFT_FATAL_DEBUG_POST_INL_H_ 1

#include <thrift/lib/cpp2/fatal/container_traits.h>

#include <thrift/lib/cpp/protocol/TBase64Utils.h>

#include <folly/Conv.h>

#include <fatal/type/enum.h>
#include <fatal/type/search.h>
#include <fatal/type/sort.h>
#include <fatal/type/transform.h>
#include <fatal/type/variant_traits.h>

#include <stdexcept>
#include <type_traits>
#include <utility>

#include <cassert>

namespace apache { namespace thrift { namespace detail {

template <typename> struct debug_equals_impl;

template <typename T, typename Callback>
bool debug_equals(
  std::string &path,
  T const &lhs,
  T const &rhs,
  Callback &&callback
) {
  using impl = detail::debug_equals_impl<
    reflect_type_class<typename std::decay<T>::type>
  >;

  static_assert(
    fatal::is_complete<impl>::value,
    "debug_equals: unsupported type"
  );

  return impl::equals(path, lhs, rhs, callback);
}

struct scoped_path {
 public:
  ~scoped_path() {
    assert(path_.size() >=  size_);
    path_.resize(size_);
  }

  static scoped_path member(std::string &path, folly::StringPiece member) {
    return scoped_path(path, '.', member);
  }

  static scoped_path index(std::string &path, int64_t index) {
    return scoped_path(path, '[', index, ']');
  }

  static scoped_path key(std::string& path, std::string const &key) {
    return scoped_path(path, '[', key, ']');
  }

 private:
   template <typename... Args>
   explicit scoped_path(std::string &path, Args &&... args)
       : size_(path.size()), path_(path) {
     folly::toAppend(std::forward<Args>(args)..., &path);
  }

  std::size_t const size_;
  std::string &path_;
};

template <>
struct debug_equals_impl<type_class::enumeration> {
  template <typename T, typename Callback>
  static bool equals(
    std::string &path,
    T const &lhs,
    T const &rhs,
    Callback &&callback
  ) {
    if (lhs != rhs) {
      callback(
        fatal::enum_to_string(lhs, "<unknown>"),
        fatal::enum_to_string(rhs, "<unknown>"),
        path,
        "value mismatch"
      );
      return false;
    }
    return true;
  }
};

template <typename ValueTypeClass>
struct debug_equals_impl<type_class::list<ValueTypeClass>> {
  template <typename T, typename Callback>
  static bool equals(
    std::string &path,
    T const &lhs,
    T const &rhs,
    Callback &&callback
  ) {
    using traits = thrift_list_traits<T>;

    if (traits::size(lhs) != traits::size(rhs)) {
      callback(lhs, rhs, path, "size mismatch");
      return false;
    }

    auto l = traits::begin(lhs);
    auto const le = traits::end(lhs);
    auto r = traits::begin(rhs);
    auto const re = traits::end(rhs);
    for (std::size_t index = 0; l != le && r != re; ++l, ++r, ++index) {
      auto guard = scoped_path::index(path, index);

      if (!debug_equals_impl<ValueTypeClass>::equals(path, *l, *r, callback)) {
        return false;
      }
    }
    assert(l == le);
    assert(r == re);
    return true;
  }
};

template <typename Type, typename>
struct debug_equals_impl_pretty {
  static std::string go(Type const &v) {
    return pretty_string(v);
  }
};

template <typename Type>
struct debug_equals_impl_pretty<Type, type_class::binary> {
  static std::string go(Type const &v) {
    std::string out;
    out.reserve(4 + v.size() * 2);

    auto const to_hex_digit = [](std::uint8_t const c) {
      return "0123456789ABCDEF"[c & 0xf];
    };

    out.append("\"0x");
    for (auto c: v) {
      out.push_back(to_hex_digit(c >> 4));
      out.push_back(to_hex_digit(c));
    }
    out.push_back('"');

    return out;
  }
};

template <typename KeyTypeClass, typename MappedTypeClass>
struct debug_equals_impl<type_class::map<KeyTypeClass, MappedTypeClass>> {
  template <typename T, typename Callback>
  static bool equals(
    std::string &path,
    T const &lhs,
    T const &rhs,
    Callback &&callback
  ) {
    using traits = thrift_map_traits<T>;
    using key_type = typename traits::key_type;
    using mapped_impl = debug_equals_impl<MappedTypeClass>;
    using pretty_key = debug_equals_impl_pretty<key_type, KeyTypeClass>;

    if (traits::size(lhs) != traits::size(rhs)) {
      callback(lhs, rhs, path, "size mismatch");
    }
    auto const le = traits::end(lhs);
    auto const re = traits::end(rhs);

    bool equals = true;

    for (auto l = traits::begin(lhs); l != le; ++l) {
      auto const &key = traits::key(l);
      auto guard = scoped_path::key(path, pretty_key::go(key));
      if (traits::find(rhs, key) == re) {
        equals = false;
        callback(lhs, rhs, path, "missing");
      }
    }

    for (auto r = traits::begin(rhs); r != re; ++r) {
      auto const& key = traits::key(r);
      auto guard = scoped_path::key(path, pretty_key::go(key));
      if (traits::find(lhs, key) == le) {
        equals = false;
        callback(lhs, rhs, path, "extra");
      }
    }

    for (auto l = traits::begin(lhs); l != le; ++l) {
      auto const& key = traits::key(l);
      auto r = traits::find(rhs, key);
      if (r != re) {
        auto guard = scoped_path::key(path, pretty_key::go(key));
        auto const& lv = traits::mapped(l);
        auto const& rv = traits::mapped(r);
        equals &= mapped_impl::equals(path, lv, rv, callback);
      }
    }

    return equals;
  }
};

template <typename ValueTypeClass>
struct debug_equals_impl<type_class::set<ValueTypeClass>> {
  template <typename T, typename Callback>
  static bool equals(
    std::string &path,
    T const &lhs,
    T const &rhs,
    Callback &&callback
  ) {
    using traits = thrift_set_traits<T>;
    using value_type = typename traits::value_type;
    using pretty_value = debug_equals_impl_pretty<value_type, ValueTypeClass>;

    if (traits::size(lhs) != traits::size(rhs)) {
      callback(lhs, rhs, path, "size mismatch");
    }

    auto const le = traits::end(lhs);
    auto const re = traits::end(rhs);

    bool equals = true;

    for (auto l = traits::begin(lhs); l != le; ++l) {
      auto guard = scoped_path::key(path, pretty_value::go(*l));
      if (traits::find(rhs, *l) == re) {
        equals = false;
        callback(lhs, rhs, path, "missing");
      }
    }

    for (auto r = traits::begin(rhs); r != re; ++r) {
      auto guard = scoped_path::key(path, pretty_value::go(*r));
      if (traits::find(lhs, *r) == le) {
        equals = false;
        callback(lhs, rhs, path, "extra");
      }
    }

    return equals;
  }
};

template <>
struct debug_equals_impl<type_class::variant> {
  template <typename T, typename Callback>
  static bool equals(
    std::string &path,
    T const &lhs,
    T const &rhs,
    Callback &&callback
  ) {
    using namespace fatal;
    using traits = variant_traits<T>;

    if (traits::get_id(lhs) != traits::get_id(rhs)) {
      callback(
        enum_to_string(traits::get_id(lhs), "<unknown>"),
        enum_to_string(traits::get_id(rhs), "<unknown>"),
        path,
        "variant stored type mismatch"
      );
      return false;
    }

    bool result = true;

    sorted_search<
      sort<typename traits::descriptors, less, get_type::id>,
      get_type::id::apply
    >(lhs.getType(), [&](auto indexed) {
      using descriptor = decltype(fatal::tag_type(indexed));

      assert(descriptor::id::value == traits::get_id(lhs));
      assert(descriptor::id::value == traits::get_id(rhs));

      using name = typename descriptor::metadata::name;
      auto guard = scoped_path::member(path, fatal::z_data<name>());

      using type_class = typename descriptor::metadata::type_class;
      typename descriptor::getter getter;
      result = debug_equals_impl<type_class>::equals(
        path, getter(lhs), getter(rhs), callback
      );
    });

    return result;
  }
};

template <>
struct debug_equals_impl<type_class::structure> {
  template <typename T, typename Callback>
  static bool equals(
    std::string &path,
    T const &lhs,
    T const &rhs,
    Callback &&callback
  ) {
    bool result = true;

    fatal::foreach<typename reflect_struct<T>::members>([&](auto indexed) {
      using member = decltype(fatal::tag_type(indexed));
      using getter = typename member::getter;

      auto guard =
          scoped_path::member(path, fatal::z_data<typename member::name>());

      result = debug_equals_impl<typename member::type_class>::equals(
                   path, getter::ref(lhs), getter::ref(rhs), callback) &&
          result;
    });

    return result;
  }
};

template <>
struct debug_equals_impl<type_class::string> {
  template <typename T, typename Callback>
  static bool equals(
    std::string &path,
    T const &lhs,
    T const &rhs,
    Callback &&callback
  ) {
    if (lhs != rhs) {
      callback(lhs, rhs, path, "string mismatch");
      return false;
    }
    return true;
  }
};

template <>
struct debug_equals_impl<type_class::binary> {
  template <typename T, typename Callback>
  static bool equals(
    std::string &path,
    T const &lhs,
    T const &rhs,
    Callback &&callback
  ) {
    if (lhs != rhs) {
      callback(lhs, rhs, path, "binary mismatch");
      return false;
    }
    return true;
  }
};

template <>
struct debug_equals_impl<type_class::floating_point> {
  template <typename T, typename Callback>
  static bool equals(
    std::string &path,
    T const &lhs,
    T const &rhs,
    Callback &&callback
  ) {
    if (lhs != rhs) {
      callback(lhs, rhs, path, "floating point value mismatch");
      return false;
    }
    return true;
  }
};

template <>
struct debug_equals_impl<type_class::integral> {
  template <typename T, typename Callback>
  static bool equals(
    std::string &path,
    T const &lhs,
    T const &rhs,
    Callback &&callback
  ) {
    if (lhs != rhs) {
      callback(lhs, rhs, path, "integral value mismatch");
      return false;
    }
    return true;
  }
};

}}} // apache::thrift::detail

#endif // THRIFT_FATAL_DEBUG_POST_INL_H_
