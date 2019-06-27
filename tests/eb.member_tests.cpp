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

class eb_member_tester : public eosio_system_tester {
public:
   enum members { participants, supporters, represent };
   const char *members_str[3]={ "participants","supporters","represent" };   

   eb_member_tester() {
      produce_blocks( 2 );

      create_account_with_resources( N(eb.member), config::system_account_name, core_sym::from_string("20.0000"), false );

      produce_blocks( 100 );      
      
      set_code( N(eb.member), contracts::member_wasm());
      set_abi( N(eb.member), contracts::member_abi().data() );
      {
         const auto& accnt = control->db().get<account_object,by_name>( N(eb.member) );
         abi_def abi;
         BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
         member_abi_ser.set_abi(abi, abi_serializer_max_time);
      } 
      
      {
         auto trace = base_tester::push_action(config::system_account_name, N(setpriv),
                                               config::system_account_name,  mutable_variant_object()
                                               ("account", "eb.member")
                                               ("is_priv", 1)
         );
      } 
   }
   
   action_result push_action_eb( const account_name& signer, const action_name &name, const variant_object &data, bool auth = true ) {
         string action_type_name = member_abi_ser.get_action_type(name);

         action act;
         act.account = N(eb.member);
         act.name = name;
         act.data = member_abi_ser.variant_to_binary( action_type_name, data, abi_serializer_max_time );

         return base_tester::push_action( std::move(act), auth ? uint64_t(signer) : signer == N(bob111111111) ? N(alice1111111) : N(bob111111111) );
   }
   
   action_result stake_eb( const account_name& from, const account_name& to, const asset& net, const asset& cpu ) {
      return push_action_eb( name(from), N(staking), mvo()
                          ("from",     from)
                          ("receiver", to)
                          ("stake_net_quantity", net)
                          ("stake_cpu_quantity", cpu)
                          ("transfer", 0 )
      );
   }

   action_result unstake_eb( const account_name& from, const account_name& to, const asset& net, const asset& cpu ) {
      return push_action_eb( name(from), N(unstaking), mvo()
                          ("from",     from)
                          ("receiver", to)
                          ("unstake_net_quantity", net)
                          ("unstake_cpu_quantity", cpu)
      );
   }
   
   action_result setcriteria( const name& member_type, const asset& low_quantity, const asset& high_quantity ) {
      return push_action_eb( N(eb.member), N(setcriteria), mvo()
                           ("member_type", member_type)
                           ("low_quantity", low_quantity)
                           ("high_quantity", high_quantity)
      );
   }
   
   fc::variant get_criteria(const name& account)
   {
      vector<char> data = get_row_by_account( N(eb.member), N(eb.member), N(criterias), account.value );  
      return data.empty() ? fc::variant() : member_abi_ser.binary_to_variant( "criteria", data, abi_serializer_max_time );
   }

   fc::variant get_participant(const name& account)
   {
      vector<char> data = get_row_by_account( N(eb.member), N(eb.member), N(participants), account.value );  
      return data.empty() ? fc::variant() : member_abi_ser.binary_to_variant( "participant", data, abi_serializer_max_time );
   }
   
   fc::variant get_supporter(const name& account)
   {
      vector<char> data = get_row_by_account( N(eb.member), N(eb.member), N(supporters), account.value );  
      return data.empty() ? fc::variant() : member_abi_ser.binary_to_variant( "supporter", data, abi_serializer_max_time );
   }
   
   fc::variant get_representative_candidate(const name& account)
   {
      vector<char> data = get_row_by_account( N(eb.member), N(eb.member), N(reprecandi), account.value );  
      return data.empty() ? fc::variant() : member_abi_ser.binary_to_variant( "representative_candidate", data, abi_serializer_max_time );
   }
   
   fc::variant get_representative(const name& account)
   {
      vector<char> data = get_row_by_account( N(eb.member), N(eb.member), N(repre), account.value );  
      return data.empty() ? fc::variant() : member_abi_ser.binary_to_variant( "representative", data, abi_serializer_max_time );
   }

