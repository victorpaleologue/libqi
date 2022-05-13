#pragma once
/*
**  Copyright (C) 2012 Aldebaran Robotics
**  See COPYING for the license
*/

#ifndef _JSONPARSER_P_HPP_
# define _JSONPARSER_P_HPP_

# include <string>
# include <boost/iterator/iterator_facade.hpp>
# include <qi/anyvalue.hpp>

namespace qi {
namespace detail {
  /**
   * @brief An iterator for text documents,
   * with line and column information,
   * and that automatically deals with new lines (LF only).
   */
  struct DocumentConstIterator: boost::iterator_facade<
      DocumentConstIterator,
      const char,
      boost::forward_traversal_tag>
  {
    DocumentConstIterator(
        std::string::const_iterator it,
        std::string::const_iterator begin)
      : _begin(begin)
      , _it(begin)
      , _line(1u)
      , _column(1u)
    {
      // Iterate from the beginning to count lines and columns.
      while (_it != it) {
        ++*this;
      }
    }

    /// Makes an iterator pointing at the beginning of the string.
    static DocumentConstIterator begin(const std::string& str) {
      return DocumentConstIterator(str.begin(), str.begin());
    }

    /// Makes an iterator pointing at the end of the string.
    static DocumentConstIterator end(const std::string& str) {
      return DocumentConstIterator(str.end(), str.begin());
    }

    /// Iterator in the input string.
    std::string::const_iterator stringIterator() const {
      return _it;
    }

    /// Line number.
    size_t line() const {
      return _line;
    }

    /// Column number.
    size_t column() const {
      return _column;
    }

    // Methods for boost iterator facade.
    const char& dereference() const {
      return *_it;
    }

    bool equal(const DocumentConstIterator& jt) const {
      bool isEqual = _begin == jt._begin && _it == jt._it;
      if (isEqual) {
        QI_ASSERT(_line == jt._line && _column == jt._column);
      }
      return isEqual;
    }

    void increment() {
      // New line resets column and increment line.
      if (*_it == '\n') {
        ++_line;
        _column = 1u;
      }

      // Other characters contribute to the column count,
      // except UTF-8 continuation characters.
      // See https://stackoverflow.com/questions/3586923/counting-unicode-characters-in-c
      else if ((*_it & 0xc0) != 0x80) {
        ++_column;
      }

      ++_it;
    }

    ptrdiff_t distance(const DocumentConstIterator& jt) const {
      return std::distance(this->_it, jt._it);
    }

  private:
    std::string::const_iterator _begin;
    std::string::const_iterator _it;
    size_t _line;
    size_t _column;
  };

} // namespace detail

  class JsonDecoderPrivate
  {
  public:
    JsonDecoderPrivate(const std::string &in);
    JsonDecoderPrivate(const std::string::const_iterator &begin,
                      const std::string::const_iterator &end);
    std::string::const_iterator decode(AnyValue &out);

  private:
    void skipWhiteSpaces();
    bool getDigits(std::string &result);
    bool getInteger(std::string &result);
    bool getInteger(qi::int64_t &result);
    bool getExponent(std::string &result);
    bool getFloat(double &result);
    bool decodeArray(AnyValue &value);
    bool decodeFloat(AnyValue &value);
    bool decodeInteger(AnyValue &value);
    bool getCleanString(std::string &result);
    bool decodeString(AnyValue &value);
    bool decodeObject(AnyValue &value);
    bool match(std::string const& expected);
    bool decodeSpecial(AnyValue &value);
    bool decodeValue(AnyValue &value);

  private:
    detail::DocumentConstIterator const _begin;
    detail::DocumentConstIterator const _end;
    detail::DocumentConstIterator _it;
  };
} // namespace qi

#endif  // _JSONPARSER_P_HPP_
