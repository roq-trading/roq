/* Copyright (c) 2017-2021, Hans Erik Thrane */

#pragma once

#include <algorithm>
#include <array>
#include <string>
#include <string_view>
#include <utility>

#include "roq/compat.h"
#include "roq/exceptions.h"
#include "roq/format.h"
#include "roq/literals.h"

namespace roq {

// fixed length string buffer
//! A fixed-length string buffer with automatic conversion to/from
//! \ref std::string_view.
/*!
 * This is useful for managing fixed length arrays as if they were string-like.
 * Typically used to avoid heap allocations, e.g. stack allocated structs used
 * for messaging.

 * This is a higher level abstraction than \ref std::array to provide
 * more * string-specific features.
 * The interface is a subset of \ref std::string and \ref std::string_view.
 */
template <std::size_t N>
class ROQ_PACKED string_buffer final {
 public:
  using value_type = char;

  string_buffer() = default;

  // cppcheck-suppress noExplicitConstructor
  string_buffer(const std::string_view &text) {  // NOLINT (allow implicit)
    copy(text);
  }

  // cppcheck-suppress noExplicitConstructor
  string_buffer(const std::string &text) {  // NOLINT (allow implicit)
    copy(text);
  }

  // cppcheck-suppress noExplicitConstructor
  string_buffer(value_type const *text)  // NOLINT (allow implicit)
      : string_buffer(std::string_view(text)) {}

  string_buffer &operator=(const std::string_view &text) {
    copy(text);
    return *this;
  }

  string_buffer &operator=(const std::string &text) {
    copy(text);
    return *this;
  }

  bool operator==(const string_buffer<N> &rhs) const {
    return static_cast<std::string_view>(*this).compare(static_cast<std::string_view>(rhs)) == 0;
  }

  template <typename... Args>
  int compare(Args &&...args) const {
    return static_cast<std::string_view>(*this).compare(std::forward<Args>(args)...);
  }

  value_type &operator[](size_t index) { return buffer_[index]; }

  value_type operator[](size_t index) const { return buffer_[index]; }

  constexpr std::size_t size() { return N; }

  inline std::size_t length() const {
    auto iter = std::find(buffer_.begin(), buffer_.end(), '\0');
    return iter - buffer_.begin();
  }

  inline bool empty() const { return buffer_[0] == '\0'; }

  value_type const *data() const { return buffer_.data(); }

  operator std::string_view() const { return std::string_view(data(), length()); }

  void clear() {
    // note!
    // we prefer to clear the entire buffer (for security reasons)
    // even though it would be enough to only set the first element
    buffer_.fill('\0');
  }

 protected:
  value_type *data() { return buffer_.data(); }

  void copy(const std::string_view &text) {
    using namespace roq::literals;
    auto len = text.length();
    if (ROQ_LIKELY(len <= size())) {
      auto last = std::copy(text.begin(), text.end(), buffer_.begin());
      std::fill(last, buffer_.end(), '\0');
    } else {
      throw LengthErrorException(R"(can't copy: len(text="{}")={} exceeds size={})"_sv, text, len, size());
    }
  }

 private:
  std::array<value_type, N> buffer_ = {};
};

template <std::size_t N>
inline bool operator==(const string_buffer<N> &lhs, const string_buffer<N> &rhs) {
  return lhs.operator==()(rhs);
}

}  // namespace roq

template <size_t N>
struct fmt::formatter<roq::string_buffer<N> > : public roq::formatter {
  template <typename Context>
  auto format(const roq::string_buffer<N> &value, Context &ctx) {
    using namespace roq::literals;
    return roq::format_to(ctx.out(), "{}"_sv, static_cast<std::string_view>(value));
  }
};
