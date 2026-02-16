include(CMakeParseArguments)

function(my_install)
  cmake_parse_arguments(ARG
    "OPTIONAL;FAST"
    "DESTINATION;COMPONENT"
    "FILES;TARGETS;DEPENDS"
    ${ARGN}
  )
  message(STATUS "OPTIONAL=${ARG_OPTIONAL}")
  message(STATUS "DEST=${ARG_DESTINATION}")
  message(STATUS "FILES=${ARG_FILES}")
  message(STATUS "UNPARSED=${ARG_UNPARSED_ARGUMENTS}")
  message(STATUS "KEYWORDS_MISSING=${ARG_KEYWORDS_MISSING_VALUES}")
endfunction()

my_install(
  FILES a.h b.h c.h
  DESTINATION include
  COMPONENT headers
  OPTIONAL
)

my_install(TARGETS mylib DESTINATION lib FAST)
my_install(DEPENDS foo bar)

function(test_prefix)
  cmake_parse_arguments(PARSE_ARGV 0 ARG "VERBOSE" "NAME" "VALUES")
  message(STATUS "Name: ${ARG_NAME}, Values: ${ARG_VALUES}")
endfunction()
test_prefix(NAME "test" VALUES 1 2 3 VERBOSE)
