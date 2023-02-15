## Copyright (c) 2012, 2013, 2014 Aldebaran Robotics. All rights reserved.
## Use of this source code is governed by a BSD-style license that can be
## found in the COPYING file.
cmake_minimum_required(VERSION 3.10)
project(Qi)

# This variant of libQi is meant to be maximally portable.
# It builds OpenSSL and Boost locally, to match closely your toolchain.
# Libraries are all static, and statically linked.
# Several components were not ported.
# See the README.rst for differences with the official libQi.

# libQi does not compile with C++17 and above.
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

if(NOT COMMAND FetchContent_Declare)
  include(FetchContent)
endif()
if(NOT COMMAND ExternalProject_Add)
  include(ExternalProject)
endif()
set(EXTERNALPROJECT_QUIET FALSE)

# OpenSSL
# =======
set(OPENSSL_SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/openssl-src)
set(OPENSSL_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/openssl)
set(OPENSSL_INCLUDE_DIR ${OPENSSL_INSTALL_DIR}/include)
if(NOT APPLE) # On Mac, we cannot build OpenSSL statically: https://stackoverflow.com/a/3801032/3836562
  set(OPENSSL_MAYBE_STATIC -static)
  set(OPENSSL_LIBRARY_SUFFIX a)
else()
  set(OPENSSL_LIBRARY_SUFFIX dylib)
endif()
if(CROSSCOMPILING_TARGET_PLATFORM) # This is a custom variable to be set by a custom toolchain
  set(OPENSSL_CROSSCOMPILATION_PREFIX "--cross-compile-prefix=${CROSSCOMPILING_TARGET_PLATFORM}-")
  set(OPENSSL_CROSSCOMPILATION_TOOLSET ${CROSSCOMPILING_TARGET_SYSTEM}-${CROSSCOMPILING_TARGET_ARCHITECTURE})
  set(OPENSSL_CONFIGURE_COMMAND ${OPENSSL_SOURCE_DIR}/Configure ${OPENSSL_CROSSCOMPILATION_TOOLSET} ${OPENSSL_CROSSCOMPILATION_PREFIX})
else()
  set(OPENSSL_CONFIGURE_COMMAND ${OPENSSL_SOURCE_DIR}/config)
endif()
ExternalProject_Add(
  OpenSSL
  SOURCE_DIR ${OPENSSL_SOURCE_DIR}
  GIT_REPOSITORY https://github.com/openssl/openssl.git
  GIT_TAG OpenSSL_1_1_1n
  USES_TERMINAL_DOWNLOAD TRUE
  CONFIGURE_COMMAND
    ${OPENSSL_CONFIGURE_COMMAND}
    ${OPENSSL_MAYBE_STATIC}
    --prefix=${OPENSSL_INSTALL_DIR}
    --openssldir=${OPENSSL_INSTALL_DIR}
  BUILD_COMMAND make
  TEST_COMMAND ""
  INSTALL_COMMAND make install_sw
  INSTALL_DIR ${OPENSSL_INSTALL_DIR}
)
# We cannot use find_library because ExternalProject_Add() is performed at build time.
# And to please the property INTERFACE_INCLUDE_DIRECTORIES,
# we make the include directory in advance.
file(MAKE_DIRECTORY ${OPENSSL_INCLUDE_DIR})

add_library(OpenSSL::SSL STATIC IMPORTED GLOBAL)
set_property(TARGET OpenSSL::SSL PROPERTY IMPORTED_LOCATION ${OPENSSL_INSTALL_DIR}/lib/libssl.${OPENSSL_LIBRARY_SUFFIX})
set_property(TARGET OpenSSL::SSL PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${OPENSSL_INCLUDE_DIR})
add_dependencies(OpenSSL::SSL OpenSSL)

