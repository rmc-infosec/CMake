cmake_minimum_required(VERSION 3.10)
cmake_policy(VERSION 3.10)

cmake_policy(SET CMP0054 NEW)
cmake_policy(SET CMP0057 NEW)
cmake_policy(GET CMP0054 cmp54_status)
message(STATUS "CMP0054: ${cmp54_status}")

cmake_policy(PUSH)
cmake_policy(SET CMP0054 OLD)
cmake_policy(GET CMP0054 inner_status)
message(STATUS "Inner CMP0054: ${inner_status}")
cmake_policy(POP)

cmake_policy(GET CMP0054 outer_status)
message(STATUS "Outer CMP0054: ${outer_status}")

cmake_minimum_required(VERSION 3.16...3.28)
