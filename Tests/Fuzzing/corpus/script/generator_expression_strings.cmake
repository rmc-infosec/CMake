# Generator expressions processed by string(GENEX_STRIP) and file(GENERATE)
set(genex_str "$<TARGET_FILE:foo>")
string(GENEX_STRIP "${genex_str}" stripped)
message(STATUS "Stripped: '${stripped}'")

set(complex_genex "$<$<BOOL:TRUE>:yes>$<$<BOOL:FALSE>:no>")
string(GENEX_STRIP "${complex_genex}" stripped2)
message(STATUS "Stripped2: '${stripped2}'")

set(nested "$<$<AND:$<BOOL:1>,$<BOOL:0>>:value>")
string(GENEX_STRIP "${nested}" stripped3)
message(STATUS "Stripped3: '${stripped3}'")

set(mixed "prefix_$<CONFIG>_suffix")
string(GENEX_STRIP "${mixed}" stripped4)
message(STATUS "Stripped4: '${stripped4}'")

# Various genex forms for coverage of strip parsing
set(g1 "$<0:text>")
set(g2 "$<1:text>")
set(g3 "$<ANGLE-R>")
set(g4 "$<COMMA>")
set(g5 "$<SEMICOLON>")
set(g6 "$<JOIN:a;b;c,->")
set(g7 "$<LOWER_CASE:HELLO>")
set(g8 "$<UPPER_CASE:hello>")
set(g9 "$<MAKE_C_IDENTIFIER:my-var>")
set(g10 "$<IF:$<BOOL:1>,yes,no>")

foreach(g IN ITEMS "${g1}" "${g2}" "${g3}" "${g4}" "${g5}" "${g6}" "${g7}" "${g8}" "${g9}" "${g10}")
  string(GENEX_STRIP "${g}" s)
  message(STATUS "Strip: '${s}'")
endforeach()
