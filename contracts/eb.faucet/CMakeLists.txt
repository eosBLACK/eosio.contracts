add_contract(eb.faucet eb.faucet ${CMAKE_CURRENT_SOURCE_DIR}/src/eb.faucet.cpp)

target_include_directories(eb.faucet
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../eosio.system/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../eosio.token/include)
   

set_target_properties(eb.faucet
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