add_library(OpenSSL::Crypto STATIC IMPORTED GLOBAL)
set_property(TARGET OpenSSL::Crypto PROPERTY IMPORTED_LOCATION ${OPENSSL_INSTALL_DIR}/lib/libcrypto.${OPENSSL_LIBRARY_SUFFIX})
set_property(TARGET OpenSSL::Crypto PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${OPENSSL_INCLUDE_DIR})
add_dependencies(OpenSSL::Crypto OpenSSL)

# Boost
# =====
set(BOOST_VERSION 1.69.0)
set(BOOST_SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/boost-src)
set(BOOST_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/boost-build)
set(BOOST_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/boost)
if(CMAKE_CROSSCOMPILING)
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(BOOST_TARGET_PLATFORM_ARG "target-os=linux")
  endif()
  file(WRITE ${BOOST_BUILD_DIR}/user-config.jam "using ${CROSSCOMPILING_COMPILER} : ${CROSSCOMPILING_TARGET_ARCHITECTURE} : ${CMAKE_CXX_COMPILER} ;")
  set(BOOST_USER_CONFIG "${BOOST_BUILD_DIR}/user-config.jam")
  set(BOOST_CROSSCOMPILATION_ARGS "toolset=${CROSSCOMPILING_COMPILER}-${CROSSCOMPILING_TARGET_ARCHITECTURE}" ${BOOST_TARGET_PLATFORM_ARG})
endif()
ExternalProject_Add(
  Boost
  SOURCE_DIR ${BOOST_SOURCE_DIR}
  BINARY_DIR ${BOOST_BUILD_DIR}
  GIT_REPOSITORY https://github.com/boostorg/boost
  GIT_TAG boost-${BOOST_VERSION}
  USES_TERMINAL_DOWNLOAD TRUE
  PATCH_COMMAND patch -uN "${BOOST_SOURCE_DIR}/tools/build/src/tools/darwin.jam" "${CMAKE_CURRENT_SOURCE_DIR}/boost-issue-440-darwin-version.patch" || true # Fixes compilation for Mac OS.
  CONFIGURE_COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/boost-configure.sh" ${BOOST_SOURCE_DIR} ${BOOST_USER_CONFIG}
  BUILD_COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/boost-build.sh" ${BOOST_SOURCE_DIR} ${BOOST_BUILD_DIR} ${BOOST_INSTALL_DIR} ${BOOST_CROSSCOMPILATION_ARGS}
  TEST_COMMAND ""
  INSTALL_COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/boost-install.sh" ${BOOST_SOURCE_DIR} ${BOOST_BUILD_DIR} ${BOOST_INSTALL_DIR} ${BOOST_CROSSCOMPILATION_ARGS}
  INSTALL_DIR ${BOOST_INSTALL_DIR}
)
# We cannot use find_library because ExternalProject_Add() is performed at build time.
# And to please the property INTERFACE_INCLUDE_DIRECTORIES,
# we make the include directory in advance.
set(BOOST_FOUND TRUE)
set(BOOST_PACKAGE_FOUND TRUE)
set(Boost_INCLUDE_DIR ${BOOST_INSTALL_DIR}/include)
file(MAKE_DIRECTORY ${Boost_INCLUDE_DIR})
add_library(Boost::headers INTERFACE IMPORTED GLOBAL)
set_property(TARGET Boost::headers PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${Boost_INCLUDE_DIR})

add_library(Boost::atomic INTERFACE IMPORTED GLOBAL)
set_property(TARGET Boost::atomic PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${Boost_INCLUDE_DIR})

add_library(Boost::chrono STATIC IMPORTED GLOBAL)
set_property(TARGET Boost::chrono PROPERTY IMPORTED_LOCATION ${BOOST_INSTALL_DIR}/lib/libboost_chrono.a)
set_property(TARGET Boost::chrono PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${Boost_INCLUDE_DIR})

add_library(Boost::date_time STATIC IMPORTED GLOBAL)
set_property(TARGET Boost::date_time PROPERTY IMPORTED_LOCATION ${BOOST_INSTALL_DIR}/lib/libboost_date_time.a)
set_property(TARGET Boost::date_time PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${Boost_INCLUDE_DIR})

