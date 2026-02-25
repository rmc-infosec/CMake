set_property(GLOBAL PROPERTY MY_GLOBAL_PROP "global_value")
get_property(val GLOBAL PROPERTY MY_GLOBAL_PROP)
message(STATUS "Global: ${val}")

set_property(GLOBAL APPEND PROPERTY MY_LIST_PROP "a")
set_property(GLOBAL APPEND PROPERTY MY_LIST_PROP "b")
get_property(list_val GLOBAL PROPERTY MY_LIST_PROP)
message(STATUS "List: ${list_val}")

get_cmake_property(vars VARIABLES)
get_cmake_property(cmds COMMANDS)
get_cmake_property(comps COMPONENTS)
get_cmake_property(macros MACROS)

get_property(defined GLOBAL PROPERTY MY_GLOBAL_PROP DEFINED)
get_property(set_val GLOBAL PROPERTY MY_GLOBAL_PROP SET)
message(STATUS "Defined: ${defined}, Set: ${set_val}")

set_property(DIRECTORY PROPERTY MY_DIR_PROP "dir_value")
get_property(dir_val DIRECTORY PROPERTY MY_DIR_PROP)
get_directory_property(parent_val PARENT_DIRECTORY)
message(STATUS "Dir: ${dir_val}, Parent: ${parent_val}")
