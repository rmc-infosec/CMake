# Deep cmake_path coverage
cmake_path(SET p "/usr/local/lib/libfoo.so.1.2.3")

cmake_path(HAS_ROOT_NAME p has_rn)
cmake_path(HAS_ROOT_DIRECTORY p has_rd)
cmake_path(HAS_ROOT_PATH p has_rp)
cmake_path(HAS_FILENAME p has_fn)
cmake_path(HAS_EXTENSION p has_ext)
cmake_path(HAS_STEM p has_stem)
cmake_path(HAS_RELATIVE_PART p has_rel)
cmake_path(HAS_PARENT_PATH p has_parent)

cmake_path(GET p EXTENSION ext)
cmake_path(GET p EXTENSION LAST_ONLY last_ext)
message(STATUS "Ext: ${ext}, LastExt: ${last_ext}")

# CONVERT
cmake_path(CONVERT "/usr/local/bin" TO_CMAKE_PATH_LIST cmake_list)
message(STATUS "CMake list: ${cmake_list}")
cmake_path(CONVERT "/usr/local/bin" TO_NATIVE_PATH_LIST native)
message(STATUS "Native: ${native}")

cmake_path(CONVERT "/usr/local;/usr/bin;/opt" TO_NATIVE_PATH_LIST multi_native NORMALIZE)
message(STATUS "Multi native: ${multi_native}")

# HASH
cmake_path(HASH p path_hash)
message(STATUS "Hash: ${path_hash}")

# Complex path operations
cmake_path(SET complex "//server/share/dir/../other/./file.txt")
cmake_path(NORMAL_PATH complex)
message(STATUS "Normalized: ${complex}")

cmake_path(SET rel_path "a/b/c")
cmake_path(ABSOLUTE_PATH rel_path BASE_DIRECTORY "/base" NORMALIZE)
message(STATUS "Absolute: ${rel_path}")

cmake_path(SET p1 "/usr/local/lib")
cmake_path(SET p2 "/usr/local/include")
cmake_path(RELATIVE_PATH p2 BASE_DIRECTORY "${p1}")
message(STATUS "Relative: ${p2}")

# Decompose and recompose
cmake_path(SET decompose "/usr/local/lib/cmake/foo-1.0/FooConfig.cmake")
cmake_path(GET decompose ROOT_PATH root)
cmake_path(GET decompose RELATIVE_PART relative)
cmake_path(GET decompose PARENT_PATH parent)
cmake_path(GET decompose FILENAME filename)
cmake_path(GET decompose STEM stem)
message(STATUS "Root: ${root}, Rel: ${relative}, Parent: ${parent}, File: ${filename}, Stem: ${stem}")
