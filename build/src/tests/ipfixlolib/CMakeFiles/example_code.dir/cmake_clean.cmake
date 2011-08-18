FILE(REMOVE_RECURSE
  "CMakeFiles/example_code.dir/example_code.o"
  "example_code.pdb"
  "example_code"
)

# Per-language clean rules from dependency scanning.
FOREACH(lang CXX)
  INCLUDE(CMakeFiles/example_code.dir/cmake_clean_${lang}.cmake OPTIONAL)
ENDFOREACH(lang)
