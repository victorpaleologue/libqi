/*
 *  Author(s):
 *  - Chris  Kilner <ckilner@aldebaran-robotics.com>
 *  - Cedric Gestes <gestes@aldebaran-robotics.com>
 *  - Herve Cuche <hcuche@aldebaran-robotics.com>
 *
 *  Copyright (C) 2010, 2011 Aldebaran Robotics
 */

/** @file
 *  @brief Convenient log macro
 */


#pragma once
#ifndef _LIBQI_QI_LOG_HPP_
#define _LIBQI_QI_LOG_HPP_

# include <map>
# include <string>
# include <iostream>
# include <sstream>
# include <cstdarg>
# include <cstdio>

#include <boost/function/function_fwd.hpp>

#include <qi/config.hpp>
#include <qi/os.hpp>


/**
 * \def qiLogDebug
 *  Log in debug mode. Not compile on release.
 */
#if defined(NO_QI_DEBUG) || defined(NDEBUG)
# define qiLogDebug(...)        if (false) qi::log::detail::NullStream(__VA_ARGS__).self()
#else
# define qiLogDebug(...)        qi::log::LogStream(qi::log::debug, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__).self()
#endif

/**
 * \def qiLogVerbose
 *  Log in verbose mode. This mode isn't show by default but always compile.
 */
#ifdef NO_QI_VERBOSE
# define qiLogVerbose(...)      if (false) qi::log::detail::NullStream(__VA_ARGS__).self()
#else
# define qiLogVerbose(...)      qi::log::LogStream(qi::log::verbose, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__).self()
#endif

/**
 * \def qiLogInfo
 *  Log in info mode.
 */
#ifdef NO_QI_INFO
# define qiLogInfo(...)         if (false) qi::log::detail::NullStream(__VA_ARGS__).self()
#else
# define qiLogInfo(...)         qi::log::LogStream(qi::log::info, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__).self()
#endif

/**
 * \def qiLogWarning
 *  Log in warning mode.
 */
#ifdef NO_QI_WARNING
# define qiLogWarning(...)      if (false) qi::log::detail::NullStream(__VA_ARGS__).self()
#else
# define qiLogWarning(...)      qi::log::LogStream(qi::log::warning, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__).self()
#endif

/**
 * \def qiLogError
 *  Log in error mode.
 */
#ifdef NO_QI_ERROR
# define qiLogError(...)        if (false) qi::log::detail::NullStream(__VA_ARGS__).self()
#else
# define qiLogError(...)        qi::log::LogStream(qi::log::error, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__).self()
#endif

/**
 * \def qiLogFatal
 *  Log in fatal mode.
 */
#ifdef NO_QI_FATAL
# define qiLogFatal(...)        if (false) qi::log::detail::NullStream(__VA_ARGS__).self()
#else
# define qiLogFatal(...)        qi::log::LogStream(qi::log::fatal, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__).self()
#endif

/**
 * \namespace qi::log
 * \brief log implementation.
 */
namespace qi {
  namespace log {

    namespace detail {

      // simple NullStream that do nothing
      class NullStream {
      public:
        NullStream(const char *, ...)
        {
        }

        NullStream &self()
        {
          return *this;
        }

        template <typename T>
        NullStream& operator<<(const T& val)
        {
          return self();
        }

        NullStream& operator<<(std::ostream& (*f)(std::ostream&))
        {
          return self();
        }

      };
    };


    /**
     * \enum LogLevel
     * \brief seven log levels display.
     */
    enum QI_API LogLevel {
        silent = 0,
        fatal,
        error,
        warning,
        info,
        verbose,
        debug,
        };

    /**
     * \typedef logFuncHandler
     * \brief Boost delegate to log function (verb, category,
     *        message, file, function, line, date).
     */
    typedef boost::function7<void,
                             const qi::log::LogLevel,
                             const qi::os::timeval,
                             const char*,
                             const char*,
                             const char*,
                             const char*,
                             int> logFuncHandler;


    QI_API void init(qi::log::LogLevel verb = qi::log::info,
                     int ctx = 0,
                     bool synchronous = true);


    /**
     * \brief Log function
     *
     * You should call qiLog* macro.
     *
     * @param verb { debug = 6, verbose=5, info = 4, warning = 3, error = 2, fatal = 1, silent = 0 }
     * @param category Log category.
     * @param msg Log message.
     * @param file __FILE__
     * @param function __FUNCTION__
     * @param line __LINE__
     */
    QI_API void log(const qi::log::LogLevel verb,
                    const char              *category,
                    const char              *msg,
                    const char              *file = "",
                    const char              *fct = "",
                    const int               line = 0);


