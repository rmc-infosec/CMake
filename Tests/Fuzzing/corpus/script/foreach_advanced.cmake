foreach(i RANGE 10)
  message(STATUS "Range: ${i}")
endforeach()

foreach(i RANGE 2 10 3)
  message(STATUS "Step range: ${i}")
endforeach()

foreach(item IN ITEMS a b c d e)
  message(STATUS "Item: ${item}")
endforeach()

set(mylist x y z)
foreach(item IN LISTS mylist)
  message(STATUS "List item: ${item}")
endforeach()

# ZIP_LISTS
set(keys "name" "age" "city")
set(values "Alice" "30" "NYC")
foreach(k v IN ZIP_LISTS keys values)
  message(STATUS "${k} = ${v}")
endforeach()

# Nested foreach with break/continue
foreach(outer RANGE 3)
  foreach(inner RANGE 3)
    if(inner EQUAL outer)
      continue()
    endif()
    if(inner GREATER 2)
      break()
    endif()
    message(STATUS "(${outer},${inner})")
  endforeach()
endforeach()
