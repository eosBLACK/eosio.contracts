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

class eb_factory_tester : public tester {
public:
   enum members { participants = 0, supporters, reprecandi };
   const char *members_str[4]={ "participants","supporters", "reprecandi" }; 

   eb_factory_tester() {
      create_accounts( { N(eb.factory), N(eb.member), N(eosio.msig), N(eosio.stake), N(eosio.ram), N(eosio.ramfee), N(eb.cryptob) } );
      
      produce_block();      
      
      // deploy eb.factory
      set_code( N(eb.factory), contracts::factory_wasm());
      set_abi( N(eb.factory), contracts::factory_abi().data() );
      {
         const auto& accnt = control->db().get<account_object,by_name>( N(eb.factory) );
         abi_def abi;
         BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
         factory_abi_ser.set_abi(abi, abi_serializer_max_time);
      } 
      
      {
         auto trace = base_tester::push_action(config::system_account_name, N(setpriv),
                                               config::system_account_name,  mutable_variant_object()
                                               ("account", "eb.factory")
                                               ("is_priv", 1)
         );
      }       
      
      // deploy eb.member
      // for inline-action
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

   action_result push_action_eb_member( const account_name& signer, const action_name &name, const variant_object &data, bool auth = true ) {
         string action_type_name = member_abi_ser.get_action_type(name);

         action act;
         act.account = N(eb.member);
         act.name = name;
         act.data = member_abi_ser.variant_to_binary( action_type_name, data, abi_serializer_max_time );

         return base_tester::push_action( std::move(act), auth ? uint64_t(signer) : signer == N(bob111111111) ? N(alice1111111) : N(bob111111111) );
   }

   action_result push_action_eb( const account_name& signer, const action_name &name, const variant_object &data, bool auth = true ) {
         string action_type_name = factory_abi_ser.get_action_type(name);

         action act;
         act.account = N(eb.factory);
         act.name = name;
         act.data = factory_abi_ser.variant_to_binary( action_type_name, data, abi_serializer_max_time );

         return base_tester::push_action( std::move(act), auth ? uint64_t(signer) : signer == N(bob) ? N(alice) : N(bob) );
   }

   // For membership checking, call inline-action internally 
   // eb.member -> ismember()
   action_result create_eb( const account_name& owner, 
                              const std::string& project_name, 
                              const std::string& url, 
                              const std::string& hash,
                              const std::string& homepage,
                              const std::string& icon,
                              const account_name& helper) {
      return push_action_eb( name(owner), N(create), mvo()
                          ("owner",     owner)
                          ("project_name", project_name)
                          ("url", url)
                          ("hash", hash)
                          ("homepage", homepage )
                          ("icon", icon )
                          ("helper", helper )
      );
   }  
   
   action_result addinfo_eb( const account_name& owner, 
                              const uint64_t& project_index, 
                              const std::string& url, 
                              const std::string& hash,
                              const std::string& homepage,
                              const std::string& icon,
                              const account_name& helper) {
      return push_action_eb( name(owner), N(addinfo), mvo()
                          ("project_index", project_index)
                          ("url", url)
                          ("hash", hash)
                          ("homepage", homepage )
                          ("icon", icon )
                          ("helper", helper )
      );
   }    
   
   action_result settoken_eb( const account_name& owner, 
                              const uint64_t& project_index, 
                              const account_name& issuer,
                              const asset& maximum_supply) {
      return push_action_eb( name(owner), N(settoken), mvo()
                          ("project_index", project_index)
                          ("issuer", issuer)
                          ("maximum_supply", maximum_supply)
      );
   }      
   
   action_result addresource_eb( const account_name& owner, 
                              const uint64_t& project_index, 
                              const uint64_t& ram_max_size, 
                              const asset& black_max_count) {
      return push_action_eb( name(owner), N(addresource), mvo()
                          ("project_index", project_index)
                          ("ram_max_size", ram_max_size)
                          ("black_max_count", black_max_count)
      );
   }    
   
   action_result addpayment_eb( const account_name& owner, 
                              const uint64_t& project_index, 
                              const uint64_t& aftertime, 
                              const int8_t& percentage) {
      return push_action_eb( name(owner), N(addpayment), mvo()
                          ("project_index", project_index)
                          ("aftertime", aftertime)
                          ("percentage", percentage)
      );
   }   
   
   action_result rmresource_eb( const account_name& owner, 
                              const uint64_t& project_index, 
                              const uint64_t& resource_index) {
      return push_action_eb( name(owner), N(rmresource), mvo()
                          ("project_index", project_index)
                          ("resource_index", resource_index)
      );
   }    
   
   action_result rmpayment_eb( const account_name& owner, 
                              const uint64_t& project_index, 
                              const uint64_t& payment_index) {
      return push_action_eb( name(owner), N(rmpayment), mvo()
                          ("project_index", project_index)
                          ("payment_index", payment_index)
      );
   }      
   
   action_result drop_eb( const account_name& owner, 
                              const uint64_t& project_index, 
                              const std::string& state) {
      return push_action_eb( name(owner), N(drop), mvo()
                          ("project_index", project_index)
                          ("state", state)
      );
   }   
   
   action_result setready_eb( const account_name& owner, 
                              const uint64_t& project_index, 
                              const uint64_t& detail_index,
                              const uint64_t& resource_index) {
      return push_action_eb( name(owner), N(setready), mvo()
                          ("project_index", project_index)
                          ("detail_index", detail_index)
                          ("resource_index", resource_index)
      );
   } 
   
   action_result cancelready_eb( const account_name& owner, 
                              const uint64_t& project_index) {
      return push_action_eb( name(owner), N(cancelready), mvo()
                          ("project_index", project_index)
      );
   }   
   
   fc::variant get_created_project(const uint64_t& index)
   {
      vector<char> data = get_row_by_account( N(eb.factory), N(created), N(projects), index );  
      return data.empty() ? fc::variant() : factory_abi_ser.binary_to_variant( "project", data, abi_serializer_max_time );
   }  
   
   fc::variant get_selected_project(const uint64_t& index)
   {
      vector<char> data = get_row_by_account( N(eb.factory), N(selected), N(projects), index );  
      return data.empty() ? fc::variant() : factory_abi_ser.binary_to_variant( "project", data, abi_serializer_max_time );
   }  
   
   fc::variant get_readied_project(const uint64_t& index)
   {
      vector<char> data = get_row_by_account( N(eb.factory), N(readied), N(projects), index );  
      return data.empty() ? fc::variant() : factory_abi_ser.binary_to_variant( "project", data, abi_serializer_max_time );
   }   
   
   fc::variant get_started_project(const uint64_t& index)
   {
      vector<char> data = get_row_by_account( N(eb.factory), N(started), N(projects), index );  
      return data.empty() ? fc::variant() : factory_abi_ser.binary_to_variant( "project", data, abi_serializer_max_time );
   }    
   
   fc::variant get_projectinfo(const uint64_t& scope, const uint64_t& index)
   {
      vector<char> data = get_row_by_account( N(eb.factory), scope, N(projectinfos), index );  
      return data.empty() ? fc::variant() : factory_abi_ser.binary_to_variant( "projectinfo", data, abi_serializer_max_time );
   } 
   
   fc::variant get_resource(const uint64_t& scope, const uint64_t& index)
   {
      vector<char> data = get_row_by_account( N(eb.factory), scope, N(resources), index );  
      return data.empty() ? fc::variant() : factory_abi_ser.binary_to_variant( "resource", data, abi_serializer_max_time );
   }   
   
   fc::variant get_payment(const uint64_t& scope, const uint64_t& index)
   {
      vector<char> data = get_row_by_account( N(eb.factory), scope, N(payments), index );  
      return data.empty() ? fc::variant() : factory_abi_ser.binary_to_variant( "payment", data, abi_serializer_max_time );
   }    
   
   fc::variant get_token(const uint64_t& scope, const uint64_t& index)
   {
      vector<char> data = get_row_by_account( N(eb.factory), scope, N(tokens), index );  
      return data.empty() ? fc::variant() : factory_abi_ser.binary_to_variant( "token", data, abi_serializer_max_time );
   }    
     
   action_result setcriteria( const name& member_type, const asset& low_quantity, const asset& high_quantity ) {
      return push_action_eb_member( N(eb.member), N(setcriteria), mvo()
                           ("member_type", member_type)
                           ("low_quantity", low_quantity)
                           ("high_quantity", high_quantity)
      );
   }
   
   action_result stake_eb_member( const account_name& from, const account_name& to, const asset& net, const asset& cpu ) {
      return push_action_eb_member( name(from), N(staking), mvo()
                          ("from",     from)
                          ("receiver", to)
                          ("stake_net_quantity", net)
                          ("stake_cpu_quantity", cpu)
                          ("transfer", 0 )
      );
   }   

   fc::variant get_criteria(const name& account)
   {
      vector<char> data = get_row_by_account( N(eb.member), N(eb.member), N(criterias), account.value );  
      return data.empty() ? fc::variant() : member_abi_ser.binary_to_variant( "criteria", data, abi_serializer_max_time );
   }   
   
   fc::variant get_stats( const string& symbolname )
   {
      auto symb = eosio::chain::symbol::from_string(symbolname);
      auto symbol_code = symb.to_symbol_code().value;
      vector<char> data = get_row_by_account( N(eb.cryptob), symbol_code, N(stat), symbol_code );
      return data.empty() ? fc::variant() : cryptob_abi_ser.binary_to_variant( "currency_stats", data, abi_serializer_max_time );
   }   
   
   
   void setPreCondition(string participant_low, string participant_high, 
                        string supporter_low, string supporter_high,
                        string representative_low, string representative_high) {
      //////////////////
      // set criteria //
      //////////////////
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
      
      BOOST_REQUIRE_EQUAL( success(), setcriteria( name(members_str[reprecandi]), asset::from_string(representative_low), asset::from_string(representative_high)) );
      criteria = get_criteria(name(members_str[reprecandi]));
      REQUIRE_MATCHING_OBJECT( criteria, mvo()
         ("member_type", members_str[reprecandi])
         ("low_quantity", representative_low)
         ("high_quantity", representative_high)
      );    
      
      ////////////
      // set BP //
      ////////////
      set_authority(N(eb.factory), "active", authority(1,
         vector<key_weight>{{get_private_key("eb.factory", "active").get_public_key(), 1}},
         vector<permission_level_weight>{{{N(eosio.prods), config::active_name}, 1}}), "owner",
         { { N(eb.factory), "active" } }, { get_private_key( N(eb.factory), "active" ) });
   
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
      
      /////////////////////
      // join membership //      
      /////////////////////
      create_account_with_resources( N(alice), N(eosio), core_sym::from_string("1.0000"), false );
      create_account_with_resources( N(bob), N(eosio), core_sym::from_string("0.4500"), false );
      create_account_with_resources( N(carol), N(eosio), core_sym::from_string("1.0000"), false );      

      // pre-condition
      // init staking amount: 20
      BOOST_REQUIRE_EQUAL( core_sym::from_string("0.0000"), get_balance( "alice" ) );
      transfer( "eosio", "alice", "5000.0000 BLACK", "eosio" );
      BOOST_REQUIRE_EQUAL( core_sym::from_string("5000.0000"), get_balance( "alice" ) );

      BOOST_REQUIRE_EQUAL( core_sym::from_string("0.0000"), get_balance( "bob" ) );
      transfer( "eosio", "bob", "5000.0000 BLACK", "eosio" );
      BOOST_REQUIRE_EQUAL( core_sym::from_string("5000.0000"), get_balance( "bob" ) );

      BOOST_REQUIRE_EQUAL( core_sym::from_string("0.0000"), get_balance( "carol" ) );
      transfer( "eosio", "carol", "5000.0000  BLACK", "eosio" );
      BOOST_REQUIRE_EQUAL( core_sym::from_string("5000.0000"), get_balance( "carol" ) );

      // total staking amount: 50
      BOOST_REQUIRE_EQUAL( success(), stake_eb_member( "alice", "alice", core_sym::from_string("15.0000"), core_sym::from_string("15.0000") ) );
      BOOST_REQUIRE_EQUAL( success(), stake_eb_member( "bob", "bob", core_sym::from_string("15.0000"), core_sym::from_string("15.0000") ) );
      BOOST_REQUIRE_EQUAL( success(), stake_eb_member( "carol", "carol", core_sym::from_string("15.0000"), core_sym::from_string("15.0000") ) );
   }   
   
   abi_serializer factory_abi_ser;
   abi_serializer msig_abi_ser;
   abi_serializer member_abi_ser;
   abi_serializer cryptob_abi_ser;
};

transaction eb_factory_tester::reqauth( account_name from, const vector<permission_level>& auths, const fc::microseconds& max_serialization_time ) {
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

BOOST_AUTO_TEST_SUITE(eb_factory_tests)

string proj_1_owner = "alice";
string proj_1_name = "test_project_1";
string proj_1_url = "www.test_1.io";
string proj_1_hash = "test_hash_1";
string proj_1_homepage = "www.homepage_1.com";
string proj_1_icon = "www.icon_1.io";
string proj_1_helper = "alice";

string proj_1_2_url = "www.test_1_2.io";
string proj_1_2_hash = "test_hash_1_2";
string proj_1_2_homepage = "www.homepage_1_2.com";
string proj_1_2_icon = "www.icon_1_2.io";

string proj_2_owner = "bob";
string proj_2_name = "test_project_2";
string proj_2_url = "www.test_2.io";
string proj_2_hash = "test_hash_2";
string proj_2_homepage = "www.homepage_2.com";
string proj_2_icon = "www.icon_2.io";
string proj_2_helper = "bob";   

string proj_3_owner = "carol";
string proj_3_name = "test_project_3";
string proj_3_url = "www.test_3.io";
string proj_3_hash = "test_hash_3";
string proj_3_homepage = "www.homepage_3.com";
string proj_3_icon = "www.icon_3.io";
string proj_3_helper = "carol";   

uint64_t project_1_index = 0;                                      
uint64_t project_2_index = 1;
uint64_t project_3_index = 2;
uint64_t project_4_index = 3;

uint64_t project_1_info_1_index = 0;
uint64_t project_1_info_1_2_index = 1;
uint64_t project_2_info_1_index = 0;
uint64_t project_3_info_1_index = 0;

uint64_t resource_1_index = 0;
uint64_t resource_1_2_index = 1;   
uint64_t resource_1_3_index = 2;   

uint64_t payment_1_index = 0;
uint64_t payment_1_2_index = 1;      
uint64_t payment_1_3_index = 2;

uint64_t ram_max_size_1 = 30;
asset black_max_count_1 = core_sym::from_string("200.0000");
uint64_t ram_max_size_2 = 20;
asset black_max_count_2 = core_sym::from_string("300.0000");   
uint64_t ram_max_size_3 = 100;
asset black_max_count_3 = core_sym::from_string("10.0000");   

uint64_t aftertime_1 = 10000;
int8_t percentage_1 = 40;
uint64_t aftertime_2 = 20000;
int8_t percentage_2 = 60;   
uint64_t aftertime_3 = 30000;
int8_t percentage_3 = 60; 

BOOST_FIXTURE_TEST_CASE( create_add_rm_drop, eb_factory_tester ) try {
   ///////////////////
   // pre-condition //
   ///////////////////
   
   // 1. set criterias
   // participant: 50 ~ 99
   // supporter: 100 ~ 199
   // representative_candidate: 200 ~ 500
   
   // 2. BP setting
   
   // 3. Join membership
   // alice, bob, carol
   setPreCondition("50.0000 BLACK", "99.0000 BLACK", 
                  "100.0000 BLACK", "199.0000 BLACK", 
                  "200.0000 BLACK", "500.0000 BLACK");     
   
   ////////////////////
   // create project //
   ////////////////////   
   // create project_1
   BOOST_REQUIRE_EQUAL( success(), create_eb( proj_1_owner,
                                                proj_1_name, 
                                                proj_1_url, 
                                                proj_1_hash,
                                                proj_1_homepage,
                                                proj_1_icon,
                                                proj_1_helper) );
   produce_blocks( 10 );
   
   // create project_2
   BOOST_REQUIRE_EQUAL( success(), create_eb( proj_2_owner,
                                                proj_2_name, 
                                                proj_2_url, 
                                                proj_2_hash,
                                                proj_2_homepage,
                                                proj_2_icon,
                                                proj_2_helper) );                                            

   produce_blocks( 10 );
   
   // create project_3
   BOOST_REQUIRE_EQUAL( success(), create_eb( proj_3_owner,
                                                proj_3_name, 
                                                proj_3_url, 
                                                proj_3_hash,
                                                proj_3_homepage,
                                                proj_3_icon,
                                                proj_3_helper) ); 

   produce_blocks( 10 );
   
   // create existed project_1
   BOOST_REQUIRE_EQUAL( wasm_assert_msg("already project name exist"), 
                        create_eb( proj_3_owner,
                                    proj_1_name, 
                                    proj_1_url, 
                                    proj_1_hash,
                                    proj_1_homepage,
                                    proj_1_icon,
                                    proj_1_helper) );                                                 
   produce_blocks( 10 );
   
   // add project_1 info
   BOOST_REQUIRE_EQUAL( success(), addinfo_eb( proj_1_owner,
                                                project_1_index, 
                                                proj_1_2_url, 
                                                proj_1_2_hash,
                                                proj_1_2_homepage,
                                                proj_1_2_icon,
                                                proj_1_helper) );    
   produce_blocks( 10 );
   
   // add project_1 resource
   BOOST_REQUIRE_EQUAL( success(), addresource_eb( proj_1_owner,
                                                project_1_index, 
                                                ram_max_size_1, 
                                                black_max_count_1) );  
   BOOST_REQUIRE_EQUAL( success(), addresource_eb( proj_1_owner,
                                                project_1_index, 
                                                ram_max_size_2, 
                                                black_max_count_2) );     
   BOOST_REQUIRE_EQUAL( success(), addresource_eb( proj_1_owner,
                                                project_1_index, 
                                                ram_max_size_3, 
                                                black_max_count_3) );                                                      
   produce_blocks( 10 );
   
   // add project_1 payment    
   BOOST_REQUIRE_EQUAL( success(), addpayment_eb( proj_1_owner,
                                                project_1_index, 
                                                aftertime_1, 
                                                percentage_1) );  
   BOOST_REQUIRE_EQUAL( success(), addpayment_eb( proj_1_owner,
                                                project_1_index, 
                                                aftertime_2, 
                                                percentage_2) );   
   BOOST_REQUIRE_EQUAL( success(), addpayment_eb( proj_1_owner,
                                                project_1_index, 
                                                aftertime_3, 
                                                percentage_3) );                                                 

   // check project_1
   auto proj_1 = get_created_project(project_1_index);
   BOOST_REQUIRE_EQUAL(false, proj_1.is_null());
   BOOST_REQUIRE_EQUAL(proj_1["owner"].as_string(), proj_1_owner);
   BOOST_REQUIRE_EQUAL(proj_1["proj_name"].as_string(), proj_1_name);
   
   auto projinfo_1 = get_projectinfo(project_1_index, project_1_info_1_index);
   BOOST_REQUIRE_EQUAL(false, projinfo_1.is_null());
   BOOST_REQUIRE_EQUAL(projinfo_1["url"].as_string(), proj_1_url);
   BOOST_REQUIRE_EQUAL(projinfo_1["hash"].as_string(), proj_1_hash);
   BOOST_REQUIRE_EQUAL(projinfo_1["homepage"].as_string(), proj_1_homepage);
   BOOST_REQUIRE_EQUAL(projinfo_1["icon"].as_string(), proj_1_icon);
   BOOST_REQUIRE_EQUAL(projinfo_1["helper"].as_string(), proj_1_helper);
   
   auto projinfo_1_2 = get_projectinfo(project_1_index, project_1_info_1_2_index);
   BOOST_REQUIRE_EQUAL(false, projinfo_1_2.is_null());
   BOOST_REQUIRE_EQUAL(projinfo_1_2["url"].as_string(), proj_1_2_url);
   BOOST_REQUIRE_EQUAL(projinfo_1_2["hash"].as_string(), proj_1_2_hash);
   BOOST_REQUIRE_EQUAL(projinfo_1_2["homepage"].as_string(), proj_1_2_homepage);
   BOOST_REQUIRE_EQUAL(projinfo_1_2["icon"].as_string(), proj_1_2_icon);
   BOOST_REQUIRE_EQUAL(projinfo_1_2["helper"].as_string(), proj_1_helper);  
   
   auto resource_1 = get_resource(project_1_index, resource_1_index);
   BOOST_REQUIRE_EQUAL(false, resource_1.is_null());
   BOOST_REQUIRE_EQUAL(resource_1["ram_max_size"].as_uint64(), ram_max_size_1);
   BOOST_REQUIRE_EQUAL(resource_1["black_max_count"].as<asset>(), black_max_count_1);
   
   auto resource_1_2 = get_resource(project_1_index, resource_1_2_index);
   BOOST_REQUIRE_EQUAL(false, resource_1_2.is_null());  
   BOOST_REQUIRE_EQUAL(resource_1_2["ram_max_size"].as_uint64(), ram_max_size_2);
   BOOST_REQUIRE_EQUAL(resource_1_2["black_max_count"].as<asset>(), black_max_count_2);   

   auto resource_1_3 = get_resource(project_1_index, resource_1_3_index);
   BOOST_REQUIRE_EQUAL(false, resource_1_3.is_null());  
   BOOST_REQUIRE_EQUAL(resource_1_3["ram_max_size"].as_uint64(), ram_max_size_3);
   BOOST_REQUIRE_EQUAL(resource_1_3["black_max_count"].as<asset>(), black_max_count_3);      
   
   auto payment_1 = get_payment(project_1_index, payment_1_index);
   BOOST_REQUIRE_EQUAL(false, payment_1.is_null());
   BOOST_REQUIRE_EQUAL(payment_1["aftertime"].as_uint64(), aftertime_1);
   BOOST_REQUIRE_EQUAL(payment_1["percentage"].as_uint64(), percentage_1);
   
   auto payment_1_2 = get_payment(project_1_index, payment_1_2_index);
   BOOST_REQUIRE_EQUAL(false, payment_1_2.is_null());  
   BOOST_REQUIRE_EQUAL(payment_1_2["aftertime"].as_uint64(), aftertime_2);
   BOOST_REQUIRE_EQUAL(payment_1_2["percentage"].as_uint64(), percentage_2);  
   
   auto payment_1_3 = get_payment(project_1_index, payment_1_3_index);
   BOOST_REQUIRE_EQUAL(false, payment_1_3.is_null());  
   BOOST_REQUIRE_EQUAL(payment_1_3["aftertime"].as_uint64(), aftertime_3);
   BOOST_REQUIRE_EQUAL(payment_1_3["percentage"].as_uint64(), percentage_3);     
   
   BOOST_REQUIRE_EQUAL( success(), rmresource_eb( proj_1_owner,
                                                project_1_index, 
                                                resource_1_2_index) );    
   BOOST_REQUIRE_EQUAL( success(), rmpayment_eb( proj_1_owner,
                                                project_1_index, 
                                                payment_1_2_index) );                                                  

   auto resource_1_2_removed = get_resource(project_1_index, resource_1_2_index);
   BOOST_REQUIRE_EQUAL(true, resource_1_2_removed.is_null()); 
   auto payment_1_2_removed = get_payment(project_1_index, payment_1_2_index);
   BOOST_REQUIRE_EQUAL(true, payment_1_2_removed.is_null());    
   
   // check project_2
   auto proj_2 = get_created_project(project_2_index);
   BOOST_REQUIRE_EQUAL(false, proj_2.is_null());
   BOOST_REQUIRE_EQUAL(proj_2["owner"].as_string(), proj_2_owner);
   BOOST_REQUIRE_EQUAL(proj_2["proj_name"].as_string(), proj_2_name);   
   
   auto projinfo_2 = get_projectinfo(project_2_index, project_2_info_1_index);
   BOOST_REQUIRE_EQUAL(false, projinfo_2.is_null());
   BOOST_REQUIRE_EQUAL(projinfo_2["url"].as_string(), proj_2_url);
   BOOST_REQUIRE_EQUAL(projinfo_2["hash"].as_string(), proj_2_hash);
   BOOST_REQUIRE_EQUAL(projinfo_2["homepage"].as_string(), proj_2_homepage);
   BOOST_REQUIRE_EQUAL(projinfo_2["icon"].as_string(), proj_2_icon);
   BOOST_REQUIRE_EQUAL(projinfo_2["helper"].as_string(), proj_2_helper);   
   
   // check project_2
   auto proj_3 = get_created_project(project_3_index);
   BOOST_REQUIRE_EQUAL(false, proj_3.is_null());
   BOOST_REQUIRE_EQUAL(proj_3["owner"].as_string(), proj_3_owner);
   BOOST_REQUIRE_EQUAL(proj_3["proj_name"].as_string(), proj_3_name);   
   
   auto projinfo_3 = get_projectinfo(project_3_index, project_3_info_1_index);
   BOOST_REQUIRE_EQUAL(false, projinfo_3.is_null());
   BOOST_REQUIRE_EQUAL(projinfo_3["url"].as_string(), proj_3_url);
   BOOST_REQUIRE_EQUAL(projinfo_3["hash"].as_string(), proj_3_hash);
   BOOST_REQUIRE_EQUAL(projinfo_3["homepage"].as_string(), proj_3_homepage);
   BOOST_REQUIRE_EQUAL(projinfo_3["icon"].as_string(), proj_3_icon);
   BOOST_REQUIRE_EQUAL(projinfo_3["helper"].as_string(), proj_3_helper);   
   
   // try project dropping
   BOOST_REQUIRE_EQUAL("missing authority of alice", 
                        drop_eb(name(proj_2_owner), project_1_index, "created"));   
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("project doesn't exist"), 
                        drop_eb(name(proj_1_owner), project_4_index, "created"));   
                    
   // drop & check project 
   BOOST_REQUIRE_EQUAL(success(), 
                        drop_eb(name(proj_1_owner), project_1_index, "created"));   
   
   auto proj_1_dropped = get_created_project(project_1_index);
   BOOST_REQUIRE_EQUAL(true, proj_1_dropped.is_null());   
   
   auto projinfo_1_dropped = get_projectinfo(project_1_index, project_1_info_1_index);
   BOOST_REQUIRE_EQUAL(true, projinfo_1_dropped.is_null());
   auto projinfo_1_2_dropped = get_projectinfo(project_1_index, project_1_info_1_2_index);
   BOOST_REQUIRE_EQUAL(true, projinfo_1_2_dropped.is_null());   
   
   auto resource_1_dropped = get_resource(project_1_index, resource_1_index);
   BOOST_REQUIRE_EQUAL(true, resource_1_dropped.is_null());
   auto resource_1_3_dropped = get_resource(project_1_index, resource_1_3_index);
   BOOST_REQUIRE_EQUAL(true, resource_1_3_dropped.is_null());     

   auto payment_1_dropped = get_payment(project_1_index, payment_1_index);
   BOOST_REQUIRE_EQUAL(true, payment_1_dropped.is_null());
   auto payment_1_3_dropped = get_payment(project_1_index, payment_1_3_index);
   BOOST_REQUIRE_EQUAL(true, payment_1_3_dropped.is_null());     
   
   // create dropped project_1   
   BOOST_REQUIRE_EQUAL( success(), create_eb( proj_1_owner,
                                                proj_1_name, 
                                                proj_1_url, 
                                                proj_1_hash,
                                                proj_1_homepage,
                                                proj_1_icon,
                                                proj_1_helper) );   
                                                
   auto proj_1_recreated = get_created_project(project_4_index);
   BOOST_REQUIRE_EQUAL(false, proj_1_recreated.is_null());
   BOOST_REQUIRE_EQUAL(proj_1_recreated["owner"].as_string(), proj_1_owner);
   BOOST_REQUIRE_EQUAL(proj_1_recreated["proj_name"].as_string(), "test_project_1");                                                
   
   /*
   std::cout << "proj_1_name_hash: " << proj_1["proj_name_hash"].as_string() << std::endl;
   std::cout << "proj_2_name_hash: " << proj_2["proj_name_hash"].as_string() << std::endl;
   std::cout << "proj_3_name_hash: " << proj_3["proj_name_hash"].as_string() << std::endl;
   
   std::cout << "proj_1_updatetime: " << projinfo_1["updatetime"].as_string() << std::endl;
   std::cout << "proj_2_updatetime: " << projinfo_2["updatetime"].as_string() << std::endl;   
   std::cout << "proj_3_updatetime: " << projinfo_3["updatetime"].as_string() << std::endl; 
   */
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( select_drop, eb_factory_tester ) try {
   ///////////////////
   // pre-condition //
   ///////////////////
   
   // 1. set criterias
   // participant: 50 ~ 99
   // supporter: 100 ~ 199
   // representative_candidate: 200 ~ 500
   
   // 2. BP setting
   
   // 3. Join membership
   // alice, bob, carol
   setPreCondition("50.0000 BLACK", "99.0000 BLACK", 
                  "100.0000 BLACK", "199.0000 BLACK", 
                  "200.0000 BLACK", "500.0000 BLACK");   
                  
   ////////////////////
   // create project //
   ////////////////////   
   // create project_1
   BOOST_REQUIRE_EQUAL( success(), create_eb( proj_1_owner,
                                                proj_1_name, 
                                                proj_1_url, 
                                                proj_1_hash,
                                                proj_1_homepage,
                                                proj_1_icon,
                                                "") );
   produce_blocks( 10 );
   
   // create project_2
   BOOST_REQUIRE_EQUAL( success(), create_eb( proj_2_owner,
                                                proj_2_name, 
                                                proj_2_url, 
                                                proj_2_hash,
                                                proj_2_homepage,
                                                proj_2_icon,
                                                proj_1_helper) );                                            
   produce_blocks( 10 );
   
   // add project_1 info
   BOOST_REQUIRE_EQUAL( success(), addinfo_eb( proj_1_owner,
                                                project_1_index, 
                                                proj_1_2_url, 
                                                proj_1_2_hash,
                                                proj_1_2_homepage,
                                                proj_1_2_icon,
                                                proj_1_helper) );    
   produce_blocks( 10 );
   
   // add project_1 resource
   BOOST_REQUIRE_EQUAL( success(), addresource_eb( proj_1_owner,
                                                project_1_index, 
                                                ram_max_size_1, 
                                                black_max_count_1) );  
   BOOST_REQUIRE_EQUAL( success(), addresource_eb( proj_1_owner,
                                                project_1_index, 
                                                ram_max_size_2, 
                                                black_max_count_2) );     
   BOOST_REQUIRE_EQUAL( success(), addresource_eb( proj_1_owner,
                                                project_1_index, 
                                                ram_max_size_3, 
                                                black_max_count_3) );
   produce_blocks( 10 );
   
   // add project_1 payment
   BOOST_REQUIRE_EQUAL( success(), addpayment_eb( proj_1_owner,
                                                project_1_index, 
                                                aftertime_1, 
                                                percentage_1) );  
   BOOST_REQUIRE_EQUAL( success(), addpayment_eb( proj_1_owner,
                                                project_1_index, 
                                                aftertime_2, 
                                                percentage_2) );   
   BOOST_REQUIRE_EQUAL( success(), addpayment_eb( proj_1_owner,
                                                project_1_index, 
                                                aftertime_3, 
                                                percentage_3) );                                                 

   ////////////////////
   // select project //
   ////////////////////
   vector<permission_level> perm = { { N(soula), config::active_name }, { N(soulb), config::active_name },
                                       {N(soulc), config::active_name}, {N(sould), config::active_name},
                                       {N(soule), config::active_name}, {N(soulf), config::active_name}
   };     

   // select project_1
   // helper is updated
   vector<permission_level> action_perm = {{N(eb.factory), config::active_name}};   
   variant pretty_trx = fc::mutable_variant_object()
      ("expiration", "2021-01-01T00:30")
      ("ref_block_num", 2)
      ("ref_block_prefix", 3)
      ("max_net_usage_words", 0)
      ("max_cpu_usage_ms", 0)
      ("delay_sec", 0)
      ("actions", fc::variants({
            fc::mutable_variant_object()
               ("account", name("eb.factory"))
               ("name", "select")
               ("authorization", action_perm)
               ("data", fc::mutable_variant_object()
                ("project_index", project_1_index)
                ("detail_index", project_1_info_1_2_index)
                ("resource_index", resource_1_3_index)
                ("helper", proj_1_helper)
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
   
   // checking
   auto proj_1_remvoed = get_created_project(project_1_index);
   BOOST_REQUIRE_EQUAL(true, proj_1_remvoed.is_null());

   auto proj_1_selected = get_selected_project(project_1_index);
   BOOST_REQUIRE_EQUAL(false, proj_1_selected.is_null());   
   BOOST_REQUIRE_EQUAL(proj_1_selected["owner"].as_string(), proj_1_owner);
   BOOST_REQUIRE_EQUAL(proj_1_selected["proj_name"].as_string(), proj_1_name);   
   BOOST_REQUIRE_EQUAL(proj_1_selected["selected_detail_index"].as_uint64(), project_1_info_1_2_index);
   BOOST_REQUIRE_EQUAL(proj_1_selected["selected_resource_index"].as_uint64(), resource_1_3_index); 
   
   auto projinfo_1_selected = get_projectinfo(project_1_index, project_1_info_1_2_index);
   BOOST_REQUIRE_EQUAL(false, projinfo_1_selected.is_null());
   BOOST_REQUIRE_EQUAL(projinfo_1_selected["url"].as_string(), proj_1_2_url);
   BOOST_REQUIRE_EQUAL(projinfo_1_selected["hash"].as_string(), proj_1_2_hash);
   BOOST_REQUIRE_EQUAL(projinfo_1_selected["homepage"].as_string(), proj_1_2_homepage);
   BOOST_REQUIRE_EQUAL(projinfo_1_selected["icon"].as_string(), proj_1_2_icon);
   BOOST_REQUIRE_EQUAL(projinfo_1_selected["helper"].as_string(), proj_1_owner);     
   
   // select project_2
   // it is to be failed, because helper is different
   vector<permission_level> action_perm2 = {{N(eb.factory), config::active_name}};   
   variant pretty_trx2 = fc::mutable_variant_object()
      ("expiration", "2021-01-01T00:30")
      ("ref_block_num", 2)
      ("ref_block_prefix", 3)
      ("max_net_usage_words", 0)
      ("max_cpu_usage_ms", 0)
      ("delay_sec", 0)
      ("actions", fc::variants({
            fc::mutable_variant_object()
               ("account", name("eb.factory"))
               ("name", "select")
               ("authorization", action_perm2)
               ("data", fc::mutable_variant_object()
                ("project_index", project_2_index)
                ("detail_index", 0)
                ("resource_index", 0)
                ("helper", name(proj_2_owner))
               )
               })
      );

   transaction trx2;
   abi_serializer::from_variant(pretty_trx2, trx2, get_resolver(), abi_serializer_max_time);
   
   // propose action
   push_action( N(soula), N(propose), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "second")
                  ("trx",           trx2)
                  ("requested", perm)
   );   
   
   //approve
   push_action( N(soula), N(approve), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "second")
                  ("level",         permission_level{ N(soula), config::active_name })
   ); 
   push_action( N(soulb), N(approve), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "second")
                  ("level",         permission_level{ N(soulb), config::active_name })
   );   
   push_action( N(soulc), N(approve), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "second")
                  ("level",         permission_level{ N(soulc), config::active_name })
   );   
   push_action( N(sould), N(approve), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "second")
                  ("level",         permission_level{ N(sould), config::active_name })
   );  
   push_action( N(soule), N(approve), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "second")
                  ("level",         permission_level{ N(soule), config::active_name })
   );  
   
