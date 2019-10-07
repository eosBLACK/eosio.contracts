add_contract(eb.cryptob eb.cryptob ${CMAKE_CURRENT_SOURCE_DIR}/src/eb.cryptob.cpp)

target_include_directories(eb.cryptob
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(eb.cryptob
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
