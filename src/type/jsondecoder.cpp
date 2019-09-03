/*
**  Copyright (C) 2013 Aldebaran Robotics
**  See COPYING for the license
*/

#include <iterator>
#include <stdexcept>
#include <qi/jsoncodec.hpp>
#include <qi/anyvalue.hpp>
#include <boost/lexical_cast.hpp>
#ifdef WITH_BOOST_LOCALE
#  include <boost/locale.hpp>
#endif
#include "jsoncodec_p.hpp"

namespace qi {

  ParseError::ParseError(const std::string& reason, size_t line, size_t column):
      std::runtime_error([&]() {
          std::ostringstream ss;
          ss << "parse error at line " << line << ", column " << column << ": " << reason;
          return ss.str();
        }()),
      _line(line),
      _column(column)
  {}

  size_t ParseError::line() const { return _line; }
  size_t ParseError::column() const { return _column; }

  namespace {
    ParseError makeParseError(const std::string& reason, const detail::DocumentConstIterator& it) {
      return ParseError(reason, it.line(), it.column());
    }
  }

  JsonDecoderPrivate::JsonDecoderPrivate(const std::string &in)
    : _begin(detail::DocumentConstIterator::begin(in)),
      _end(detail::DocumentConstIterator::end(in)),
      _it(detail::DocumentConstIterator::begin(in))
  {}

  JsonDecoderPrivate::JsonDecoderPrivate(
      const std::string::const_iterator &begin,
      const std::string::const_iterator &end)
    : _begin(begin, begin),
      _end(end, begin),
      _it(begin, begin)
  {}

  std::string::const_iterator JsonDecoderPrivate::decode(AnyValue &out)
  {
    _it = _begin;
    if (!decodeValue(out))
      throw makeParseError("unknown", _it);
    return _it.stringIterator();
  }

  void JsonDecoderPrivate::skipWhiteSpaces()
  {
    while (_it != _end && (*_it == ' ' || *_it == '\n'))
    {
      ++_it;
    }
  }

  bool JsonDecoderPrivate::getDigits(std::string &result)
  {
    auto begin = _it;

    while (_it != _end && *_it >= '0' && *_it <= '9')
      ++_it;

    if (_it == begin)
        return false;
    result = std::string(begin.stringIterator(), _it.stringIterator());
    return true;
  }

  bool JsonDecoderPrivate::getInteger(std::string &result)
  {
    auto save = _it;
    std::string integerStr;

    if (_it == _end)
      return false;
    if (*_it == '-')
    {
      ++_it;
      integerStr = "-";
    }
    std::string digitsStr;

    if (!getDigits(digitsStr))
    {
      _it = save;
      return false;
    }
    integerStr += digitsStr;

    result = integerStr;
    return true;
  }

  bool JsonDecoderPrivate::getInteger(qi::int64_t &result)
  {
    std::string integerStr;

    if (!getInteger(integerStr))
      return false;
    result = ::atol(integerStr.c_str());
    return true;
  }

  bool JsonDecoderPrivate::getExponent(std::string &result)
  {
    auto save = _it;

    if (_it == _end || (*_it != 'e' && *_it != 'E'))
      return false;
    ++_it;
    std::string exponentStr;

    exponentStr += 'e';
    if (*_it == '+' || *_it == '-')
    {
      exponentStr += *_it;
      ++_it;
    }
    else
      exponentStr += '+';
    std::string integerStr;
    if (!getDigits(integerStr))
    {
      _it = save;
      return false;
    }

    result = exponentStr + integerStr;
    return true;
  }

  bool JsonDecoderPrivate::getFloat(double &result)
  {
    std::string floatStr;
    std::string beforePoint;
    std::string afterPoint;
    std::string exponent;
    auto save = _it;

    if (!getInteger(beforePoint))
      return false;
    if (!getExponent(exponent))
    {
      if (_it == _end || *_it != '.')
      {
        _it = save;
        return false;
      }

      ++_it;
      if (!getDigits(afterPoint))
      {
        _it = save;
        return false;
      }
      getExponent(exponent);
      floatStr = beforePoint + "." + afterPoint + exponent;
    }
    else
      floatStr = beforePoint + exponent;

    result = boost::lexical_cast<double>(floatStr.c_str());
    return true;
  }

  bool JsonDecoderPrivate::decodeArray(AnyValue &value)
  {
    if (_it == _end || *_it != '[')
      return false;
    ++_it;
    AnyValueVector   tmpArray;

    while (true)
    {
      skipWhiteSpaces();
      AnyValue subElement;
      if (!decodeValue(subElement))
        break;
      tmpArray.push_back(subElement);
      if (*_it != ',')
        break;
      ++_it;
    }
    if (*_it != ']')
      throw makeParseError("unterminated list", _it);

    ++_it;
    value = AnyValue(tmpArray);
    return true;
  }

  bool JsonDecoderPrivate::decodeFloat(AnyValue &value)
  {
    double tmpFloat;

    if (!getFloat(tmpFloat))
      return false;
    value = AnyValue(tmpFloat);
    return true;
  }

