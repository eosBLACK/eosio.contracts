#include <boost/test/unit_test.hpp>
#include <eosio/chain/contract_table_objects.hpp>
#include <eosio/chain/global_property_object.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/wast_to_wasm.hpp>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <fc/log/logger.hpp>
#include <eosio/chain/exceptions.hpp>
#include <Runtime/Runtime.h>

#include "eosio.system_tester.hpp"

struct _abi_hash {
   name owner;
   fc::sha256 hash;
};
FC_REFLECT( _abi_hash, (owner)(hash) );

using namespace eosio_system;

class eb_faucet_tester : public eosio_system_tester {
public:
   eb_faucet_tester() {
      produce_blocks( 2 );

      create_account_with_resources( N(eb.faucet), config::system_account_name, core_sym::from_string("20.0000"), false );

      produce_blocks( 100 );      
      
      set_code( N(eb.faucet), contracts::faucet_wasm());
      set_abi( N(eb.faucet), contracts::faucet_abi().data() );
      {
         const auto& accnt = control->db().get<account_object,by_name>( N(eb.faucet) );
         abi_def abi;
         BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
         faucet_abi_ser.set_abi(abi, abi_serializer_max_time);
      } 
      
      {
         auto trace = base_tester::push_action(config::system_account_name, N(setpriv),
                                               config::system_account_name,  mutable_variant_object()
                                               ("account", "eb.faucet")
                                               ("is_priv", 1)
         );
      } 
   }
   
   action_result push_action_eb( const account_name& signer, const action_name &name, const variant_object &data, bool auth = true ) {
         string action_type_name = faucet_abi_ser.get_action_type(name);

         action act;
         act.account = N(eb.faucet);
         act.name = name;
         act.data = faucet_abi_ser.variant_to_binary( action_type_name, data, abi_serializer_max_time );

         return base_tester::push_action( std::move(act), auth ? uint64_t(signer) : signer == N(bob111111111) ? N(alice1111111) : N(bob111111111) );
   }

   action_result create_account( const account_name create, 
                                 const account_name sender, 
                                 const fc::crypto::public_key owner_key, 
                                 const fc::crypto::public_key active_key ) {
      return push_action_eb( name(create), N(newaccount), mvo()
                          ("sender", sender)
                          ("owner_key", owner_key)
                          ("active_key", active_key )
      );
   }   
   
   action_result send_token( const account_name requestor, const account_name receiver ) {
      return push_action_eb( name(requestor), N(sendtoken), mvo()
                          ("receiver", receiver)
      );
   }    

   abi_serializer faucet_abi_ser;
};

BOOST_AUTO_TEST_SUITE(eb_faucet_tests)

