add_contract(eb.cryptor eb.cryptor ${CMAKE_CURRENT_SOURCE_DIR}/src/eb.cryptor.cpp)

target_include_directories(eb.cryptor
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/../eosio.system/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../eb.cryptob/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../eosio.token/include
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(eb.cryptor
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
