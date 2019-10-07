#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/wast_to_wasm.hpp>

#include <Runtime/Runtime.h>

#include <fc/variant_object.hpp>
#include "contracts.hpp"
#include "test_symbol.hpp"

using namespace eosio::testing;
using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;

using mvo = fc::mutable_variant_object;

class eb_cryptob_tester : public tester {
public:
   enum members { participants = 0, supporters, reprecandi };
   const char *members_str[4]={ "participants","supporters", "reprecandi" }; 

   eb_cryptob_tester() {
      create_accounts( { N(eb.cryptob), N(eb.member), N(eosio.msig), N(eosio.stake), N(eosio.ram), N(eosio.ramfee) } );
      
      produce_block();      
      
      // deploy eb.cryptob
      set_code( N(eb.cryptob), contracts::cryptob_wasm());
      set_abi( N(eb.cryptob), contracts::cryptob_abi().data() );
      {
         const auto& accnt = control->db().get<account_object,by_name>( N(eb.cryptob) );
         abi_def abi;
         BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
         cryptob_abi_ser.set_abi(abi, abi_serializer_max_time);
      } 
      
      {
         auto trace = base_tester::push_action(config::system_account_name, N(setpriv),
                                               config::system_account_name,  mutable_variant_object()
                                               ("account", "eb.cryptob")
                                               ("is_priv", 1)
         );
      }       
 
      
      // deploy eb.msig
      set_code( N(eosio.msig), contracts::msig_wasm() );
      set_abi( N(eosio.msig), contracts::msig_abi().data() );
      {
         produce_blocks();
         const auto& accnt = control->db().get<account_object,by_name>( N(eosio.msig) );
         abi_def abi;
         BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
         msig_abi_ser.set_abi(abi, abi_serializer_max_time);
      }
      
      {
         auto trace = base_tester::push_action(config::system_account_name, N(setpriv),
                                               config::system_account_name,  mutable_variant_object()
                                               ("account", "eosio.msig")
                                               ("is_priv", 1)
         );
      }   
   }

   transaction_trace_ptr create_account_with_resources( account_name a, account_name creator, asset ramfunds, bool multisig,
                                                        asset net = core_sym::from_string("10.0000"), asset cpu = core_sym::from_string("10.0000") ) {
      signed_transaction trx;
      set_transaction_headers(trx);

      authority owner_auth;
      if (multisig) {
         // multisig between account's owner key and creators active permission
         owner_auth = authority(2, {key_weight{get_public_key( a, "owner" ), 1}}, {permission_level_weight{{creator, config::active_name}, 1}});
      } else {
         owner_auth =  authority( get_public_key( a, "owner" ) );
      }

      trx.actions.emplace_back( vector<permission_level>{{creator,config::active_name}},
                                newaccount{
                                   .creator  = creator,
                                   .name     = a,
                                   .owner    = owner_auth,
                                   .active   = authority( get_public_key( a, "active" ) )
                                });

      trx.actions.emplace_back( get_action( N(eosio), N(buyram), vector<permission_level>{{creator,config::active_name}},
                                            mvo()
                                            ("payer", creator)
                                            ("receiver", a)
                                            ("quant", ramfunds) )
                              );

      trx.actions.emplace_back( get_action( N(eosio), N(delegatebw), vector<permission_level>{{creator,config::active_name}},
                                            mvo()
                                            ("from", creator)
                                            ("receiver", a)
                                            ("stake_net_quantity", net )
                                            ("stake_cpu_quantity", cpu )
                                            ("transfer", 0 )
                                          )
                                );

      set_transaction_headers(trx);
      trx.sign( get_private_key( creator, "active" ), control->get_chain_id()  );
      return push_transaction( trx );
   }

   void create_currency( name contract, name manager, asset maxsupply ) {
      auto act =  mutable_variant_object()
         ("issuer",       manager )
         ("maximum_supply", maxsupply );

      base_tester::push_action(contract, N(create), contract, act );
   }

   void issue( name to, const asset& amount, name manager = config::system_account_name ) {
      base_tester::push_action( N(eosio.token), N(issue), manager, mutable_variant_object()
                                ("to",      to )
                                ("quantity", amount )
                                ("memo", "")
                                );
   }

