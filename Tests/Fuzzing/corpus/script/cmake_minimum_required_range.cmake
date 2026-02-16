# cmake_minimum_required version range
cmake_minimum_required(VERSION 3.5)
message(STATUS "After 3.5")

cmake_minimum_required(VERSION 3.10...3.28)
message(STATUS "After range 3.10...3.28")

cmake_policy(VERSION 3.16)
cmake_policy(VERSION 3.10...3.28)

# Policy stack operations
cmake_policy(PUSH)
cmake_policy(SET CMP0054 NEW)
cmake_policy(SET CMP0057 NEW)
cmake_policy(SET CMP0077 NEW)
cmake_policy(SET CMP0076 NEW)

cmake_policy(GET CMP0054 p54)
cmake_policy(GET CMP0057 p57)
message(STATUS "CMP0054=${p54} CMP0057=${p57}")

cmake_policy(PUSH)
cmake_policy(SET CMP0054 OLD)
cmake_policy(GET CMP0054 inner)
message(STATUS "Inner CMP0054=${inner}")
cmake_policy(POP)

cmake_policy(GET CMP0054 after_pop)
message(STATUS "After pop CMP0054=${after_pop}")
cmake_policy(POP)