add_library(Boost::filesystem STATIC IMPORTED GLOBAL)
set_property(TARGET Boost::filesystem PROPERTY IMPORTED_LOCATION ${BOOST_INSTALL_DIR}/lib/libboost_filesystem.a)
set_property(TARGET Boost::filesystem PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${Boost_INCLUDE_DIR})

add_library(Boost::program_options STATIC IMPORTED GLOBAL)
set_property(TARGET Boost::program_options PROPERTY IMPORTED_LOCATION ${BOOST_INSTALL_DIR}/lib/libboost_program_options.a)
set_property(TARGET Boost::program_options PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${Boost_INCLUDE_DIR})

add_library(Boost::random STATIC IMPORTED GLOBAL)
set_property(TARGET Boost::random PROPERTY IMPORTED_LOCATION ${BOOST_INSTALL_DIR}/lib/libboost_random.a)
set_property(TARGET Boost::random PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${Boost_INCLUDE_DIR})

add_library(Boost::regex STATIC IMPORTED GLOBAL)
set_property(TARGET Boost::regex PROPERTY IMPORTED_LOCATION ${BOOST_INSTALL_DIR}/lib/libboost_regex.a)
set_property(TARGET Boost::regex PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${Boost_INCLUDE_DIR})

add_library(Boost::thread STATIC IMPORTED GLOBAL)
set_property(TARGET Boost::thread PROPERTY IMPORTED_LOCATION ${BOOST_INSTALL_DIR}/lib/libboost_thread.a)
set_property(TARGET Boost::thread PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${Boost_INCLUDE_DIR})

# LibQi
# =====
include(CheckSymbolExists)
check_symbol_exists(_SC_HOST_NAME_MAX "unistd.h" HAVE_SC_HOST_NAME_MAX)
set_source_files_properties(src/os_posix.cpp
  PROPERTIES
  COMPILE_DEFINITIONS HAVE_SC_HOST_NAME_MAX
)

#### Set definitions {{{
# We always want boost filesystem v3
add_definitions("-DBOOST_FILESYSTEM_VERSION=3")
add_definitions("-DBOOST_ASIO_DISABLE_STD_CHRONO")

if(MSVC)
  add_definitions(" -DWIN32_LEAN_AND_MEAN ")
endif()

if (WIN32)
  add_definitions(" -D_SCL_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -DWIN32_LEAN_AND_MEAN")
endif()

if (UNIX)
  add_definitions(" -fno-strict-aliasing ")
endif()

if(WITH_BOOST_LOCALE)
  add_definitions(" -DWITH_BOOST_LOCALE ")
endif()

if (WITH_SYSTEMD)
  add_definitions("-DWITH_SYSTEMD")
endif()
#### }}}

#### Set sources {{{
set(QI_H
  qi/detail/async.hxx
  qi/detail/clock.hxx
  qi/detail/eventloop.hxx
  qi/detail/future.hxx
  qi/detail/future_fwd.hpp
  qi/detail/futurebarrier.hpp
  qi/detail/futureunwrap.hpp
  qi/detail/executioncontext.hpp
  qi/detail/log.hxx
  qi/detail/mpl.hpp
  qi/detail/print.hpp
  qi/detail/trackable.hxx
  qi/api.hpp
  qi/application.hpp
  qi/actor.hpp
  qi/async.hpp
  qi/atomic.hpp
  qi/buffer.hpp
  qi/clock.hpp
  qi/either.hpp
  qi/flags.hpp
  qi/future.hpp
  qi/futuregroup.hpp
  qi/log/consoleloghandler.hpp
  qi/log/csvloghandler.hpp
  qi/log/fileloghandler.hpp
  qi/log/headfileloghandler.hpp
  qi/log/tailfileloghandler.hpp
  qi/log.hpp
  qi/macro.hpp
  qi/numeric.hpp
  qi/os.hpp
  qi/getenv.hpp
  qi/preproc.hpp
  qi/types.hpp
  qi/path.hpp
  qi/path_conf.hpp
  qi/periodictask.hpp
  qi/qi.hpp
  qi/registration.hpp
  qi/stats.hpp
  qi/trackable.hpp
  qi/translator.hpp
  qi/eventloop.hpp
  qi/version.hpp
  qi/iocolor.hpp
  qi/strand.hpp
  qi/assert.hpp

  qi/ptruid.hpp
  qi/objectuid.hpp
  qi/tag.hpp
  qi/uuid.hpp
)