BOOST_FIXTURE_TEST_CASE( new_account, eb_faucet_tester ) try {
   cross_15_percent_threshold();

   produce_blocks( 10 );

   transfer( "eosio", "eb.faucet", core_sym::from_string("1000.0000"), "eosio" );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("1000.0000"), get_balance( "eb.faucet" ) );

   BOOST_REQUIRE_EQUAL( core_sym::from_string("0.0000"), get_balance( "alice1111111" ) );
   transfer( "eosio", "alice1111111", core_sym::from_string("1000.0000"), "eosio" );

   BOOST_REQUIRE_EQUAL( core_sym::from_string("1000.0000"), get_balance( "alice1111111" ) );
   BOOST_REQUIRE_EQUAL( success(), stake( "eosio", "alice1111111", core_sym::from_string("200.0000"), core_sym::from_string("100.0000") ) );

   auto total = get_total_stake("alice1111111");
   BOOST_REQUIRE_EQUAL( core_sym::from_string("210.0000"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( core_sym::from_string("110.0000"), total["cpu_weight"].as<asset>());
   
   
   BOOST_REQUIRE_EQUAL( success(), create_account( "alice1111111", 
                                                   "eosoulhammer", 
                                                   get_public_key( N(eosoulhammer), "active"), 
                                                   get_public_key( N(eosoulhammer), "active")));
   produce_blocks( 100 );

   total = get_total_stake("eosoulhammer");
   BOOST_REQUIRE_EQUAL( core_sym::from_string("1.0000"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( core_sym::from_string("1.0000"), total["cpu_weight"].as<asset>());   
   BOOST_REQUIRE_EQUAL( name("eosoulhammer"), total["owner"].as<name>());

   transfer( "eosio", "eosoulhammer", core_sym::from_string("1000.0000"), "eosio" );
   transfer( "alice1111111", "eosoulhammer", core_sym::from_string("1.0000"), "alice1111111" );
   transfer( "eosoulhammer", "alice1111111", core_sym::from_string("1.0000"), "eosoulhammer" );
} FC_LOG_AND_RETHROW() 

BOOST_FIXTURE_TEST_CASE( send_token_free, eb_faucet_tester ) try {
   cross_15_percent_threshold();

   produce_blocks( 10 );

   transfer( "eosio", "eb.faucet", core_sym::from_string("1000.0000"), "eosio" );
   transfer( "eosio", "alice1111111", core_sym::from_string("1000.0000"), "eosio" );
   
   BOOST_REQUIRE_EQUAL( core_sym::from_string("1000.0000"), get_balance( "eb.faucet" ) );
   BOOST_REQUIRE_EQUAL( success(), send_token( "alice1111111", "bob111111111" ));
   BOOST_REQUIRE_EQUAL( core_sym::from_string("900.0000"), get_balance( "eb.faucet" ) );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("100.0000"), get_balance( "bob111111111" ) );   
   
   BOOST_REQUIRE_EQUAL( wasm_assert_msg("Opportunity is only every six hours"), send_token( "alice1111111", "bob111111111" ));
   BOOST_REQUIRE_EQUAL( core_sym::from_string("900.0000"), get_balance( "eb.faucet" ) );   
   BOOST_REQUIRE_EQUAL( core_sym::from_string("100.0000"), get_balance( "bob111111111" ) );   
   
   produce_block( fc::hours(5) );
   BOOST_REQUIRE_EQUAL( wasm_assert_msg("Opportunity is only every six hours"), send_token( "alice1111111", "bob111111111" ));
   BOOST_REQUIRE_EQUAL( core_sym::from_string("900.0000"), get_balance( "eb.faucet" ) );   
   BOOST_REQUIRE_EQUAL( core_sym::from_string("100.0000"), get_balance( "bob111111111" ) );    
   
   produce_block( fc::minutes(59) );
   BOOST_REQUIRE_EQUAL( wasm_assert_msg("Opportunity is only every six hours"), send_token( "alice1111111", "bob111111111" ));
   BOOST_REQUIRE_EQUAL( core_sym::from_string("900.0000"), get_balance( "eb.faucet" ) );   
   BOOST_REQUIRE_EQUAL( core_sym::from_string("100.0000"), get_balance( "bob111111111" ) );   
   
   produce_block( fc::minutes(2) );
   BOOST_REQUIRE_EQUAL( success(), send_token( "alice1111111", "bob111111111" ));
   BOOST_REQUIRE_EQUAL( core_sym::from_string("800.0000"), get_balance( "eb.faucet" ) );   
   BOOST_REQUIRE_EQUAL( core_sym::from_string("200.0000"), get_balance( "bob111111111" ) );   
   
   produce_block( fc::hours(5) );
   BOOST_REQUIRE_EQUAL( wasm_assert_msg("Opportunity is only every six hours"), send_token( "alice1111111", "bob111111111" ));
   BOOST_REQUIRE_EQUAL( core_sym::from_string("800.0000"), get_balance( "eb.faucet" ) );   
   BOOST_REQUIRE_EQUAL( core_sym::from_string("200.0000"), get_balance( "bob111111111" ) );      
   
   produce_block( fc::hours(1) );
   BOOST_REQUIRE_EQUAL( success(), send_token( "alice1111111", "bob111111111" ));
   BOOST_REQUIRE_EQUAL( core_sym::from_string("700.0000"), get_balance( "eb.faucet" ) );   
   BOOST_REQUIRE_EQUAL( core_sym::from_string("300.0000"), get_balance( "bob111111111" ) );      
} FC_LOG_AND_RETHROW() 

BOOST_AUTO_TEST_SUITE_END()
