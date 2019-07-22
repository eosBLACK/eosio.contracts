add_contract(eb.member eb.member ${CMAKE_CURRENT_SOURCE_DIR}/src/eb.member.cpp)

target_include_directories(eb.member
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../eosio.system/include)
   

set_target_properties(eb.member
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