set(QI_C
  src/dlfcn.cpp
  src/path.cpp
  src/application.cpp
  src/application_p.hpp
  src/buffer.cpp
  src/buffer_p.hpp
  src/bufferreader.cpp
  src/clock.cpp
  src/sdklayout.hpp
  src/future.cpp
  src/log.cpp
  src/log_p.hpp
  src/consoleloghandler.cpp
  src/fileloghandler.cpp
  src/csvloghandler.cpp
  src/headfileloghandler.cpp
  src/tailfileloghandler.cpp
  src/locale-light.cpp
  src/os.cpp
  src/path_conf.cpp
  src/periodictask.cpp
  src/print.cpp
  src/utils.hpp
  src/utils.cpp
  src/eventloop.cpp
  src/eventloop_p.hpp
  src/sdklayout-boost.cpp
  src/version.cpp
  src/iocolor.cpp
  src/strand.cpp
  src/ptruid.cpp
)

#### Add optional files to source {{{
if (WIN32)
  list(APPEND QI_C
    src/os_launch_win32.cpp
    src/os_win32.cpp
    src/os_win32.hpp
    src/os_debugger_win32.cpp
  )
else()
  list(APPEND QI_C
    src/os_posix.cpp
    src/os_debugger_posix.cpp
    )
  if(ANDROID)
    list(APPEND QI_H qi/log/androidloghandler.hpp)
    list(APPEND QI_C
      src/androidloghandler.cpp
      src/os_launch_android.cpp
    )
  else()
    list(APPEND QI_C src/os_launch_posix.cpp)
  endif()
endif()

if(WITH_BOOST_LOCALE)
  list(APPEND QI_H qi/translator.hpp)
  list(APPEND QI_C src/translator.cpp)
endif()

if (WITH_SYSTEMD)
  list(APPEND QI_H qi/log/journaldloghandler.hpp)
  list(APPEND QI_C src/journaldloghandler.cpp)
endif()