   // execute
   transaction_trace_ptr trace2;
   control->applied_transaction.connect([&]( const transaction_trace_ptr& t) { if (t->scheduled) { trace2 = t; } } );

   push_action( N(soula), N(exec), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "second")
                  ("executer",      "soula")
   );   
   
   //BOOST_REQUIRE( bool(trace2) );
   BOOST_REQUIRE_EQUAL( false, bool(trace2) );
   
   // select project_2 again
   vector<permission_level> action_perm3 = {{N(eb.factory), config::active_name}};   
   variant pretty_trx3 = fc::mutable_variant_object()
      ("expiration", "2021-01-01T00:30")
      ("ref_block_num", 2)
      ("ref_block_prefix", 3)
      ("max_net_usage_words", 0)
      ("max_cpu_usage_ms", 0)
      ("delay_sec", 0)
      ("actions", fc::variants({
            fc::mutable_variant_object()
               ("account", name("eb.factory"))
               ("name", "select")
               ("authorization", action_perm3)
               ("data", fc::mutable_variant_object()
                ("project_index", project_2_index)
                ("detail_index", 0)
                ("resource_index", 0)
                ("helper", name(proj_1_owner))
               )
               })
      );

   transaction trx3;
   abi_serializer::from_variant(pretty_trx3, trx3, get_resolver(), abi_serializer_max_time);
   
   // propose action
   push_action( N(soula), N(propose), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "third")
                  ("trx",           trx3)
                  ("requested", perm)
   );   
   
   //approve
   push_action( N(soula), N(approve), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "third")
                  ("level",         permission_level{ N(soula), config::active_name })
   ); 
   push_action( N(soulb), N(approve), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "third")
                  ("level",         permission_level{ N(soulb), config::active_name })
   );   
   push_action( N(soulc), N(approve), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "third")
                  ("level",         permission_level{ N(soulc), config::active_name })
   );   
   push_action( N(sould), N(approve), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "third")
                  ("level",         permission_level{ N(sould), config::active_name })
   );  
   push_action( N(soule), N(approve), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "third")
                  ("level",         permission_level{ N(soule), config::active_name })
   );  
   
   // execute
   transaction_trace_ptr trace3;
   control->applied_transaction.connect([&]( const transaction_trace_ptr& t) { if (t->scheduled) { trace3 = t; } } );

   push_action( N(soula), N(exec), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "third")
                  ("executer",      "soula")
   );   
   
   BOOST_REQUIRE( bool(trace3) );   
   BOOST_REQUIRE_EQUAL( 1, trace3->action_traces.size() );
   BOOST_REQUIRE_EQUAL( transaction_receipt::executed, trace3->receipt->status );   
   
   auto proj_2_remvoed = get_created_project(project_2_index);
   BOOST_REQUIRE_EQUAL(true, proj_2_remvoed.is_null());

   auto proj_2_selected = get_selected_project(project_2_index);
   BOOST_REQUIRE_EQUAL(false, proj_2_selected.is_null());   
   BOOST_REQUIRE_EQUAL(proj_2_selected["owner"].as_string(), proj_2_owner);
   BOOST_REQUIRE_EQUAL(proj_2_selected["proj_name"].as_string(), proj_2_name);   
   BOOST_REQUIRE_EQUAL(proj_2_selected["selected_detail_index"].as_uint64(), 0);
   BOOST_REQUIRE_EQUAL(proj_2_selected["selected_resource_index"].as_uint64(), 0); 
      
   auto projinfo_2_selected = get_projectinfo(project_2_index, project_2_info_1_index);
   BOOST_REQUIRE_EQUAL(false, projinfo_2_selected.is_null());
   BOOST_REQUIRE_EQUAL(projinfo_2_selected["url"].as_string(), proj_2_url);
   BOOST_REQUIRE_EQUAL(projinfo_2_selected["hash"].as_string(), proj_2_hash);
   BOOST_REQUIRE_EQUAL(projinfo_2_selected["homepage"].as_string(), proj_2_homepage);
   BOOST_REQUIRE_EQUAL(projinfo_2_selected["icon"].as_string(), proj_2_icon);
   BOOST_REQUIRE_EQUAL(projinfo_2_selected["helper"].as_string(), proj_1_owner);  
   
   //////////////////
   // drop project //
   //////////////////
   
   // drop project_1
   vector<permission_level> action_perm4 = {{N(eb.factory), config::active_name}};  
   variant pretty_trx4 = fc::mutable_variant_object()
      ("expiration", "2021-01-01T00:30")
      ("ref_block_num", 2)
      ("ref_block_prefix", 3)
      ("max_net_usage_words", 0)
      ("max_cpu_usage_ms", 0)
      ("delay_sec", 0)
      ("actions", fc::variants({
            fc::mutable_variant_object()
               ("account", name("eb.factory"))
               ("name", "drop")
               ("authorization", action_perm4)
               ("data", fc::mutable_variant_object()
                ("project_index", project_1_index)
               )
               })
      );

   transaction trx4;
   abi_serializer::from_variant(pretty_trx4, trx4, get_resolver(), abi_serializer_max_time);
   
   // propose action
   push_action( N(soula), N(propose), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "drop")
                  ("trx",           trx4)
                  ("requested", perm)
   );   
   
   //approve
   push_action( N(soula), N(approve), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "drop")
                  ("level",         permission_level{ N(soula), config::active_name })
   ); 
   push_action( N(soulb), N(approve), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "drop")
                  ("level",         permission_level{ N(soulb), config::active_name })
   );   
   push_action( N(soulc), N(approve), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "drop")
                  ("level",         permission_level{ N(soulc), config::active_name })
   );   
   push_action( N(sould), N(approve), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "drop")
                  ("level",         permission_level{ N(sould), config::active_name })
   );  
   push_action( N(soule), N(approve), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "drop")
                  ("level",         permission_level{ N(soule), config::active_name })
   );  
   
   // execute
   transaction_trace_ptr trace4;
   control->applied_transaction.connect([&]( const transaction_trace_ptr& t) { if (t->scheduled) { trace4 = t; } } );

   push_action( N(soula), N(exec), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "drop")
                  ("executer",      "soula")
   );   
   
   BOOST_REQUIRE( bool(trace4) );   
   BOOST_REQUIRE_EQUAL( 1, trace4->action_traces.size() );
   BOOST_REQUIRE_EQUAL( transaction_receipt::executed, trace4->receipt->status );   
   
   auto proj_1_dropped = get_selected_project(project_1_index);
   BOOST_REQUIRE_EQUAL(true, proj_1_dropped.is_null());   
   
   auto projinfo_1_dropped = get_projectinfo(project_1_index, project_1_info_1_index);
   BOOST_REQUIRE_EQUAL(true, projinfo_1_dropped.is_null());
   auto projinfo_1_2_dropped = get_projectinfo(project_1_index, project_1_info_1_2_index);
   BOOST_REQUIRE_EQUAL(true, projinfo_1_2_dropped.is_null());   
   
   auto resource_1_dropped = get_resource(project_1_index, resource_1_index);
   BOOST_REQUIRE_EQUAL(true, resource_1_dropped.is_null());
   auto resource_1_2_dropped = get_resource(project_1_index, resource_1_2_index);
   BOOST_REQUIRE_EQUAL(true, resource_1_2_dropped.is_null());        
   auto resource_1_3_dropped = get_resource(project_1_index, resource_1_3_index);
   BOOST_REQUIRE_EQUAL(true, resource_1_3_dropped.is_null());     

   auto payment_1_dropped = get_payment(project_1_index, payment_1_index);
   BOOST_REQUIRE_EQUAL(true, payment_1_dropped.is_null());
   auto payment_1_2_dropped = get_payment(project_1_index, payment_1_2_index);
   BOOST_REQUIRE_EQUAL(true, payment_1_2_dropped.is_null());     
   auto payment_1_3_dropped = get_payment(project_1_index, payment_1_3_index);
   BOOST_REQUIRE_EQUAL(true, payment_1_3_dropped.is_null());     
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( ready_add_rm_cancel_drop, eb_factory_tester ) try {
   ///////////////////
   // pre-condition //
   ///////////////////
   
   // 1. set criterias
   // participant: 50 ~ 99
   // supporter: 100 ~ 199
   // representative_candidate: 200 ~ 500
   
   // 2. BP setting
   
   // 3. Join membership
   // alice, bob, carol
   setPreCondition("50.0000 BLACK", "99.0000 BLACK", 
                  "100.0000 BLACK", "199.0000 BLACK", 
                  "200.0000 BLACK", "500.0000 BLACK");   
                  
   ////////////////////
   // create project //
   ////////////////////   
   // create project_1
   BOOST_REQUIRE_EQUAL( success(), create_eb( proj_1_owner,
                                                proj_1_name, 
                                                proj_1_url, 
                                                proj_1_hash,
                                                proj_1_homepage,
                                                proj_1_icon,
                                                "") );
                                                
   produce_blocks( 10 );
   
   // create project_2
   BOOST_REQUIRE_EQUAL( success(), create_eb( proj_2_owner,
                                                proj_2_name, 
                                                proj_2_url, 
                                                proj_2_hash,
                                                proj_2_homepage,
                                                proj_2_icon,
                                                proj_1_helper) );                                            
   produce_blocks( 10 );
   
   // add project_1 info
   BOOST_REQUIRE_EQUAL( success(), addinfo_eb( proj_1_owner,
                                                project_1_index, 
                                                proj_1_2_url, 
                                                proj_1_2_hash,
                                                proj_1_2_homepage,
                                                proj_1_2_icon,
                                                proj_1_helper) );    
   produce_blocks( 10 );
   
   // add project_1 resource
   BOOST_REQUIRE_EQUAL( success(), addresource_eb( proj_1_owner,
                                                project_1_index, 
                                                ram_max_size_1, 
                                                black_max_count_1) );  
   BOOST_REQUIRE_EQUAL( success(), addresource_eb( proj_1_owner,
                                                project_1_index, 
                                                ram_max_size_2, 
                                                black_max_count_2) );     
   BOOST_REQUIRE_EQUAL( success(), addresource_eb( proj_1_owner,
                                                project_1_index, 
                                                ram_max_size_3, 
                                                black_max_count_3) );
   produce_blocks( 10 );
   
   // add project_1 payment
   BOOST_REQUIRE_EQUAL( success(), addpayment_eb( proj_1_owner,
                                                project_1_index, 
                                                aftertime_1, 
                                                percentage_1) );  
   BOOST_REQUIRE_EQUAL( success(), addpayment_eb( proj_1_owner,
                                                project_1_index, 
                                                aftertime_2, 
                                                percentage_2) );   
   BOOST_REQUIRE_EQUAL( success(), addpayment_eb( proj_1_owner,
                                                project_1_index, 
                                                aftertime_3, 
                                                percentage_3) );     
                                                
   BOOST_REQUIRE_EQUAL( success(), settoken_eb( proj_1_owner,
                                                project_1_index, 
                                                "eosio", 
                                                asset::from_string("1000000000000.000 TOKENA")) );                                                 
                                             
   auto resource_1 = get_token(project_1_index, 0);
   BOOST_REQUIRE_EQUAL(false, resource_1.is_null());
   BOOST_REQUIRE_EQUAL(resource_1["issuer"].as_string(), "eosio");
   BOOST_REQUIRE_EQUAL(resource_1["maximum_supply"].as<asset>(), asset::from_string("1000000000000.000 TOKENA"));    
                                                
                                                
   ////////////////////
   // select project //
   ////////////////////
   vector<permission_level> perm = { { N(soula), config::active_name }, { N(soulb), config::active_name },
                                       {N(soulc), config::active_name}, {N(sould), config::active_name},
                                       {N(soule), config::active_name}, {N(soulf), config::active_name}
   };    

   // select project_1
   vector<permission_level> action_perm = {{N(eb.factory), config::active_name}};   
   variant pretty_trx = fc::mutable_variant_object()
      ("expiration", "2021-01-01T00:30")
      ("ref_block_num", 2)
      ("ref_block_prefix", 3)
      ("max_net_usage_words", 0)
      ("max_cpu_usage_ms", 0)
      ("delay_sec", 0)
      ("actions", fc::variants({
            fc::mutable_variant_object()
               ("account", name("eb.factory"))
               ("name", "select")
               ("authorization", action_perm)
               ("data", fc::mutable_variant_object()
                ("project_index", project_1_index)
                ("detail_index", project_1_info_1_index)
                ("resource_index", resource_1_2_index)
                ("helper", proj_1_helper)
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
   
   auto proj_1_remvoed = get_created_project(project_1_index);
   BOOST_REQUIRE_EQUAL(true, proj_1_remvoed.is_null());

   auto proj_1_selected = get_selected_project(project_1_index);
   BOOST_REQUIRE_EQUAL(false, proj_1_selected.is_null());   
   BOOST_REQUIRE_EQUAL(proj_1_selected["owner"].as_string(), proj_1_owner);
   BOOST_REQUIRE_EQUAL(proj_1_selected["proj_name"].as_string(), proj_1_name);   
   BOOST_REQUIRE_EQUAL(proj_1_selected["selected_detail_index"].as_uint64(), project_1_info_1_index);
   BOOST_REQUIRE_EQUAL(proj_1_selected["selected_resource_index"].as_uint64(), resource_1_2_index); 
   
   auto projinfo_1_selected = get_projectinfo(project_1_index, project_1_info_1_2_index);
   BOOST_REQUIRE_EQUAL(false, projinfo_1_selected.is_null());
   BOOST_REQUIRE_EQUAL(projinfo_1_selected["url"].as_string(), proj_1_2_url);
   BOOST_REQUIRE_EQUAL(projinfo_1_selected["hash"].as_string(), proj_1_2_hash);
   BOOST_REQUIRE_EQUAL(projinfo_1_selected["homepage"].as_string(), proj_1_2_homepage);
   BOOST_REQUIRE_EQUAL(projinfo_1_selected["icon"].as_string(), proj_1_2_icon);
   BOOST_REQUIRE_EQUAL(projinfo_1_selected["helper"].as_string(), proj_1_owner);     
   
   
   
   
   ///////////////////
   // ready project //
   ///////////////////
   
   BOOST_REQUIRE_EQUAL( success(), setready_eb( proj_1_owner,
                                                   project_1_index, 
                                                   project_1_info_1_2_index, 
                                                   resource_1_3_index) );
                                                   
   auto proj_1_selected_removed = get_selected_project(project_1_index);
   BOOST_REQUIRE_EQUAL(true, proj_1_selected_removed.is_null());   
   
   auto proj_1_readied = get_readied_project(project_1_index);
   BOOST_REQUIRE_EQUAL(false, proj_1_readied.is_null());   
   BOOST_REQUIRE_EQUAL(proj_1_readied["owner"].as_string(), proj_1_owner);
   BOOST_REQUIRE_EQUAL(proj_1_readied["proj_name"].as_string(), proj_1_name);   
   BOOST_REQUIRE_EQUAL(proj_1_readied["selected_detail_index"].as_uint64(), project_1_info_1_index);
   BOOST_REQUIRE_EQUAL(proj_1_readied["selected_resource_index"].as_uint64(), resource_1_2_index);    
   BOOST_REQUIRE_EQUAL(proj_1_readied["started_detail_index"].as_uint64(), project_1_info_1_2_index);
   BOOST_REQUIRE_EQUAL(proj_1_readied["started_resource_index"].as_uint64(), resource_1_3_index);    
   
   ///////////////////////////////
   // add & remove project data //
   ///////////////////////////////
   
   // TODO BY SOULHAMMER
   // verify failure
   
   ///////////////////////////
   // cancel readed project //
   ///////////////////////////

   vector<permission_level> action_perm_3 = {{N(eb.factory), config::active_name}};   
   variant pretty_trx_3 = fc::mutable_variant_object()
      ("expiration", "2021-01-01T00:30")
      ("ref_block_num", 2)
      ("ref_block_prefix", 3)
      ("max_net_usage_words", 0)
      ("max_cpu_usage_ms", 0)
      ("delay_sec", 0)
      ("actions", fc::variants({
            fc::mutable_variant_object()
               ("account", name("eb.factory"))
               ("name", "cancelready")
               ("authorization", action_perm_3)
               ("data", fc::mutable_variant_object()
                ("project_index", project_1_index)
               )
               })
      );

   transaction trx_3;
   abi_serializer::from_variant(pretty_trx_3, trx_3, get_resolver(), abi_serializer_max_time);
   
   // propose action
   push_action( N(soula), N(propose), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "first")
                  ("trx",           trx_3)
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
   push_action( N(soule), N(approve), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "first")
                  ("level",         permission_level{ N(soule), config::active_name })
   );  
   
   // execute
   transaction_trace_ptr trace_3;
   control->applied_transaction.connect([&]( const transaction_trace_ptr& t) { if (t->scheduled) { trace_3 = t; } } );

   // approve
   push_action( N(soula), N(exec), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "first")
                  ("executer",      "soula")
   );   
   
   BOOST_REQUIRE( bool(trace_3) );
   BOOST_REQUIRE_EQUAL( 1, trace_3->action_traces.size() );
   BOOST_REQUIRE_EQUAL( transaction_receipt::executed, trace_3->receipt->status );                                                     
                                                   
   auto proj_1_readied_canceled = get_readied_project(project_1_index);
   BOOST_REQUIRE_EQUAL(true, proj_1_readied_canceled.is_null());     
   
   auto proj_1_selected_canceled = get_selected_project(project_1_index);
   BOOST_REQUIRE_EQUAL(false, proj_1_selected_canceled.is_null());   
   BOOST_REQUIRE_EQUAL(proj_1_selected_canceled["owner"].as_string(), proj_1_owner);
   BOOST_REQUIRE_EQUAL(proj_1_selected_canceled["proj_name"].as_string(), proj_1_name);   
   BOOST_REQUIRE_EQUAL(proj_1_selected_canceled["selected_detail_index"].as_uint64(), project_1_info_1_index);
   BOOST_REQUIRE_EQUAL(proj_1_selected_canceled["selected_resource_index"].as_uint64(), resource_1_2_index); 
   BOOST_REQUIRE_EQUAL(proj_1_selected_canceled["started_detail_index"].as_uint64(), project_1_info_1_2_index);
   BOOST_REQUIRE_EQUAL(proj_1_selected_canceled["started_resource_index"].as_uint64(), resource_1_3_index);  
   
   //////////////////
   // drop project //
   //////////////////
   
   vector<permission_level> action_perm_4 = {{N(eb.factory), config::active_name}};   
   variant pretty_trx_4 = fc::mutable_variant_object()
      ("expiration", "2021-01-01T00:30")
      ("ref_block_num", 2)
      ("ref_block_prefix", 3)
      ("max_net_usage_words", 0)
      ("max_cpu_usage_ms", 0)
      ("delay_sec", 0)
      ("actions", fc::variants({
            fc::mutable_variant_object()
               ("account", name("eb.factory"))
               ("name", "drop")
               ("authorization", action_perm_4)
               ("data", fc::mutable_variant_object()
                ("project_index", project_1_index)
               )
               })
      );

   transaction trx_4;
   abi_serializer::from_variant(pretty_trx_4, trx_4, get_resolver(), abi_serializer_max_time);
   
   // propose action
   push_action( N(soula), N(propose), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "drop")
                  ("trx",           trx_4)
                  ("requested", perm)
   );   
   
   // approve
   push_action( N(soula), N(approve), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "drop")
                  ("level",         permission_level{ N(soula), config::active_name })
   ); 
   push_action( N(soulb), N(approve), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "drop")
                  ("level",         permission_level{ N(soulb), config::active_name })
   );   
   push_action( N(soulc), N(approve), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "drop")
                  ("level",         permission_level{ N(soulc), config::active_name })
   );   
   push_action( N(sould), N(approve), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "drop")
                  ("level",         permission_level{ N(sould), config::active_name })
   ); 
   push_action( N(soule), N(approve), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "drop")
                  ("level",         permission_level{ N(soule), config::active_name })
   );  
   
   // execute
   transaction_trace_ptr trace_4;
   control->applied_transaction.connect([&]( const transaction_trace_ptr& t) { if (t->scheduled) { trace_4 = t; } } );

   // approve
   push_action( N(soula), N(exec), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "drop")
                  ("executer",      "soula")
   );   
   
   BOOST_REQUIRE( bool(trace_4) );
   BOOST_REQUIRE_EQUAL( 1, trace_4->action_traces.size() );
   BOOST_REQUIRE_EQUAL( transaction_receipt::executed, trace_4->receipt->status );    
   
   auto proj_1_dropped = get_selected_project(project_1_index);
   BOOST_REQUIRE_EQUAL(true, proj_1_dropped.is_null());   
   
   auto projinfo_1_dropped = get_projectinfo(project_1_index, project_1_info_1_index);
   BOOST_REQUIRE_EQUAL(true, projinfo_1_dropped.is_null());
   auto projinfo_1_2_dropped = get_projectinfo(project_1_index, project_1_info_1_2_index);
   BOOST_REQUIRE_EQUAL(true, projinfo_1_2_dropped.is_null());   
   
   auto resource_1_dropped = get_resource(project_1_index, resource_1_index);
   BOOST_REQUIRE_EQUAL(true, resource_1_dropped.is_null());
   auto resource_1_2_dropped = get_resource(project_1_index, resource_1_2_index);
   BOOST_REQUIRE_EQUAL(true, resource_1_2_dropped.is_null());        
   auto resource_1_3_dropped = get_resource(project_1_index, resource_1_3_index);
   BOOST_REQUIRE_EQUAL(true, resource_1_3_dropped.is_null());     

   auto payment_1_dropped = get_payment(project_1_index, payment_1_index);
   BOOST_REQUIRE_EQUAL(true, payment_1_dropped.is_null());
   auto payment_1_2_dropped = get_payment(project_1_index, payment_1_2_index);
   BOOST_REQUIRE_EQUAL(true, payment_1_2_dropped.is_null());     
   auto payment_1_3_dropped = get_payment(project_1_index, payment_1_3_index);
   BOOST_REQUIRE_EQUAL(true, payment_1_3_dropped.is_null());    
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( start_add_rm_drop, eb_factory_tester ) try {
   ///////////////////
   // pre-condition //
   ///////////////////
   
   // 1. set criterias
   // participant: 50 ~ 99
   // supporter: 100 ~ 199
   // representative_candidate: 200 ~ 500
   
   // 2. BP setting
   
   // 3. Join membership
   // alice, bob, carol
   setPreCondition("50.0000 BLACK", "99.0000 BLACK", 
                  "100.0000 BLACK", "199.0000 BLACK", 
                  "200.0000 BLACK", "500.0000 BLACK");   

   ////////////////////
   // create project //
   ////////////////////   
   // create project_1
   BOOST_REQUIRE_EQUAL( success(), create_eb( proj_1_owner,
                                                proj_1_name, 
                                                proj_1_url, 
                                                proj_1_hash,
                                                proj_1_homepage,
                                                proj_1_icon,
                                                "") );
   produce_blocks( 10 );
   
   // create project_2
   BOOST_REQUIRE_EQUAL( success(), create_eb( proj_2_owner,
                                                proj_2_name, 
                                                proj_2_url, 
                                                proj_2_hash,
                                                proj_2_homepage,
                                                proj_2_icon,
                                                proj_1_helper) );                                            
   produce_blocks( 10 );
   
   // add project_1 info
   BOOST_REQUIRE_EQUAL( success(), addinfo_eb( proj_1_owner,
                                                project_1_index, 
                                                proj_1_2_url, 
                                                proj_1_2_hash,
                                                proj_1_2_homepage,
                                                proj_1_2_icon,
                                                proj_1_helper) );    
   produce_blocks( 10 );
   
   // add project_1 resource
   BOOST_REQUIRE_EQUAL( success(), addresource_eb( proj_1_owner,
                                                project_1_index, 
                                                ram_max_size_1, 
                                                black_max_count_1) );  
   BOOST_REQUIRE_EQUAL( success(), addresource_eb( proj_1_owner,
                                                project_1_index, 
                                                ram_max_size_2, 
                                                black_max_count_2) );     
   BOOST_REQUIRE_EQUAL( success(), addresource_eb( proj_1_owner,
                                                project_1_index, 
                                                ram_max_size_3, 
                                                black_max_count_3) );
   produce_blocks( 10 );
   
   // add project_1 payment
   BOOST_REQUIRE_EQUAL( success(), addpayment_eb( proj_1_owner,
                                                project_1_index, 
                                                aftertime_1, 
                                                percentage_1) );  
   BOOST_REQUIRE_EQUAL( success(), addpayment_eb( proj_1_owner,
                                                project_1_index, 
                                                aftertime_2, 
                                                percentage_2) );   
   BOOST_REQUIRE_EQUAL( success(), addpayment_eb( proj_1_owner,
                                                project_1_index, 
                                                aftertime_3, 
                                                percentage_3) );     
                                                
   BOOST_REQUIRE_EQUAL( success(), settoken_eb( proj_1_owner,
                                                project_1_index, 
                                                "eosoulhammer", 
                                                asset::from_string("10000000000000.000 TOKENA")) );        
                                                
   auto resource_1 = get_token(project_1_index, 0);
   BOOST_REQUIRE_EQUAL(false, resource_1.is_null());
   BOOST_REQUIRE_EQUAL(resource_1["issuer"].as_string(), "eosoulhammer");
   BOOST_REQUIRE_EQUAL(resource_1["maximum_supply"].as<asset>(), asset::from_string("10000000000000.000 TOKENA"));                                                  

   ////////////////////
   // select project //
   ////////////////////
   vector<permission_level> perm = { { N(soula), config::active_name }, { N(soulb), config::active_name },
                                       {N(soulc), config::active_name}, {N(sould), config::active_name},
                                       {N(soule), config::active_name}, {N(soulf), config::active_name}
   }; 

   // select project_1
   vector<permission_level> action_perm = {{N(eb.factory), config::active_name}};   
   variant pretty_trx = fc::mutable_variant_object()
      ("expiration", "2021-01-01T00:30")
      ("ref_block_num", 2)
      ("ref_block_prefix", 3)
      ("max_net_usage_words", 0)
      ("max_cpu_usage_ms", 0)
      ("delay_sec", 0)
      ("actions", fc::variants({
            fc::mutable_variant_object()
               ("account", name("eb.factory"))
               ("name", "select")
               ("authorization", action_perm)
               ("data", fc::mutable_variant_object()
                ("project_index", project_1_index)
                ("detail_index", project_1_info_1_index)
                ("resource_index", resource_1_2_index)
                ("helper", proj_1_helper)
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
   push_action( N(soule), N(approve), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "first")
                  ("level",         permission_level{ N(soule), config::active_name })
   );  
   
   // execute
   transaction_trace_ptr trace;
   control->applied_transaction.connect([&]( const transaction_trace_ptr& t) { if (t->scheduled) { trace = t; } } );
   
   push_action( N(soula), N(exec), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "first")
                  ("executer",      "soula")
   );   
   
   BOOST_REQUIRE( bool(trace) );
   BOOST_REQUIRE_EQUAL( 1, trace->action_traces.size() );
   BOOST_REQUIRE_EQUAL( transaction_receipt::executed, trace->receipt->status );   
   
   // checking
   auto proj_1_remvoed = get_created_project(project_1_index);
   BOOST_REQUIRE_EQUAL(true, proj_1_remvoed.is_null());

   auto proj_1_selected = get_selected_project(project_1_index);
   BOOST_REQUIRE_EQUAL(false, proj_1_selected.is_null());   
   BOOST_REQUIRE_EQUAL(proj_1_selected["owner"].as_string(), proj_1_owner);
   BOOST_REQUIRE_EQUAL(proj_1_selected["proj_name"].as_string(), proj_1_name);   
   BOOST_REQUIRE_EQUAL(proj_1_selected["selected_detail_index"].as_uint64(), project_1_info_1_index);
   BOOST_REQUIRE_EQUAL(proj_1_selected["selected_resource_index"].as_uint64(), resource_1_2_index); 
   
   auto projinfo_1_selected = get_projectinfo(project_1_index, project_1_info_1_2_index);
   BOOST_REQUIRE_EQUAL(false, projinfo_1_selected.is_null());
   BOOST_REQUIRE_EQUAL(projinfo_1_selected["url"].as_string(), proj_1_2_url);
   BOOST_REQUIRE_EQUAL(projinfo_1_selected["hash"].as_string(), proj_1_2_hash);
   BOOST_REQUIRE_EQUAL(projinfo_1_selected["homepage"].as_string(), proj_1_2_homepage);
   BOOST_REQUIRE_EQUAL(projinfo_1_selected["icon"].as_string(), proj_1_2_icon);
   BOOST_REQUIRE_EQUAL(projinfo_1_selected["helper"].as_string(), proj_1_owner);     
   
   ///////////////////
   // ready project //
   ///////////////////
   
   BOOST_REQUIRE_EQUAL( success(), setready_eb( proj_1_owner,
                                                   project_1_index, 
                                                   project_1_info_1_2_index, 
                                                   resource_1_3_index) );
                                                   
   auto proj_1_selected_removed = get_selected_project(project_1_index);
   BOOST_REQUIRE_EQUAL(true, proj_1_selected_removed.is_null());   
   
   auto proj_1_readied = get_readied_project(project_1_index);
   BOOST_REQUIRE_EQUAL(false, proj_1_readied.is_null());   
   BOOST_REQUIRE_EQUAL(proj_1_readied["owner"].as_string(), proj_1_owner);
   BOOST_REQUIRE_EQUAL(proj_1_readied["proj_name"].as_string(), proj_1_name);   
   BOOST_REQUIRE_EQUAL(proj_1_readied["selected_detail_index"].as_uint64(), project_1_info_1_index);
   BOOST_REQUIRE_EQUAL(proj_1_readied["selected_resource_index"].as_uint64(), resource_1_2_index);    
   BOOST_REQUIRE_EQUAL(proj_1_readied["started_detail_index"].as_uint64(), project_1_info_1_2_index);
   BOOST_REQUIRE_EQUAL(proj_1_readied["started_resource_index"].as_uint64(), resource_1_3_index);
   
   ///////////////////
   // start project //
   ///////////////////   
   
   // start project_1
   vector<permission_level> action_perm_2 = {{N(eb.factory), config::active_name}};   
   variant pretty_trx_2 = fc::mutable_variant_object()
      ("expiration", "2021-01-01T00:30")
      ("ref_block_num", 2)
      ("ref_block_prefix", 3)
      ("max_net_usage_words", 0)
      ("max_cpu_usage_ms", 0)
      ("delay_sec", 0)
      ("actions", fc::variants({
            fc::mutable_variant_object()
               ("account", name("eb.factory"))
               ("name", "start")
               ("authorization", action_perm_2)
               ("data", fc::mutable_variant_object()
                ("project_index", project_1_index)
                ("detail_index", project_1_info_1_2_index)
                ("resource_index", resource_1_3_index)
                ("helper", proj_1_helper)
               )
               })
      );

   transaction trx_2;
   abi_serializer::from_variant(pretty_trx_2, trx_2, get_resolver(), abi_serializer_max_time);
   
   // propose action
   push_action( N(soula), N(propose), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "second")
                  ("trx",           trx_2)
                  ("requested", perm)
   );   
   
   // approve
   push_action( N(soula), N(approve), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "second")
                  ("level",         permission_level{ N(soula), config::active_name })
   ); 
   push_action( N(soulb), N(approve), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "second")
                  ("level",         permission_level{ N(soulb), config::active_name })
   );   
   push_action( N(soulc), N(approve), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "second")
                  ("level",         permission_level{ N(soulc), config::active_name })
   );   
   push_action( N(sould), N(approve), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "second")
                  ("level",         permission_level{ N(sould), config::active_name })
   ); 
   push_action( N(soule), N(approve), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "second")
                  ("level",         permission_level{ N(soule), config::active_name })
   );  
   
   // execute
   transaction_trace_ptr trace_2;
   control->applied_transaction.connect([&]( const transaction_trace_ptr& t) { if (t->scheduled) { trace_2 = t; } } );

   push_action( N(soula), N(exec), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "second")
                  ("executer",      "soula")
   );   
   
   BOOST_REQUIRE( bool(trace_2) );
   BOOST_REQUIRE_EQUAL( 1, trace_2->action_traces.size() );
   BOOST_REQUIRE_EQUAL( transaction_receipt::executed, trace_2->receipt->status );     
   
   // checking
   proj_1_readied = get_readied_project(project_1_index);
   BOOST_REQUIRE_EQUAL(true, proj_1_readied.is_null());     
   
   auto proj_1_started = get_started_project(project_1_index);
   BOOST_REQUIRE_EQUAL(false, proj_1_started.is_null());    
   BOOST_REQUIRE_EQUAL(proj_1_started["owner"].as_string(), proj_1_owner);
   BOOST_REQUIRE_EQUAL(proj_1_started["proj_name"].as_string(), proj_1_name);   
   BOOST_REQUIRE_EQUAL(proj_1_started["selected_detail_index"].as_uint64(), project_1_info_1_index);
   BOOST_REQUIRE_EQUAL(proj_1_started["selected_resource_index"].as_uint64(), resource_1_2_index);    
   BOOST_REQUIRE_EQUAL(proj_1_started["started_detail_index"].as_uint64(), project_1_info_1_2_index);
   BOOST_REQUIRE_EQUAL(proj_1_started["started_resource_index"].as_uint64(), resource_1_3_index);   
   
   ///////////////////////////////
   // add & remove project data //
   ///////////////////////////////
   
   // TODO BY SOULHAMMER
   // verify failure
   
   //////////////////
   // drop project //
   //////////////////
   
   // this testing is to be failed.
   // when project state is started, drop is impossible
   
   vector<permission_level> action_perm_3 = {{N(eb.factory), config::active_name}};   
   variant pretty_trx_3 = fc::mutable_variant_object()
      ("expiration", "2021-01-01T00:30")
      ("ref_block_num", 2)
      ("ref_block_prefix", 3)
      ("max_net_usage_words", 0)
      ("max_cpu_usage_ms", 0)
      ("delay_sec", 0)
      ("actions", fc::variants({
            fc::mutable_variant_object()
               ("account", name("eb.factory"))
               ("name", "drop")
               ("authorization", action_perm_3)
               ("data", fc::mutable_variant_object()
                ("project_index", project_1_index)
               )
               })
      );

   transaction trx_3;
   abi_serializer::from_variant(pretty_trx_3, trx_3, get_resolver(), abi_serializer_max_time);
   
   // propose action
   push_action( N(soula), N(propose), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "second")
                  ("trx",           trx_3)
                  ("requested", perm)
   );   
   
   // approve
   push_action( N(soula), N(approve), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "second")
                  ("level",         permission_level{ N(soula), config::active_name })
   ); 
   push_action( N(soulb), N(approve), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "second")
                  ("level",         permission_level{ N(soulb), config::active_name })
   );   
   push_action( N(soulc), N(approve), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "second")
                  ("level",         permission_level{ N(soulc), config::active_name })
   );   
   push_action( N(sould), N(approve), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "second")
                  ("level",         permission_level{ N(sould), config::active_name })
   ); 
   push_action( N(soule), N(approve), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "second")
                  ("level",         permission_level{ N(soule), config::active_name })
   );  
   
   // execute
   transaction_trace_ptr trace_3;
   control->applied_transaction.connect([&]( const transaction_trace_ptr& t) { if (t->scheduled) { trace_3 = t; } } );

   push_action( N(soula), N(exec), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "second")
                  ("executer",      "soula")
   );   
   
   BOOST_REQUIRE_EQUAL( false, bool(trace_3) );
} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( start_create_token, eb_factory_tester ) try {
   ///////////////////
   // pre-condition //
   ///////////////////
   
   // 1. set criterias
   // participant: 50 ~ 99
   // supporter: 100 ~ 199
   // representative_candidate: 200 ~ 500
   
   // 2. BP setting
   
   // 3. Join membership
   // alice, bob, carol
   setPreCondition("50.0000 BLACK", "99.0000 BLACK", 
                  "100.0000 BLACK", "199.0000 BLACK", 
                  "200.0000 BLACK", "500.0000 BLACK");   

   ////////////////////
   // create project //
   ////////////////////   
   // create project_1
   BOOST_REQUIRE_EQUAL( success(), create_eb( proj_1_owner,
                                                proj_1_name, 
                                                proj_1_url, 
                                                proj_1_hash,
                                                proj_1_homepage,
                                                proj_1_icon,
                                                "") );
   produce_blocks( 10 );
   
   // create project_2
   BOOST_REQUIRE_EQUAL( success(), create_eb( proj_2_owner,
                                                proj_2_name, 
                                                proj_2_url, 
                                                proj_2_hash,
                                                proj_2_homepage,
                                                proj_2_icon,
                                                proj_1_helper) );                                            
   produce_blocks( 10 );
   
   // add project_1 info
   BOOST_REQUIRE_EQUAL( success(), addinfo_eb( proj_1_owner,
                                                project_1_index, 
                                                proj_1_2_url, 
                                                proj_1_2_hash,
                                                proj_1_2_homepage,
                                                proj_1_2_icon,
                                                proj_1_helper) );    
   produce_blocks( 10 );
   
   // add project_1 resource
   BOOST_REQUIRE_EQUAL( success(), addresource_eb( proj_1_owner,
                                                project_1_index, 
                                                ram_max_size_1, 
                                                black_max_count_1) );  
   BOOST_REQUIRE_EQUAL( success(), addresource_eb( proj_1_owner,
                                                project_1_index, 
                                                ram_max_size_2, 
                                                black_max_count_2) );     
   BOOST_REQUIRE_EQUAL( success(), addresource_eb( proj_1_owner,
                                                project_1_index, 
                                                ram_max_size_3, 
                                                black_max_count_3) );
   produce_blocks( 10 );
   
   // add project_1 payment
   BOOST_REQUIRE_EQUAL( success(), addpayment_eb( proj_1_owner,
                                                project_1_index, 
                                                aftertime_1, 
                                                percentage_1) );  
   BOOST_REQUIRE_EQUAL( success(), addpayment_eb( proj_1_owner,
                                                project_1_index, 
                                                aftertime_2, 
                                                percentage_2) );   
   BOOST_REQUIRE_EQUAL( success(), addpayment_eb( proj_1_owner,
                                                project_1_index, 
                                                aftertime_3, 
                                                percentage_3) );     
                                                
   BOOST_REQUIRE_EQUAL( success(), settoken_eb( proj_1_owner,
                                                project_1_index, 
                                                "eosio", 
                                                asset::from_string("1000000000000.000 TOKENA")) );     
                                                
   BOOST_REQUIRE_EQUAL( success(), settoken_eb( proj_1_owner,
                                                project_1_index, 
                                                "eosoulhammer", 
                                                asset::from_string("10000000000000.000 TOKENB")) );                                                    
                                                
   auto resource_1 = get_token(project_1_index, 0);
   BOOST_REQUIRE_EQUAL(false, resource_1.is_null());
   BOOST_REQUIRE_EQUAL(resource_1["issuer"].as_string(), "eosoulhammer");
   BOOST_REQUIRE_EQUAL(resource_1["maximum_supply"].as<asset>(), asset::from_string("10000000000000.000 TOKENB"));                                                  

   ////////////////////
   // select project //
   ////////////////////
   vector<permission_level> perm = { { N(soula), config::active_name }, { N(soulb), config::active_name },
                                       {N(soulc), config::active_name}, {N(sould), config::active_name},
                                       {N(soule), config::active_name}, {N(soulf), config::active_name}
   }; 

   // select project_1
   vector<permission_level> action_perm = {{N(eb.factory), config::active_name}};   
   variant pretty_trx = fc::mutable_variant_object()
      ("expiration", "2021-01-01T00:30")
      ("ref_block_num", 2)
      ("ref_block_prefix", 3)
      ("max_net_usage_words", 0)
      ("max_cpu_usage_ms", 0)
      ("delay_sec", 0)
      ("actions", fc::variants({
            fc::mutable_variant_object()
               ("account", name("eb.factory"))
               ("name", "select")
               ("authorization", action_perm)
               ("data", fc::mutable_variant_object()
                ("project_index", project_1_index)
                ("detail_index", project_1_info_1_index)
                ("resource_index", resource_1_2_index)
                ("helper", proj_1_helper)
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
   push_action( N(soule), N(approve), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "first")
                  ("level",         permission_level{ N(soule), config::active_name })
   );  
   
   // execute
   transaction_trace_ptr trace;
   control->applied_transaction.connect([&]( const transaction_trace_ptr& t) { if (t->scheduled) { trace = t; } } );
   
   push_action( N(soula), N(exec), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "first")
                  ("executer",      "soula")
   );   
   
   BOOST_REQUIRE( bool(trace) );
   BOOST_REQUIRE_EQUAL( 1, trace->action_traces.size() );
   BOOST_REQUIRE_EQUAL( transaction_receipt::executed, trace->receipt->status );   
   
   // checking
   auto proj_1_remvoed = get_created_project(project_1_index);
   BOOST_REQUIRE_EQUAL(true, proj_1_remvoed.is_null());

   auto proj_1_selected = get_selected_project(project_1_index);
   BOOST_REQUIRE_EQUAL(false, proj_1_selected.is_null());   
   BOOST_REQUIRE_EQUAL(proj_1_selected["owner"].as_string(), proj_1_owner);
   BOOST_REQUIRE_EQUAL(proj_1_selected["proj_name"].as_string(), proj_1_name);   
   BOOST_REQUIRE_EQUAL(proj_1_selected["selected_detail_index"].as_uint64(), project_1_info_1_index);
   BOOST_REQUIRE_EQUAL(proj_1_selected["selected_resource_index"].as_uint64(), resource_1_2_index); 
   
   auto projinfo_1_selected = get_projectinfo(project_1_index, project_1_info_1_2_index);
   BOOST_REQUIRE_EQUAL(false, projinfo_1_selected.is_null());
   BOOST_REQUIRE_EQUAL(projinfo_1_selected["url"].as_string(), proj_1_2_url);
   BOOST_REQUIRE_EQUAL(projinfo_1_selected["hash"].as_string(), proj_1_2_hash);
   BOOST_REQUIRE_EQUAL(projinfo_1_selected["homepage"].as_string(), proj_1_2_homepage);
   BOOST_REQUIRE_EQUAL(projinfo_1_selected["icon"].as_string(), proj_1_2_icon);
   BOOST_REQUIRE_EQUAL(projinfo_1_selected["helper"].as_string(), proj_1_owner);     
   
   ///////////////////
   // ready project //
   ///////////////////
   
   BOOST_REQUIRE_EQUAL( success(), setready_eb( proj_1_owner,
                                                   project_1_index, 
                                                   project_1_info_1_2_index, 
                                                   resource_1_3_index) );
                                                   
   auto proj_1_selected_removed = get_selected_project(project_1_index);
   BOOST_REQUIRE_EQUAL(true, proj_1_selected_removed.is_null());   
   
   auto proj_1_readied = get_readied_project(project_1_index);
   BOOST_REQUIRE_EQUAL(false, proj_1_readied.is_null());   
   BOOST_REQUIRE_EQUAL(proj_1_readied["owner"].as_string(), proj_1_owner);
   BOOST_REQUIRE_EQUAL(proj_1_readied["proj_name"].as_string(), proj_1_name);   
   BOOST_REQUIRE_EQUAL(proj_1_readied["selected_detail_index"].as_uint64(), project_1_info_1_index);
   BOOST_REQUIRE_EQUAL(proj_1_readied["selected_resource_index"].as_uint64(), resource_1_2_index);    
   BOOST_REQUIRE_EQUAL(proj_1_readied["started_detail_index"].as_uint64(), project_1_info_1_2_index);
   BOOST_REQUIRE_EQUAL(proj_1_readied["started_resource_index"].as_uint64(), resource_1_3_index);
   
   ///////////////////
   // start project //
   ///////////////////   
   
   // start project_1
   vector<permission_level> action_perm_2 = {{N(eb.factory), config::active_name}};   
   variant pretty_trx_2 = fc::mutable_variant_object()
      ("expiration", "2021-01-01T00:30")
      ("ref_block_num", 2)
      ("ref_block_prefix", 3)
      ("max_net_usage_words", 0)
      ("max_cpu_usage_ms", 0)
      ("delay_sec", 0)
      ("actions", fc::variants({
            fc::mutable_variant_object()
               ("account", name("eb.factory"))
               ("name", "start")
               ("authorization", action_perm_2)
               ("data", fc::mutable_variant_object()
                ("project_index", project_1_index)
                ("detail_index", project_1_info_1_2_index)
                ("resource_index", resource_1_3_index)
                ("helper", proj_1_helper)
               )
               })
      );

   transaction trx_2;
   abi_serializer::from_variant(pretty_trx_2, trx_2, get_resolver(), abi_serializer_max_time);
   
   // propose action
   push_action( N(soula), N(propose), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "second")
                  ("trx",           trx_2)
                  ("requested", perm)
   );   
   
   // approve
   push_action( N(soula), N(approve), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "second")
                  ("level",         permission_level{ N(soula), config::active_name })
   ); 
   push_action( N(soulb), N(approve), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "second")
                  ("level",         permission_level{ N(soulb), config::active_name })
   );   
   push_action( N(soulc), N(approve), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "second")
                  ("level",         permission_level{ N(soulc), config::active_name })
   );   
   push_action( N(sould), N(approve), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "second")
                  ("level",         permission_level{ N(sould), config::active_name })
   ); 
   push_action( N(soule), N(approve), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "second")
                  ("level",         permission_level{ N(soule), config::active_name })
   );  
   
   // execute
   transaction_trace_ptr trace_2;
   control->applied_transaction.connect([&]( const transaction_trace_ptr& t) { if (t->scheduled) { trace_2 = t; } } );

   push_action( N(soula), N(exec), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "second")
                  ("executer",      "soula")
   );   
   
   BOOST_REQUIRE( bool(trace_2) );
   BOOST_REQUIRE_EQUAL( 1, trace_2->action_traces.size() );
   BOOST_REQUIRE_EQUAL( transaction_receipt::executed, trace_2->receipt->status );     
   
   // checking
   proj_1_readied = get_readied_project(project_1_index);
   BOOST_REQUIRE_EQUAL(true, proj_1_readied.is_null());     
   
   auto proj_1_started = get_started_project(project_1_index);
   BOOST_REQUIRE_EQUAL(false, proj_1_started.is_null());    
   BOOST_REQUIRE_EQUAL(proj_1_started["owner"].as_string(), proj_1_owner);
   BOOST_REQUIRE_EQUAL(proj_1_started["proj_name"].as_string(), proj_1_name);   
   BOOST_REQUIRE_EQUAL(proj_1_started["selected_detail_index"].as_uint64(), project_1_info_1_index);
   BOOST_REQUIRE_EQUAL(proj_1_started["selected_resource_index"].as_uint64(), resource_1_2_index);    
   BOOST_REQUIRE_EQUAL(proj_1_started["started_detail_index"].as_uint64(), project_1_info_1_2_index);
   BOOST_REQUIRE_EQUAL(proj_1_started["started_resource_index"].as_uint64(), resource_1_3_index);   
   
   ///////////////////////////////
   // add & remove project data //
   ///////////////////////////////
   
   // TODO BY SOULHAMMER
   // verify failure
   
   //////////////////
   // drop project //
   //////////////////
   
   // this testing is to be failed.
   // when project state is started, drop is impossible
   
   vector<permission_level> action_perm_3 = {{N(eb.factory), config::active_name}};   
   variant pretty_trx_3 = fc::mutable_variant_object()
      ("expiration", "2021-01-01T00:30")
      ("ref_block_num", 2)
      ("ref_block_prefix", 3)
      ("max_net_usage_words", 0)
      ("max_cpu_usage_ms", 0)
      ("delay_sec", 0)
      ("actions", fc::variants({
            fc::mutable_variant_object()
               ("account", name("eb.factory"))
               ("name", "drop")
               ("authorization", action_perm_3)
               ("data", fc::mutable_variant_object()
                ("project_index", project_1_index)
               )
               })
      );

   transaction trx_3;
   abi_serializer::from_variant(pretty_trx_3, trx_3, get_resolver(), abi_serializer_max_time);
   
   // propose action
   push_action( N(soula), N(propose), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "second")
                  ("trx",           trx_3)
                  ("requested", perm)
   );   
   
   // approve
   push_action( N(soula), N(approve), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "second")
                  ("level",         permission_level{ N(soula), config::active_name })
   ); 
   push_action( N(soulb), N(approve), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "second")
                  ("level",         permission_level{ N(soulb), config::active_name })
   );   
   push_action( N(soulc), N(approve), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "second")
                  ("level",         permission_level{ N(soulc), config::active_name })
   );   
   push_action( N(sould), N(approve), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "second")
                  ("level",         permission_level{ N(sould), config::active_name })
   ); 
   push_action( N(soule), N(approve), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "second")
                  ("level",         permission_level{ N(soule), config::active_name })
   );  
   
   // execute
   transaction_trace_ptr trace_3;
   control->applied_transaction.connect([&]( const transaction_trace_ptr& t) { if (t->scheduled) { trace_3 = t; } } );

   push_action( N(soula), N(exec), mvo()
                  ("proposer",      "soula")
                  ("proposal_name", "second")
                  ("executer",      "soula")
   );   
   
   BOOST_REQUIRE_EQUAL( false, bool(trace_3) );
   
   
   auto stats = get_stats("3,TOKENB");
   REQUIRE_MATCHING_OBJECT( stats, mvo()
      ("supply", "0.000 TOKENB")
      ("max_supply", "10000000000000.000 TOKENB")
      ("issuer", "eosoulhammer")
   );    
   
   
} FC_LOG_AND_RETHROW()




BOOST_AUTO_TEST_SUITE_END()