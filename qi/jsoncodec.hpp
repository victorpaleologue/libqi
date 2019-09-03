#pragma once
/*
**  Copyright (C) 2013 Aldebaran Robotics
**  See COPYING for the license
*/

#ifndef _QI_TYPE_JSONCODEC_HPP_
#define _QI_TYPE_JSONCODEC_HPP_

#include <qi/api.hpp>
#include <qi/anyvalue.hpp>

namespace qi {

  // Do not use enum here because we want to pipe those values and we don't want to cast them each time we pipe them
  using JsonOption = unsigned int;
  const JsonOption JsonOption_None = 0;
  const JsonOption JsonOption_PrettyPrint = 1;
  const JsonOption JsonOption_Expand = 2;

  /** @return the value encoded in JSON.
   * @param val Value to encode
   * @param jsonPrintOption Option to change JSON output
   */
  QI_API std::string encodeJSON(const qi::AutoAnyReference &val, JsonOption jsonPrintOption = JsonOption_None);

  /**
   * @brief Parse error associating a message to a parsing location (line and column).
   */
  struct QI_API ParseError: std::runtime_error
  {
    ParseError(const std::string& reason, size_t line, size_t column);

    /** Line at which the parse error occurred. */
    size_t line() const;

    /** Column at which the parse error occurred. */
    size_t column() const;

  private:
    size_t _line;
    size_t _column;
  };

  /**
    * Creates an AnyValue described with a JSON string.
    * @param in an UTF-8 JSON string to decode.
    * @return a AnyValue corresponding to the JSON description.
    * @throw ParseError if the JSON string could not be parsed.
    */
  QI_API qi::AnyValue decodeJSON(const std::string &in);

  /**
    * Sets an AnyValue to the value described by the JSON UTF-8 sequence between two string iterators.
    * @param begin iterator to the beginning of the sequence to decode.
    * @param end iterator to the end of the sequence to decode.
    * @param target the AnyValue to set. Not modified if an error occured.
    * @return an iterator to the last read char + 1.
    * @throw ParseError if the JSON string could not be parsed.
    */
  QI_API std::string::const_iterator decodeJSON(const std::string::const_iterator &begin,
                                         const std::string::const_iterator &end,
                                         AnyValue &target);



}

#endif  // _QITYPE_JSONCODEC_HPP_
