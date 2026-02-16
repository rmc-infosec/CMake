set(CONTENT_VAR "generated content line 1\ngenerated content line 2\n")

file(GENERATE
  OUTPUT "/tmp/cmake_fuzz_gen_$<BOOL:1>.txt"
  CONTENT "${CONTENT_VAR}"
)

file(GENERATE
  OUTPUT "/tmp/cmake_fuzz_gen_cond.txt"
  CONTENT "condition was true\n"
  CONDITION $<BOOL:TRUE>
)

file(WRITE "/tmp/cmake_fuzz_input.txt.in" "@VAR1@ and @VAR2@ template\n")
set(VAR1 "first")
set(VAR2 "second")
file(GENERATE
  OUTPUT "/tmp/cmake_fuzz_gen_input.txt"
  INPUT "/tmp/cmake_fuzz_input.txt.in"
)

file(REMOVE "/tmp/cmake_fuzz_input.txt.in")