   void init_criteria(string participant_low, string participant_high, 
                     string supporter_low, string supporter_high,
                     string representative_low, string representative_high) {
      BOOST_REQUIRE_EQUAL( success(), setcriteria( name(members_str[participants]), asset::from_string(participant_low), asset::from_string(participant_high)) );
      auto criteria = get_criteria(name(members_str[participants]));
      REQUIRE_MATCHING_OBJECT( criteria, mvo()
         ("member_type", members_str[participants])
         ("low_quantity", participant_low)
         ("high_quantity", participant_high)
      );
      
      BOOST_REQUIRE_EQUAL( success(), setcriteria( name(members_str[supporters]), asset::from_string(supporter_low), asset::from_string(supporter_high)) );
      criteria = get_criteria(name(members_str[supporters]));
      REQUIRE_MATCHING_OBJECT( criteria, mvo()
         ("member_type", members_str[supporters])
         ("low_quantity", supporter_low)
         ("high_quantity", supporter_high)
      );   
      
      BOOST_REQUIRE_EQUAL( success(), setcriteria( name(members_str[represent]), asset::from_string(representative_low), asset::from_string(representative_high)) );
      criteria = get_criteria(name(members_str[represent]));
      REQUIRE_MATCHING_OBJECT( criteria, mvo()
         ("member_type", members_str[represent])
         ("low_quantity", representative_low)
         ("high_quantity", representative_high)
      );       
   }

   abi_serializer member_abi_ser;
};

BOOST_AUTO_TEST_SUITE(eb_member_tests)

