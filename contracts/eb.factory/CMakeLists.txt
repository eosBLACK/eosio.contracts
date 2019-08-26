add_contract(eb.factory eb.factory ${CMAKE_CURRENT_SOURCE_DIR}/src/eb.factory.cpp)

target_include_directories(eb.factory
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../eosio.system/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../eb.member/include)

   

set_target_properties(eb.factory
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