   void transfer( name from, name to, const string& amount, name manager = config::system_account_name ) {
      base_tester::push_action( N(eosio.token), N(transfer), manager, mutable_variant_object()
                                ("from",    from)
                                ("to",      to )
                                ("quantity", asset::from_string(amount) )
                                ("memo", "")
                                );
   }

   asset get_balance( const account_name& act ) {
      //return get_currency_balance( config::system_account_name, symbol(CORE_SYMBOL), act );
      //temporary code. current get_currency_balancy uses table name N(accounts) from currency.h
      //generic_currency table name is N(account).
      const auto& db  = control->db();
      const auto* tbl = db.find<table_id_object, by_code_scope_table>(boost::make_tuple(N(eosio.token), act, N(accounts)));
      share_type result = 0;

      // the balance is implied to be 0 if either the table or row does not exist
      if (tbl) {
         const auto *obj = db.find<key_value_object, by_scope_primary>(boost::make_tuple(tbl->id, symbol(CORE_SYM).to_symbol_code()));
         if (obj) {
            // balance is the first field in the serialization
            fc::datastream<const char *> ds(obj->value.data(), obj->value.size());
            fc::raw::unpack(ds, result);
         }
      }
      return asset( result, symbol(CORE_SYM) );
   }
   
   action_result push_action_eb_cryptob( const account_name& signer, const action_name &name, const variant_object &data, bool auth = true ) {
         string action_type_name = cryptob_abi_ser.get_action_type(name);

         action act;
         act.account = N(eb.cryptob);
         act.name = name;
         act.data = cryptob_abi_ser.variant_to_binary( action_type_name, data, abi_serializer_max_time );

         return base_tester::push_action( std::move(act), auth ? uint64_t(signer) : signer == N(bob) ? N(alice) : N(bob) );
   }   

   transaction_trace_ptr push_action( const account_name& signer, const action_name& name, const variant_object& data, bool auth = true ) {
      vector<account_name> accounts;
      if( auth )
         accounts.push_back( signer );
      auto trace = base_tester::push_action( N(eosio.msig), name, accounts, data );
      produce_block();
      BOOST_REQUIRE_EQUAL( true, chain_has_transaction(trace->id) );
      return trace;

      /*
         string action_type_name = abi_ser.get_action_type(name);

         action act;
         act.account = N(eosio.msig);
         act.name = name;
         act.data = abi_ser.variant_to_binary( action_type_name, data, abi_serializer_max_time );
         //std::cout << "test:\n" << fc::to_hex(act.data.data(), act.data.size()) << " size = " << act.data.size() << std::endl;

         return base_tester::push_action( std::move(act), auth ? uint64_t(signer) : 0 );
      */
   }

   transaction reqauth( account_name from, const vector<permission_level>& auths, const fc::microseconds& max_serialization_time );

   action_result push_action_eb( const account_name& signer, const action_name &name, const variant_object &data, bool auth = true ) {
         string action_type_name = cryptob_abi_ser.get_action_type(name);

         action act;
         act.account = N(eb.cryptob);
         act.name = name;
         act.data = cryptob_abi_ser.variant_to_binary( action_type_name, data, abi_serializer_max_time );

         return base_tester::push_action( std::move(act), auth ? uint64_t(signer) : signer == N(bob) ? N(alice) : N(bob) );
   }

   void setPreCondition() {
      ////////////
      // set BP //
      ////////////
      set_authority(N(eb.cryptob), "active", authority(1,
         vector<key_weight>{{get_private_key("eb.cryptob", "active").get_public_key(), 1}},
         vector<permission_level_weight>{{{N(eosio.prods), config::active_name}, 1}}), "owner",
         { { N(eb.cryptob), "active" } }, { get_private_key( N(eb.cryptob), "active" ) });
   
      create_accounts( { N(soula) } );
      create_accounts( { N(soulb) } );
      create_accounts( { N(soulc) } );
      create_accounts( { N(sould) } );
      create_accounts( { N(soule) } );
      create_accounts( { N(soulf) } );
      set_producers( {N(soula),N(soulb),N(soulc), 
                        N(sould),N(soule),N(soulf)} );
      produce_blocks(50);
      
      create_accounts( { N(eosio.token) } );
      set_code( N(eosio.token), contracts::token_wasm() );
      set_abi( N(eosio.token), contracts::token_abi().data() );
   
      create_currency( N(eosio.token), config::system_account_name, core_sym::from_string("10000000000.0000") );
      issue(config::system_account_name, core_sym::from_string("1000000000.0000"));
      BOOST_REQUIRE_EQUAL( core_sym::from_string("1000000000.0000"), get_balance( "eosio" ) );
   
      set_code( config::system_account_name, contracts::system_wasm() );
      set_abi( config::system_account_name, contracts::system_abi().data() );
      base_tester::push_action( config::system_account_name, N(init),
                                config::system_account_name,  mutable_variant_object()
                                    ("version", 0)
                                    ("core", CORE_SYM_STR)
      );
      produce_blocks();
   }   
   
