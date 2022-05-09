libQi
=====

libQi is a middleware framework that provides RPC, type-erasure,
cross-language interoperability, OS abstractions, logging facilities,
asynchronous task management, dynamic module loading.

This is not the official libQi project, but a variant that does not use QiBuild,
omits several components such as performance profiling,
CLI tools, old compatibility bindings, examples and tests.

It requires no external dependency, because it recompiles OpenSSL and Boost locally.
Everything is build statically to maximize portability.

Compilation
-----------

This is a standard CMake project.
Clone this repository and `cd` to it.

.. code-block:: sh

  mkdir build
  cd build
  cmake ..
  cmake --build .
  cmake --install . --prefix /path/to/install

Example
-------

The following example shows some features of the framework, please refer to the
documentation for further details.
In this example, we use CMake to configure an executable named `my_service`,
that would download and recompile on `qi` locally.

.. code-block:: cmake

  # Makes Qi available.
  # Also makes OpenSSL and Boost available.
  include(FetchContent)
  FetchContent_Declare(
    qi
    GIT_REPOSITORY <path/to/this/repository>
    GIT_TAG <hash/or/name/of/desired/reference>
  )
  FetchContent_MakeAvailable(qi)

  add_executable(my_service src/nao.cpp)
  set_target_properties(my_service PROPERTIES LINK_SEARCH_START_STATIC ON)
  set_target_properties(my_service PROPERTIES LINK_SEARCH_END_STATIC ON)
  target_link_options(my_service PRIVATE -static-libgcc -static-libstdc++ -static)
  target_include_directories(my_service PRIVATE ${QI_INCLUDE_DIRS})
  add_dependencies(my_service qi)

.. code-block:: cpp

  #include <boost/make_shared.hpp>
  #include <qi/log.hpp>
  #include <qi/applicationsession.hpp>
  #include <qi/anyobject.hpp>
  #include <qi/registration.hpp>

  qiLogCategory("myapplication");

  class MyService
  {
  public:
    void myFunction(int val) {
      qiLogInfo() << "myFunction called with " << val;
    }
    qi::Signal<int> eventTriggered;
    qi::Property<float> angle;
  };

  // register the service to the type-system
  QI_REGISTER_OBJECT(MyService, myFunction, eventTriggered, angle);

  void print()
  {
    qiLogInfo() << "print was called";
  }

  int main(int argc, char* argv[])
  {
    // initializes Qi's typesystem with basic types
    qi::registerBaseTypes();

    // parse command line arguments such as --qi-url, to connect to a service directory
    qi::ApplicationSession app(argc, argv);

    // connect the session included in the app
    app.start();

    // this session is connected and ready to use
    qi::SessionPtr session = app.session();

    // register our service
    session->registerService("MyService", boost::make_shared<MyService>());

    // get our service through the middleware
    qi::AnyObject obj = session->service("MyService");

    // call myFunction
    obj.call<void>("myFunction", 42);

    // call print in 2 seconds
    qi::async(&print, qi::Seconds(2));

    // block until ctrl-c
    app.run();
  }

The executable is built with the following commands:

.. code-block:: sh

  mkdir build
  cd build
  cmake ..
  cmake --build .
  cmake --install . --prefix /path/to/install
You can then run the program with:

.. code-block:: console

  ./myservice --qi-standalone # for a standalone server
  ./myservice --qi-url tcp://somemachine:9559 # to connect to another galaxy of sessions



Cross-compilation
-----------------

For NAO v5, NAOqi v2.1
======================

You *should* be able to cross-compile your project using the official cross-toolchains.
The [official documentation](https://developer.softbankrobotics.com/nao-naoqi-2-1/naoqi-developer-guide/getting-started/retrieving-software#retrieving-software)
provides [a dead link to retrieve it](https://community.aldebaran.com/en/resources/software).
It cannot be found either on the new [official download page](https://www.softbankrobotics.com/emea/en/support/nao-6/downloads-softwares/former-versions?os=49&category=76).

This is where this project helps.
Given that you can find a cross-toolchain for your platform targetting Linux x86,
you should be able to compile libQi and use it to control a robot.

For instance, on a Mac with an M1 processor (aarch64 architecture),
you can find a set of generic toolchains [here](https://github.com/messense/homebrew-macos-cross-toolchains).
You can use [`mac-homebrew-i686.toolchain.cmake`](mac-homebrew-i686.toolchain.cmake)
to use the [`musl`](https://musl.libc.org/) toolchain,
which supports compiling very portable binaries with static linking:

.. code-block:: sh
  brew tap messense/macos-cross-toolchains
  brew install i686-unknown-linux-musl

  cmake -B build-nao -DCMAKE_TOOLCHAIN_FILE=mac-homebrew-i686.toolchain.cmake
  cmake --build build-nao
  cmake --install build-nao --prefix /path/to/install

Links
-----

Upstream Git repository:
http://github.com/aldebaran/libqi

Documentation:
http://doc.aldebaran.com/libqi/