  bool JsonDecoderPrivate::decodeInteger(AnyValue &value)
  {
    qi::int64_t tmpInteger;

    if (!getInteger(tmpInteger))
      return false;
    value = AnyValue(tmpInteger);
    return true;
  }

  bool JsonDecoderPrivate::getCleanString(std::string &result)
  {
    if (_it == _end || *_it != '"')
      return false;
    std::string tmpString;

    ++_it;
    for (;_it != _end && *_it != '"';)
    {
      if (*_it == '\\')
      {
        auto nextIt = std::next(_it, 1);
        if (nextIt == _end)
          throw makeParseError("incomplete escape sequence", _it);

        switch (*nextIt)
        {
        case '"' : tmpString += '"' ; std::advance(_it, 2); break;
        case '\\': tmpString += '\\'; std::advance(_it, 2); break;
        case '/' : tmpString += '/' ; std::advance(_it, 2); break;
        case 'b' : tmpString += '\b'; std::advance(_it, 2); break;
        case 'f' : tmpString += '\f'; std::advance(_it, 2); break;
        case 'n' : tmpString += '\n'; std::advance(_it, 2); break;
        case 'r' : tmpString += '\r'; std::advance(_it, 2); break;
        case 't' : tmpString += '\t'; std::advance(_it, 2); break;
#ifdef WITH_BOOST_LOCALE
        case 'u' :
        {
          if (std::distance(_it, _end) <= 6)
            throw makeParseError("incomplete unicode character", _it);

          std::istringstream ss(std::string(_it.stringIterator() + 2, _it.stringIterator() + 6));
          int val;
          ss >> std::hex >> val;
          if (!ss.eof())
            throw makeParseError("malformed unicode character", _it);

          tmpString += boost::locale::conv::utf_to_utf<char>(&val, &val + 1);
          std::advance(_it, 6);
          break;
        }
#endif
        default:
          throw makeParseError("incomplete escape sequence", _it);
        }
      }
      else
      {
        tmpString += *_it;
        ++_it;
      }
    }
    if (_it == _end)
      throw makeParseError("unterminated string", _it);

    ++_it;
    result = tmpString;
    return true;
  }

  bool JsonDecoderPrivate::decodeString(AnyValue &value)
  {
    std::string tmpString;

    if (!getCleanString(tmpString))
      return false;
    value = AnyValue(tmpString);
    return true;
  }

  bool JsonDecoderPrivate::decodeObject(AnyValue &value)
  {
    if (_it == _end || *_it != '{')
      return false;
    ++_it;

    std::map<std::string, AnyValue> tmpMap;
    while (true)
    {
      skipWhiteSpaces();
      std::string key;

      if (!getCleanString(key))
        break;
      skipWhiteSpaces();
      if (_it == _end || *_it != ':')
        throw makeParseError("missing ':' after field", _it);

      ++_it;
      AnyValue tmpValue;
      if (!decodeValue(tmpValue))
        throw makeParseError("failed to decode value", _it);

      if (_it == _end)
        break;
      tmpMap[key] = tmpValue;
      if (*_it != ',')
        break;
      ++_it;
    }
    if (_it == _end || *_it != '}')
      throw makeParseError("unterminated object", _it);

    ++_it;
    value = AnyValue(tmpMap);
    return true;
  }

  bool JsonDecoderPrivate::match(std::string const& expected)
  {
    auto save = _it;
    std::string::const_iterator begin = expected.begin();

    while (_it != _end && begin != expected.end())
    {
      if (*_it != *begin)
      {
        _it = save;
        return false;
      }
      ++_it;
      ++begin;
    }
    if (begin != expected.end())
    {
      _it = save;
      return false;
    }
    return true;
  }

  bool JsonDecoderPrivate::decodeSpecial(AnyValue &value)
  {
    if (_it == _end)
      return false;
    if (match("true"))
      value = AnyValue(true);
    else if (match("false"))
      value = AnyValue::from(false);
    else if (match("null"))
      value = AnyValue(qi::typeOf<void>());
    else
      return false;
    return true;
  }

  bool JsonDecoderPrivate::decodeValue(AnyValue &value)
  {
    skipWhiteSpaces();
    if (decodeSpecial(value)
        || decodeString(value)
        || decodeFloat(value)
        || decodeInteger(value)
        || decodeArray(value)
        || decodeObject(value)
        )
    {
      skipWhiteSpaces();
      return true;
    }
    return false;
  }

  std::string::const_iterator decodeJSON(const std::string::const_iterator &begin,
                                         const std::string::const_iterator &end,
                                         AnyValue &target)
  {
    JsonDecoderPrivate parser(begin, end);
    return parser.decode(target);
  }

  AnyValue decodeJSON(const std::string &in)
  {
    AnyValue value;
    JsonDecoderPrivate parser(in);

    parser.decode(value);
    return value;
  }
}