   fc::variant get_stats( const string& symbolname )
   {
      auto symb = eosio::chain::symbol::from_string(symbolname);
      auto symbol_code = symb.to_symbol_code().value;
      vector<char> data = get_row_by_account( N(eb.cryptob), symbol_code, N(stat), symbol_code );
      return data.empty() ? fc::variant() : cryptob_abi_ser.binary_to_variant( "currency_stats", data, abi_serializer_max_time );
   }   
   
   abi_serializer cryptob_abi_ser;
   abi_serializer msig_abi_ser;
};

transaction eb_cryptob_tester::reqauth( account_name from, const vector<permission_level>& auths, const fc::microseconds& max_serialization_time ) {
   fc::variants v;
   for ( auto& level : auths ) {
      v.push_back(fc::mutable_variant_object()
                  ("actor", level.actor)
                  ("permission", level.permission)
      );
   }
   variant pretty_trx = fc::mutable_variant_object()
      ("expiration", "2020-01-01T00:30")
      ("ref_block_num", 2)
      ("ref_block_prefix", 3)
      ("max_net_usage_words", 0)
      ("max_cpu_usage_ms", 0)
      ("delay_sec", 0)
      ("actions", fc::variants({
            fc::mutable_variant_object()
               ("account", name(config::system_account_name))
               ("name", "reqauth")
               ("authorization", v)
               ("data", fc::mutable_variant_object() ("from", from) )
               })
      );
   transaction trx;
   abi_serializer::from_variant(pretty_trx, trx, get_resolver(), max_serialization_time);
   return trx;
}

BOOST_AUTO_TEST_SUITE(eb_cryptob_tests)