    /**
     * \brief Convert log verbosity to char*
     * @param verb { debug = 6, verbose=5, info = 4, warning = 3, error = 2, fatal = 1, silent = 0 }
     *
     * \return [SILENT], [FATAL], [ERROR],
     *         [WARN ], [INFO ], [VERB ],
     *         [DEBUG]
     */
    QI_API const char* logLevelToString(const qi::log::LogLevel verb);

    /**
     * \brief Convert string to log verbosity
     * @param verb debug, verbose, info,
     *             warning, error, fatal,
     *             silent
     *
     * \return Log level verbosity
     */
    QI_API const qi::log::LogLevel stringToLogLevel(const char* verb);


    /**
     * \brief Set log verbosity.
     *
     * If you don't want any log use silent mode.
     *
     * @param lv maximal verbosity shown
     */
    QI_API void setVerbosity(const qi::log::LogLevel lv);

    /**
     * \brief Get log verbosity.
     * @return Maximal verbosity display.
     */
    QI_API qi::log::LogLevel verbosity();


    /**
     * \brief Set log context.
     *
     * Display log context (line, function, file).
     *
     * @param ctx Value to set context.
     *            0: none, 1: categories, 2: date, 3: file+line,
     *            4: date+categories, 5: date+line+file,
     *            6: categories+line+file,
     *            7: all (date+categories+line+file+function)
     */
    QI_API void setContext(int ctx);

    /**
     * \brief Get log context.
     * @return true if active, false otherwise.
     */
    QI_API int context();


    /**
     * \brief Set synchronous logs.
     *
     * @param sync Value to set context.
     */
    QI_API void setSynchronousLog(bool sync);



    /**
     * \brief Add log handler.
     *
     * @param fct Boost delegate to log handler function.
     * @param name name of the handler, this is the one used to remove handler (prefer lowcase).
     */
    QI_API void addLogHandler(const std::string& name,
                              qi::log::logFuncHandler fct);

    /**
     * \brief remove log handler.
     *
     * @param name name of the handler.
     */
    QI_API void removeLogHandler(const std::string& name);

    /**
     * \brief flush asynchronous log.
     */
    QI_API void flush();

    /** \class LogStream log.hpp "qi/log.hpp"
     */
    class LogStream: public std::stringstream
    {
    public:

      /**
       * \brief LogStream. Copy Ctor.
       * @param rhs LogStream.
       */
      LogStream(const LogStream &rhs)
        : _logLevel(rhs._logLevel)
        , _category(rhs._category)
        , _file(rhs._file)
        , _function(rhs._function)
        , _line(rhs._line)
      {
      }

      /**
       * \brief LogStream assignment operator.
       * @param rhs LogStream.
       */
      LogStream &operator=(const LogStream &rhs)
      {
        _logLevel = rhs._logLevel;
        _category = rhs._category;
        _file     = rhs._file;
        _function = rhs._function;
        _line     = rhs._line;
        return *this;
      }

      /**
       * \brief LogStream. Will log at object destruction
       * @param level { debug = 6, verbose=5, info = 4, warning = 3, error = 2, fatal = 1, silent = 0 }
       * @param file __FILE__
       * @param function __FUNCTION__
       * @param line __LINE__
       * @param category log category
       */
      LogStream(const LogLevel    level,
                const char        *file,
                const char        *function,
                const int         line,
                const char        *category)
        : _logLevel(level)
        , _category(category)
        , _file(file)
        , _function(function)
        , _line(line)
      {
      }

      /**
       * \brief LogStream. Will log at object destruction
       * @param level { debug = 6, verbose=5, info = 4, warning = 3, error = 2, fatal = 1, silent = 0 }
       * @param file __FILE__
       * @param function __FUNCTION__
       * @param line __LINE__
       * @param category log category
       * @param fmt message format.
       */
      LogStream(const LogLevel    level,
                const char        *file,
                const char        *function,
                const int         line,
                const char        *category,
                const char        *fmt, ...)
        : _logLevel(level)
        , _category(category)
        , _file(file)
        , _function(function)
        , _line(line)
      {
        char buffer[2048];
        va_list vl;
        va_start(vl, fmt);
       #ifdef _MSC_VER
        vsnprintf_s(buffer, 2048, 2047, fmt, vl);
       #else
        vsnprintf(buffer, 2048, fmt, vl);
       #endif
        buffer[2047] = 0;
        va_end(vl);
        *this << buffer;
      }

      /** \brief Destructor */
      ~LogStream()
      {
        qi::log::log(_logLevel, _category, this->str().c_str(), _file, _function, _line);
      }

      /** \brief Necessary to work with an anonymous object */
      LogStream& self() {
        return *this;
      }

    private:
      LogLevel    _logLevel;
      const char *_category;
      const char *_file;
      const char *_function;
      int         _line;
    };
  }
}

#endif  // _LIBQI_QI_LOG_HPP_