set(BINARY_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/include")
file(MAKE_DIRECTORY ${BINARY_INCLUDE_DIR}/qi)
set(CONFIG_PATH ${BINARY_INCLUDE_DIR}/qi/config.hpp)
configure_file(qi/config.hpp.in ${CONFIG_PATH})
include_directories("${BINARY_INCLUDE_DIR}")
install(
  FILES       "${CONFIG_PATH}"
  COMPONENT   devel
  DESTINATION "include/qi"
)

list(APPEND QI_C ${CONFIG_ABS_PATH})
#### }}}

set(QITYPE_H
  qi/anyfunction.hpp
  qi/anyobject.hpp
  qi/signature.hpp
  qi/property.hpp
  qi/signal.hpp
  qi/signalspy.hpp
  qi/anyvalue.hpp
  qi/anymodule.hpp

  qi/type/detail/signal.hxx
  qi/type/detail/property.hxx
  qi/type/detail/accessor.hxx
  qi/type/detail/anyreference.hpp
  qi/type/detail/anyreference.hxx
  qi/type/detail/anyvalue.hpp
  qi/type/detail/anyvalue.hxx
  qi/type/detail/anyfunction.hxx
  qi/type/detail/anyfunctionfactory.hxx
  qi/type/detail/anyiterator.hpp
  qi/type/detail/anyiterator.hxx
  qi/type/detail/bindtype.hxx
  qi/type/detail/functionsignature.hxx
  qi/type/detail/futureadapter.hxx
  qi/type/detail/futureadapter.hpp
  qi/type/detail/dynamicobjectbuilder.hxx
  qi/type/detail/genericobject.hpp
  qi/type/detail/object.hxx
  qi/type/detail/proxyregister.hpp
  qi/type/detail/hasless.hxx
  qi/type/detail/objecttypebuilder.hxx
  qi/type/detail/type.hxx
  qi/type/detail/buffertypeinterface.hxx
  qi/type/detail/typedispatcher.hxx
  qi/type/detail/dynamictypeinterface.hxx
  qi/type/detail/typeimpl.hxx
  qi/type/detail/typeinterface.hpp
  qi/type/detail/inttypeinterface.hxx
  qi/type/detail/listtypeinterface.hxx
  qi/type/detail/maptypeinterface.hxx
  qi/type/detail/optionaltypeinterface.hxx

  qi/type/detail/pointertypeinterface.hxx
  qi/type/detail/staticobjecttype.hpp
  qi/type/detail/stringtypeinterface.hxx
  qi/type/detail/structtypeinterface.hxx
  qi/type/detail/type.hpp
  qi/type/detail/manageable.hpp
  qi/type/detail/traceanalyzer.hpp

  qi/api.hpp
  qi/binarycodec.hpp
  qi/type/dynamicobject.hpp
  qi/type/dynamicobjectbuilder.hpp
  qi/type/fwd.hpp
  qi/jsoncodec.hpp
  qi/type/metamethod.hpp
  qi/type/metaobject.hpp
  qi/type/metaproperty.hpp
  qi/type/metasignal.hpp
  qi/type/objecttypebuilder.hpp
  qi/type/proxyproperty.hpp
  qi/type/proxysignal.hpp
  qi/type/typeinterface.hpp
  qi/type/typeobject.hpp
  qi/type/typedispatcher.hpp
)



set(QITYPE_C
  src/type/binarycodec.cpp
  src/type/binarycodec_p.hpp
  src/type/dynamicobject.cpp
  src/type/dynamicobjectbuilder.cpp
  src/type/anyfunction.cpp
  src/type/anyreference.cpp
  src/type/anyvalue.cpp
  src/type/anyobject.cpp
  src/type/genericobject.cpp
  src/type/jsoncodec_p.hpp
  src/type/jsondecoder.cpp
  src/type/jsonencoder.cpp
  src/type/manageable.cpp
  src/type/metamethod.cpp
  src/type/metaproperty.cpp
  src/type/metasignal.cpp
  src/type/metasignal_p.cpp
  src/type/metasignal_p.hpp
  src/type/metaobject.cpp
  src/type/metaobject_p.hpp
  src/type/anymodule.cpp
  src/type/objecttypebuilder.cpp
  src/type/signal.cpp
  src/type/signal_p.hpp
  src/type/signalspy.cpp
  src/type/signatureconvertor.cpp
  src/type/signatureconvertor.hpp
  src/type/staticobjecttype.cpp
  src/type/typeinterface.cpp
  src/type/structtypeinterface.cpp
  src/type/type.cpp
  src/type/signature.cpp
  src/type/traceanalyzer.cpp
)


set(QIM_H
  qi/api.hpp
  qi/messaging/authprovider.hpp
  qi/messaging/authproviderfactory.hpp
  qi/messaging/autoservice.hpp
  qi/messaging/clientauthenticator.hpp
  qi/messaging/clientauthenticatorfactory.hpp
  qi/messaging/detail/autoservice.hxx
  qi/messaging/gateway.hpp
  qi/messaging/messagesocket_fwd.hpp
  qi/messaging/servicedirectoryproxy.hpp
  qi/messaging/serviceinfo.hpp
  qi/applicationsession.hpp
  qi/session.hpp
  qi/uri.hpp
  qi/url.hpp
)

set(QIM_C
  src/messaging/applicationsession_internal.hpp
  src/messaging/applicationsession_internal.cpp
  src/messaging/applicationsession.cpp
  src/messaging/authprovider_p.hpp
  src/messaging/authprovider.cpp
  src/messaging/boundobject.cpp
  src/messaging/boundobject.hpp
  src/messaging/clientauthenticator_p.hpp
  src/messaging/clientauthenticator.cpp
  src/messaging/gateway.cpp
  src/messaging/message.hpp
  src/messaging/message.cpp
  src/messaging/messagedispatcher.hpp
  src/messaging/messagedispatcher.cpp
  src/messaging/objecthost.hpp
  src/messaging/objecthost.cpp
  src/messaging/objectregistrar.hpp
  src/messaging/objectregistrar.cpp
  src/messaging/remoteobject.cpp
  src/messaging/remoteobject_p.hpp
  src/messaging/servicedirectory.cpp
  src/messaging/servicedirectory.hpp
  src/messaging/servicedirectoryclient.hpp
  src/messaging/servicedirectoryclient.cpp
  src/messaging/servicedirectoryproxy.cpp
  src/messaging/serviceinfo.cpp
  src/messaging/session.cpp
  src/messaging/session_p.hpp
  src/messaging/sessionservice.hpp
  src/messaging/sessionservice.cpp
  src/messaging/sessionservices.hpp
  src/messaging/sessionservices.cpp
  src/messaging/server.hpp
  src/messaging/server.cpp
  src/messaging/streamcontext.hpp
  src/messaging/streamcontext.cpp
  src/messaging/transportserver.hpp
  src/messaging/transportserver.cpp
  src/messaging/transportserverasio_p.cpp
  src/messaging/transportserverasio_p.hpp
  src/messaging/messagesocket.hpp
  src/messaging/messagesocket.cpp
  src/messaging/transportsocketcache.cpp
  src/messaging/transportsocketcache.hpp
  src/messaging/tcpmessagesocket.cpp
  src/messaging/tcpmessagesocket.hpp
  src/messaging/uri.cpp
  src/messaging/url.cpp
  src/registration.cpp
)

set(QI_SOCKET_H
  src/messaging/sock/accept.hpp
  src/messaging/sock/common.hpp
  src/messaging/sock/concept.hpp
  src/messaging/sock/connect.hpp
  src/messaging/sock/connectedstate.hpp
  src/messaging/sock/connectingstate.hpp
  src/messaging/sock/disconnectedstate.hpp
  src/messaging/sock/disconnectingstate.hpp
  src/messaging/sock/error.hpp
  src/messaging/sock/macrolog.hpp
  src/messaging/sock/socketptr.hpp
  src/messaging/sock/sslcontextptr.hpp
  src/messaging/sock/socketwithcontext.hpp
  src/messaging/sock/networkasio.hpp
  src/messaging/sock/option.hpp
  src/messaging/sock/receive.hpp
  src/messaging/sock/resolve.hpp
  src/messaging/sock/send.hpp
  src/messaging/sock/traits.hpp
)

set(QIPERF_H
  qi/perf/dataperfsuite.hpp
  qi/perf/detail/dataperfsuite.hxx
  qi/perf/dataperf.hpp
  qi/perf/measure.hpp
)

set(QIPERF_C
  src/perf/dataperfsuite_p.hpp
  src/perf/dataperf_p.hpp
  src/perf/dataperfsuite.cpp
  src/perf/dataperf.cpp
  src/perf/measure.cpp
)

set(KA_H
  ka/algorithm.hpp
  ka/base64.hpp
  ka/ark/inputiter.hpp
  ka/ark/mutable.hpp
  ka/concept.hpp
  ka/conceptpredicate.hpp
  ka/empty.hpp
  ka/errorhandling.hpp
  ka/flatten.hpp
  ka/functional.hpp
  ka/functor.hpp
  ka/functorcontainer.hpp
  ka/indexed.hpp
  ka/integersequence.hpp
  ka/iterator.hpp
  ka/macro.hpp
  ka/macroregular.hpp
  ka/memory.hpp
  ka/moveoncopy.hpp
  ka/mutablestore.hpp
  ka/mutex.hpp
  ka/opt.hpp
  ka/productiter.hpp
  ka/range.hpp
  ka/relationpredicate.hpp
  ka/scoped.hpp
  ka/sha1.hpp
  ka/src.hpp
  ka/testutils.hpp
  ka/typetraits.hpp
  ka/unit.hpp
  ka/uri.hpp
  ka/uri/io.hpp
  ka/uri/uri_fwd.hpp
  ka/uri/uri.hpp
  ka/uri/parsing.hpp
  ka/utility.hpp
)

#### }}}

add_library(
  qi STATIC
  ${QI_H}     ${QI_C}
  ${QITYPE_H} ${QITYPE_C}
  ${QIM_H}    ${QIM_C}
  ${QIPERF_H} ${QIPERF_C}
  ${QI_SOCKET_H}
  ${KA_H}
)

target_include_directories(qi PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${BINARY_INCLUDE_DIR}")
set_target_properties(
  qi PROPERTIES
  PUBLIC_HEADER "${QI_H};${QITYPE_H};${QIM_H};${QIPERF_H};${KA_H}"
)

# In case that was not clear enough. We build and link statically.
set_target_properties(qi PROPERTIES LINK_SEARCH_START_STATIC ON)
set_target_properties(qi PROPERTIES LINK_SEARCH_END_STATIC ON)
target_link_options(qi PUBLIC -static-libgcc -static-libstdc++ -static)

add_dependencies(qi OpenSSL Boost)
target_link_libraries(
  qi PUBLIC
  OpenSSL::SSL OpenSSL::Crypto
  Boost::headers Boost::atomic Boost::chrono Boost::date_time
  Boost::filesystem Boost::program_options Boost::random Boost::regex Boost::thread
)
target_include_directories(qi PUBLIC ${Boost_INCLUDE_DIR})

#### Add optional libs {{{
if(WIN32)
  target_link_libraries(qi WSOCK IPHLPAPI PSAPI WSA) #Really?
endif()

if(WITH_BOOST_LOCALE)
  target_link_libraries(qi PUBLIC Boost::locale)
endif()

if (WITH_SYSTEMD)
  target_link_libraries(qi PUBLIC systemd)
endif()

# Threads
find_package(Threads)
if(Threads_FOUND)
  target_link_libraries(qi PRIVATE Threads::Threads ${CMAKE_THREAD_LIBS_INIT})
else() # No threads library on Mac or Android
  target_link_libraries(qi PRIVATE -pthread)
endif()

if(ANDROID)
  target_link_libraries(qi PUBLIC log)
endif()

if(UNIX AND NOT APPLE AND NOT ANDROID)
  target_link_libraries(qi PUBLIC dl rt)
endif()
#### }}}

# Install a share/qi/path.conf so that libqi's sdkPrefix can be found
file(WRITE "${CMAKE_BINARY_DIR}/path.conf" "# Installed by qibuild. Do not remove\n")
set_target_properties(qi PROPERTIES RESOURCE "${CMAKE_BINARY_DIR}/path.conf")
install(TARGETS qi
  RESOURCE DESTINATION "qi"
  PUBLIC_HEADER DESTINATION include/qi
)

# Install includes and dependencies (OpenSSL, Boost)
install(DIRECTORY "${OPENSSL_INCLUDE_DIR}" "${Boost_INCLUDE_DIR}/boost" "qi" "ka" DESTINATION "include")
install(DIRECTORY  "${OPENSSL_INSTALL_DIR}/lib" "${BOOST_INSTALL_DIR}/lib"  DESTINATION ".")