BOOST_FIXTURE_TEST_CASE( stake_unstake, eb_member_tester ) try {
   cross_15_percent_threshold();

   produce_blocks( 10 );
   produce_block( fc::hours(3*24) );

   BOOST_REQUIRE_EQUAL( core_sym::from_string("0.0000"), get_balance( "alice1111111" ) );
   transfer( "eosio", "alice1111111", core_sym::from_string("1000.0000"), "eosio" );

   BOOST_REQUIRE_EQUAL( core_sym::from_string("1000.0000"), get_balance( "alice1111111" ) );
   BOOST_REQUIRE_EQUAL( success(), stake_eb( "eosio", "alice1111111", core_sym::from_string("200.0000"), core_sym::from_string("100.0000") ) );

   auto total = get_total_stake("alice1111111");
   BOOST_REQUIRE_EQUAL( core_sym::from_string("210.0000"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( core_sym::from_string("110.0000"), total["cpu_weight"].as<asset>());

   const auto init_eosio_stake_balance = get_balance( N(eosio.stake) );
   BOOST_REQUIRE_EQUAL( success(), stake_eb( "alice1111111", "alice1111111", core_sym::from_string("200.0000"), core_sym::from_string("100.0000") ) );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("700.0000"), get_balance( "alice1111111" ) );
   BOOST_REQUIRE_EQUAL( init_eosio_stake_balance + core_sym::from_string("300.0000"), get_balance( N(eosio.stake) ) );
   BOOST_REQUIRE_EQUAL( success(), unstake_eb( "alice1111111", "alice1111111", core_sym::from_string("200.0000"), core_sym::from_string("100.0000") ) );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("700.0000"), get_balance( "alice1111111" ) );

   produce_block( fc::hours(3*24-1) );
   produce_blocks(1);
   BOOST_REQUIRE_EQUAL( core_sym::from_string("700.0000"), get_balance( "alice1111111" ) );
   BOOST_REQUIRE_EQUAL( init_eosio_stake_balance + core_sym::from_string("300.0000"), get_balance( N(eosio.stake) ) );
   //after 3 days funds should be released
   produce_block( fc::hours(1) );
   produce_blocks(1);
   BOOST_REQUIRE_EQUAL( core_sym::from_string("1000.0000"), get_balance( "alice1111111" ) );
   BOOST_REQUIRE_EQUAL( init_eosio_stake_balance, get_balance( N(eosio.stake) ) );

   BOOST_REQUIRE_EQUAL( success(), stake_eb( "alice1111111", "bob111111111", core_sym::from_string("200.0000"), core_sym::from_string("100.0000") ) );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("700.0000"), get_balance( "alice1111111" ) );
   total = get_total_stake("bob111111111");
   BOOST_REQUIRE_EQUAL( core_sym::from_string("210.0000"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( core_sym::from_string("110.0000"), total["cpu_weight"].as<asset>());

   total = get_total_stake( "alice1111111" );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("210.0000").get_amount(), total["net_weight"].as<asset>().get_amount() );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("110.0000").get_amount(), total["cpu_weight"].as<asset>().get_amount() );

   REQUIRE_MATCHING_OBJECT( voter( "alice1111111", core_sym::from_string("300.0000")), get_voter_info( "alice1111111" ) );

   auto bytes = total["ram_bytes"].as_uint64();
   BOOST_REQUIRE_EQUAL( true, 0 < bytes );

   //unstake_eb from bob111111111
   BOOST_REQUIRE_EQUAL( success(), unstake_eb( "alice1111111", "bob111111111", core_sym::from_string("200.0000"), core_sym::from_string("100.0000") ) );
   total = get_total_stake("bob111111111");
   BOOST_REQUIRE_EQUAL( core_sym::from_string("10.0000"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( core_sym::from_string("10.0000"), total["cpu_weight"].as<asset>());
   produce_block( fc::hours(3*24-1) );
   produce_blocks(1);
   BOOST_REQUIRE_EQUAL( core_sym::from_string("700.0000"), get_balance( "alice1111111" ) );
   //after 3 days funds should be released
   produce_block( fc::hours(1) );
   produce_blocks(1);

   REQUIRE_MATCHING_OBJECT( voter( "alice1111111", core_sym::from_string("0.0000") ), get_voter_info( "alice1111111" ) );
   produce_blocks(1);
   BOOST_REQUIRE_EQUAL( core_sym::from_string("1000.0000"), get_balance( "alice1111111" ) );
} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( member_criteria, eb_member_tester ) try {
   cross_15_percent_threshold();

   init_criteria("10.0000 BLACK", "30.0000 BLACK", 
                  "31.0000 BLACK", "100.0000 BLACK", 
                  "101.0000 BLACK", "300.0000 BLACK");
   
   BOOST_REQUIRE_EQUAL( success(), setcriteria( name(members_str[represent]), asset::from_string("101.0000 BLACK"), asset::from_string("200.0000 BLACK")) );
   auto criteria = get_criteria(name(members_str[represent]));
   REQUIRE_MATCHING_OBJECT( criteria, mvo()
      ("member_type", members_str[represent])
      ("low_quantity", "101.0000 BLACK")
      ("high_quantity", "200.0000 BLACK")
   );    
   
   criteria = get_criteria(name(members_str[participants]));
   REQUIRE_MATCHING_OBJECT( criteria, mvo()
      ("member_type", members_str[participants])
      ("low_quantity", "10.0000 BLACK")
      ("high_quantity", "30.0000 BLACK")
   );   
   
   criteria = get_criteria(name(members_str[supporters]));
   REQUIRE_MATCHING_OBJECT( criteria, mvo()
      ("member_type", members_str[supporters])
      ("low_quantity", "31.0000 BLACK")
      ("high_quantity", "100.0000 BLACK")
   );    
} FC_LOG_AND_RETHROW() 
   
BOOST_FIXTURE_TEST_CASE( member_level_change_by_staking_unstaking, eb_member_tester ) try {
   cross_15_percent_threshold();

   produce_blocks( 10 );
   produce_block( fc::hours(3*24) );
   
   // participant: 50 ~ 99
   // supporter: 100 ~ 199
   // representative_candidate: 200 ~ 500
   init_criteria("50.0000 BLACK", "99.0000 BLACK", 
                  "100.0000 BLACK", "199.0000 BLACK", 
                  "200.0000 BLACK", "500.0000 BLACK");  

   // pre-condition
   // init staking amount: 20
   BOOST_REQUIRE_EQUAL( core_sym::from_string("0.0000"), get_balance( "alice1111111" ) );
   transfer( "eosio", "alice1111111", core_sym::from_string("5000.0000"), "eosio" );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("5000.0000"), get_balance( "alice1111111" ) );
   BOOST_REQUIRE_EQUAL( true, get_participant(name("alice1111111")).is_null() );
   BOOST_REQUIRE_EQUAL( true, get_supporter(name("alice1111111")).is_null() );
   BOOST_REQUIRE_EQUAL( true, get_representative_candidate(name("alice1111111")).is_null() );

   // total staking amount: 30
   BOOST_REQUIRE_EQUAL( success(), stake_eb( "alice1111111", "alice1111111", core_sym::from_string("5.0000"), core_sym::from_string("5.0000") ) );
   BOOST_REQUIRE_EQUAL( true, get_participant(name("alice1111111")).is_null() );
   BOOST_REQUIRE_EQUAL( true, get_supporter(name("alice1111111")).is_null() );
   BOOST_REQUIRE_EQUAL( true, get_representative_candidate(name("alice1111111")).is_null() );
   
   // total staking amount: 50
   BOOST_REQUIRE_EQUAL( success(), stake_eb( "alice1111111", "alice1111111", core_sym::from_string("10.0000"), core_sym::from_string("10.0000") ) );
   auto member = get_participant(name("alice1111111"));
   REQUIRE_MATCHING_OBJECT( member, mvo()
      ("account", "alice1111111")
   );    
   BOOST_REQUIRE_EQUAL( true, get_supporter(name("alice1111111")).is_null() );
   BOOST_REQUIRE_EQUAL( true, get_representative_candidate(name("alice1111111")).is_null() );
   
   // total staking amount: 100
   BOOST_REQUIRE_EQUAL( success(), stake_eb( "alice1111111", "alice1111111", core_sym::from_string("25.0000"), core_sym::from_string("25.0000") ) );
   member = get_supporter(name("alice1111111"));
   REQUIRE_MATCHING_OBJECT( member, mvo()
      ("account", "alice1111111")
   );    
   BOOST_REQUIRE_EQUAL( true, get_participant(name("alice1111111")).is_null() );
   BOOST_REQUIRE_EQUAL( true, get_representative_candidate(name("alice1111111")).is_null() );
   
   // total staking amount: 199
   BOOST_REQUIRE_EQUAL( success(), stake_eb( "alice1111111", "alice1111111", core_sym::from_string("50.0000"), core_sym::from_string("49.0000") ) );
   member = get_supporter(name("alice1111111"));
   REQUIRE_MATCHING_OBJECT( member, mvo()
      ("account", "alice1111111")
   );    
   BOOST_REQUIRE_EQUAL( true, get_participant(name("alice1111111")).is_null() );
   BOOST_REQUIRE_EQUAL( true, get_representative_candidate(name("alice1111111")).is_null() );
   
   
   // total staking amount: 200
   BOOST_REQUIRE_EQUAL( success(), stake_eb( "alice1111111", "alice1111111", core_sym::from_string("1.0000"), core_sym::from_string("0.0000") ) );
   member = get_representative_candidate(name("alice1111111"));
   REQUIRE_MATCHING_OBJECT( member, mvo()
      ("account", "alice1111111")
   );  
   BOOST_REQUIRE_EQUAL( true, get_participant(name("alice1111111")).is_null() );
   BOOST_REQUIRE_EQUAL( true, get_supporter(name("alice1111111")).is_null() );
   
   // total staking amount: 1000
   BOOST_REQUIRE_EQUAL( success(), stake_eb( "alice1111111", "alice1111111", core_sym::from_string("400.0000"), core_sym::from_string("400.0000") ) );
   member = get_representative_candidate(name("alice1111111"));
   REQUIRE_MATCHING_OBJECT( member, mvo()
      ("account", "alice1111111")
   );  
   BOOST_REQUIRE_EQUAL( true, get_participant(name("alice1111111")).is_null() );
   BOOST_REQUIRE_EQUAL( true, get_supporter(name("alice1111111")).is_null() );   
   
   // total staking amount: 500
   BOOST_REQUIRE_EQUAL( success(), unstake_eb( "alice1111111", "alice1111111", core_sym::from_string("250.0000"), core_sym::from_string("250.0000") ) );
   member = get_representative_candidate(name("alice1111111"));
   REQUIRE_MATCHING_OBJECT( member, mvo()
      ("account", "alice1111111")
   );  
   BOOST_REQUIRE_EQUAL( true, get_participant(name("alice1111111")).is_null() );
   BOOST_REQUIRE_EQUAL( true, get_supporter(name("alice1111111")).is_null() );     
   
   // total staking amount: 199
   BOOST_REQUIRE_EQUAL( success(), unstake_eb( "alice1111111", "alice1111111", core_sym::from_string("150.0000"), core_sym::from_string("151.0000") ) );
   member = get_supporter(name("alice1111111"));
   REQUIRE_MATCHING_OBJECT( member, mvo()
      ("account", "alice1111111")
   );  
   BOOST_REQUIRE_EQUAL( true, get_participant(name("alice1111111")).is_null() );
   BOOST_REQUIRE_EQUAL( true, get_representative_candidate(name("alice1111111")).is_null() );   
   
   // total staking amount: 49
   BOOST_REQUIRE_EQUAL( success(), unstake_eb( "alice1111111", "alice1111111", core_sym::from_string("75.0000"), core_sym::from_string("75.0000") ) );
   BOOST_REQUIRE_EQUAL( true, get_participant(name("alice1111111")).is_null() );
   BOOST_REQUIRE_EQUAL( true, get_supporter(name("alice1111111")).is_null() );
   BOOST_REQUIRE_EQUAL( true, get_representative_candidate(name("alice1111111")).is_null() );  
} FC_LOG_AND_RETHROW() 

BOOST_AUTO_TEST_SUITE_END()
