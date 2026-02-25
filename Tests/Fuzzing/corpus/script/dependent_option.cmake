include(CMakeDependentOption OPTIONAL)

option(USE_FOO "Use Foo" ON)
option(USE_BAR "Use Bar" OFF)

if(COMMAND cmake_dependent_option)
  cmake_dependent_option(USE_FOO_EXTRA "Extra Foo features" ON "USE_FOO" OFF)
  cmake_dependent_option(USE_BAR_EXTRA "Extra Bar features" ON "USE_BAR;USE_FOO" OFF)
  message(STATUS "FOO_EXTRA=${USE_FOO_EXTRA} BAR_EXTRA=${USE_BAR_EXTRA}")
endif()

# Manual dependent option pattern
if(USE_FOO)
  option(FOO_VERBOSE "Verbose Foo" OFF)
  option(FOO_DEBUG "Debug Foo" ON)
  message(STATUS "Foo options: VERBOSE=${FOO_VERBOSE} DEBUG=${FOO_DEBUG}")
else()
  set(FOO_VERBOSE OFF)
  set(FOO_DEBUG OFF)
endif()