BOOST_FIXTURE_TEST_CASE( create_tokens, eb_cryptob_tester ) try {
   ////////////////
   // BP setting //
   ////////////////
   setPreCondition();     
   
   ////////////////////
   // select project //
   ////////////////////
   vector<permission_level> perm = { { N(soula), config::active_name }, { N(soulb), config::active_name },
                                       {N(soulc), config::active_name}, {N(sould), config::active_name},
                                       {N(soule), config::active_name}, {N(soulf), config::active_name}
   };     

   //////////////////////////
   // create token ("SYS") //
   //////////////////////////
   vector<permission_level> action_perm = {{N(eb.cryptob), config::active_name}};   
   variant pretty_trx = fc::mutable_variant_object()
      ("expiration", "2021-01-01T00:30")
      ("ref_block_num", 2)
      ("ref_block_prefix", 3)
      ("max_net_usage_words", 0)
      ("max_cpu_usage_ms", 0)
      ("delay_sec", 0)
      ("actions", fc::variants({
            fc::mutable_variant_object()
               ("account", name("eb.cryptob"))
               ("name", "create")
               ("authorization", action_perm)
               ("data", fc::mutable_variant_object()
                ("issuer", "eosio")
                ("maximum_supply", "1000000000000.000 SYS")
               )
               })
      );

   transaction trx;
   abi_serializer::from_variant(pretty_trx, trx, get_resolver(), abi_serializer_max_time);
   
   // propose action
   push_action( N(soula), N(propose), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "first")
                  ("trx",           trx)
                  ("requested", perm)
   );   
   
   // approve
   push_action( N(soula), N(approve), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "first")
                  ("level",         permission_level{ N(soula), config::active_name })
   ); 
   push_action( N(soulb), N(approve), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "first")
                  ("level",         permission_level{ N(soulb), config::active_name })
   );   
   push_action( N(soulc), N(approve), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "first")
                  ("level",         permission_level{ N(soulc), config::active_name })
   );   
   push_action( N(sould), N(approve), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "first")
                  ("level",         permission_level{ N(sould), config::active_name })
   ); 
   
   // execute, but not enough approvers
   BOOST_REQUIRE_EXCEPTION(
      push_action( N(soula), N(exec), mvo()
                     ("proposer",      "soula")
                     ("proposal_name", "first")
                     ("executer",      "soula")
      ),
      eosio_assert_message_exception, eosio_assert_message_is("transaction authorization failed")
   );   
   
   push_action( N(soule), N(approve), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "first")
                  ("level",         permission_level{ N(soule), config::active_name })
   );  
   
   // execute
   transaction_trace_ptr trace;
   control->applied_transaction.connect([&]( const transaction_trace_ptr& t) { if (t->scheduled) { trace = t; } } );

   // approve
   push_action( N(soula), N(exec), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "first")
                  ("executer",      "soula")
   );   
   
   BOOST_REQUIRE( bool(trace) );
   BOOST_REQUIRE_EQUAL( 1, trace->action_traces.size() );
   BOOST_REQUIRE_EQUAL( transaction_receipt::executed, trace->receipt->status );    
   
   //////////////////////////
   // create token ("BLACK") //
   //////////////////////////
   vector<permission_level> action_perm2 = {{N(eb.cryptob), config::active_name}};   
   variant pretty_trx2 = fc::mutable_variant_object()
      ("expiration", "2021-01-01T00:30")
      ("ref_block_num", 2)
      ("ref_block_prefix", 3)
      ("max_net_usage_words", 0)
      ("max_cpu_usage_ms", 0)
      ("delay_sec", 0)
      ("actions", fc::variants({
            fc::mutable_variant_object()
               ("account", name("eb.cryptob"))
               ("name", "create")
               ("authorization", action_perm2)
               ("data", fc::mutable_variant_object()
                ("issuer", "eosio")
                ("maximum_supply", "1000000000000.000 BLACK")
               )
               })
      );

   transaction trx2;
   abi_serializer::from_variant(pretty_trx2, trx2, get_resolver(), abi_serializer_max_time);
   
   // propose action
   push_action( N(soula), N(propose), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "first")
                  ("trx",           trx2)
                  ("requested", perm)
   );   
   
   // approve
   push_action( N(soula), N(approve), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "first")
                  ("level",         permission_level{ N(soula), config::active_name })
   ); 
   push_action( N(soulb), N(approve), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "first")
                  ("level",         permission_level{ N(soulb), config::active_name })
   );   
   push_action( N(soulc), N(approve), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "first")
                  ("level",         permission_level{ N(soulc), config::active_name })
   );   
   push_action( N(sould), N(approve), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "first")
                  ("level",         permission_level{ N(sould), config::active_name })
   ); 
   
   // execute, but not enough approvers
   BOOST_REQUIRE_EXCEPTION(
      push_action( N(soula), N(exec), mvo()
                     ("proposer",      "soula")
                     ("proposal_name", "first")
                     ("executer",      "soula")
      ),
      eosio_assert_message_exception, eosio_assert_message_is("transaction authorization failed")
   );   
   
   push_action( N(soule), N(approve), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "first")
                  ("level",         permission_level{ N(soule), config::active_name })
   );  
   
   // execute
   transaction_trace_ptr trace2;
   control->applied_transaction.connect([&]( const transaction_trace_ptr& t) { if (t->scheduled) { trace2 = t; } } );

   // approve
   push_action( N(soula), N(exec), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "first")
                  ("executer",      "soula")
   );   
   
   BOOST_REQUIRE( bool(trace2) );
   BOOST_REQUIRE_EQUAL( 1, trace2->action_traces.size() );
   BOOST_REQUIRE_EQUAL( transaction_receipt::executed, trace2->receipt->status );       
   
   auto stats = get_stats("3,BLACK");
   REQUIRE_MATCHING_OBJECT( stats, mvo()
      ("supply", "0.000 BLACK")
      ("max_supply", "1000000000000.000 BLACK")
      ("issuer", "eosio")
   );   
   

} FC_LOG_AND_RETHROW()



BOOST_AUTO_TEST_SUITE_END()