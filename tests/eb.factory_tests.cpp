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

class eb_factory_tester : public eosio_system_tester {
public:
   eb_factory_tester() {
      produce_blocks( 2 );

      create_account_with_resources( N(eb.factory), config::system_account_name, core_sym::from_string("20.0000"), false );

      produce_blocks( 100 );      
      
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
   }
   
   action_result push_action_eb( const account_name& signer, const action_name &name, const variant_object &data, bool auth = true ) {
         string action_type_name = factory_abi_ser.get_action_type(name);

         action act;
         act.account = N(eb.factory);
         act.name = name;
         act.data = factory_abi_ser.variant_to_binary( action_type_name, data, abi_serializer_max_time );

         return base_tester::push_action( std::move(act), auth ? uint64_t(signer) : signer == N(bob111111111) ? N(alice1111111) : N(bob111111111) );
   }

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
   
   fc::variant get_created_project(const uint64_t& index)
   {
      vector<char> data = get_row_by_account( N(eb.factory), N(created), N(projects), index );  
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
   
   abi_serializer factory_abi_ser;
};

BOOST_AUTO_TEST_SUITE(eb_factory_tests)

BOOST_FIXTURE_TEST_CASE( create_drop, eb_factory_tester ) try {
   cross_15_percent_threshold();

   produce_blocks( 10 );
   produce_block( fc::hours(3*24) );

   BOOST_REQUIRE_EQUAL( core_sym::from_string("0.0000"), get_balance( "alice1111111" ) );
   transfer( "eosio", "alice1111111", core_sym::from_string("1000.0000"), "eosio" );
   
   string proj_1_owner = "alice1111111";
   string proj_1_name = "test_1_project";
   string proj_1_url = "www.test_1.io";
   string proj_1_hash = "test_1_hash";
   string proj_1_homepage = "www.homepage_1.com";
   string proj_1_icon = "www.icon_1.io";
   string proj_1_helper = "alice1111111";
   
   string proj_1_url_update = "www.test_1_2.io";
   string proj_1_hash_update = "test_1_2_hash";
   string proj_1_homepage_update = "www.homepage_1_2.com";
   string proj_1_icon_update = "www.icon_1_2.io";
   
   string proj_2_owner = "bob111111111";
   string proj_2_name = "test_2_project";
   string proj_2_url = "www.test_2.io";
   string proj_2_hash = "test_2_hash";
   string proj_2_homepage = "www.homepage_2.com";
   string proj_2_icon = "www.icon_2.io";
   string proj_2_helper = "bob111111111";   
   
   string proj_3_owner = "carol1111111";
   string proj_3_name = "test_3_project";
   string proj_3_url = "www.test_3.io";
   string proj_3_hash = "test_3_hash";
   string proj_3_homepage = "www.homepage_3.com";
   string proj_3_icon = "www.icon_3.io";
   string proj_3_helper = "carol1111111";      
   
   ///////////////////////////////////                   
   // create & add & remove & check //
   ///////////////////////////////////                   
   BOOST_REQUIRE_EQUAL( success(), create_eb( proj_1_owner,
                                                proj_1_name, 
                                                proj_1_url, 
                                                proj_1_hash,
                                                proj_1_homepage,
                                                proj_1_icon,
                                                proj_1_helper) );
                                                
   produce_blocks( 10 );
   BOOST_REQUIRE_EQUAL( success(), create_eb( proj_2_owner,
                                                proj_2_name, 
                                                proj_2_url, 
                                                proj_2_hash,
                                                proj_2_homepage,
                                                proj_2_icon,
                                                proj_2_helper) );                                            

   produce_blocks( 10 );
   BOOST_REQUIRE_EQUAL( success(), create_eb( proj_3_owner,
                                                proj_3_name, 
                                                proj_3_url, 
                                                proj_3_hash,
                                                proj_3_homepage,
                                                proj_3_icon,
                                                proj_3_helper) ); 

   produce_blocks( 10 );
   BOOST_REQUIRE_EQUAL( wasm_assert_msg("already same project name exist"), 
                        create_eb( proj_3_owner,
                                    proj_1_name, 
                                    proj_1_url, 
                                    proj_1_hash,
                                    proj_1_homepage,
                                    proj_1_icon,
                                    proj_1_helper) );                                                 

   uint64_t project_1_index = 0;                                      
   uint64_t project_2_index = 1;
   uint64_t project_3_index = 2;
   uint64_t project_4_index = 3;
   
   uint64_t projectinfo_1_index = 0;
   uint64_t projectinfo_1_2_index = 1;
   uint64_t projectinfo_2_index = 0;
   uint64_t projectinfo_3_index = 0;
   
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
   
   // add info
   produce_blocks( 10 );
   BOOST_REQUIRE_EQUAL( success(), addinfo_eb( proj_1_owner,
                                                project_1_index, 
                                                proj_1_url_update, 
                                                proj_1_hash_update,
                                                proj_1_homepage_update,
                                                proj_1_icon_update,
                                                proj_1_helper) );    
    
   // add resource
   produce_blocks( 10 );
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
                                                
   // add payment    
   produce_blocks( 10 );
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

   auto proj_1 = get_created_project(project_1_index);
   BOOST_REQUIRE_EQUAL(false, proj_1.is_null());
   BOOST_REQUIRE_EQUAL(proj_1["owner"].as_string(), proj_1_owner);
   BOOST_REQUIRE_EQUAL(proj_1["proj_name"].as_string(), "test_1_project");
   
   auto projinfo_1 = get_projectinfo(project_1_index, projectinfo_1_index);
   BOOST_REQUIRE_EQUAL(false, projinfo_1.is_null());
   BOOST_REQUIRE_EQUAL(projinfo_1["url"].as_string(), proj_1_url);
   BOOST_REQUIRE_EQUAL(projinfo_1["hash"].as_string(), proj_1_hash);
   BOOST_REQUIRE_EQUAL(projinfo_1["homepage"].as_string(), proj_1_homepage);
   BOOST_REQUIRE_EQUAL(projinfo_1["icon"].as_string(), proj_1_icon);
   BOOST_REQUIRE_EQUAL(projinfo_1["helper"].as_string(), proj_1_helper);
   
   auto projinfo_1_2 = get_projectinfo(project_1_index, projectinfo_1_2_index);
   BOOST_REQUIRE_EQUAL(false, projinfo_1_2.is_null());
   BOOST_REQUIRE_EQUAL(projinfo_1_2["url"].as_string(), proj_1_url_update);
   BOOST_REQUIRE_EQUAL(projinfo_1_2["hash"].as_string(), proj_1_hash_update);
   BOOST_REQUIRE_EQUAL(projinfo_1_2["homepage"].as_string(), proj_1_homepage_update);
   BOOST_REQUIRE_EQUAL(projinfo_1_2["icon"].as_string(), proj_1_icon_update);
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
   

   auto proj_2 = get_created_project(project_2_index);
   BOOST_REQUIRE_EQUAL(false, proj_2.is_null());
   BOOST_REQUIRE_EQUAL(proj_2["owner"].as_string(), proj_2_owner);
   BOOST_REQUIRE_EQUAL(proj_2["proj_name"].as_string(), proj_2_name);   
   
   auto projinfo_2 = get_projectinfo(project_2_index, projectinfo_2_index);
   BOOST_REQUIRE_EQUAL(false, projinfo_2.is_null());
   BOOST_REQUIRE_EQUAL(projinfo_2["url"].as_string(), proj_2_url);
   BOOST_REQUIRE_EQUAL(projinfo_2["hash"].as_string(), proj_2_hash);
   BOOST_REQUIRE_EQUAL(projinfo_2["homepage"].as_string(), proj_2_homepage);
   BOOST_REQUIRE_EQUAL(projinfo_2["icon"].as_string(), proj_2_icon);
   BOOST_REQUIRE_EQUAL(projinfo_2["helper"].as_string(), proj_2_helper);   
   
   auto proj_3 = get_created_project(project_3_index);
   BOOST_REQUIRE_EQUAL(false, proj_3.is_null());
   BOOST_REQUIRE_EQUAL(proj_3["owner"].as_string(), proj_3_owner);
   BOOST_REQUIRE_EQUAL(proj_3["proj_name"].as_string(), proj_3_name);   
   
   auto projinfo_3 = get_projectinfo(project_3_index, projectinfo_3_index);
   BOOST_REQUIRE_EQUAL(false, projinfo_3.is_null());
   BOOST_REQUIRE_EQUAL(projinfo_3["url"].as_string(), proj_3_url);
   BOOST_REQUIRE_EQUAL(projinfo_3["hash"].as_string(), proj_3_hash);
   BOOST_REQUIRE_EQUAL(projinfo_3["homepage"].as_string(), proj_3_homepage);
   BOOST_REQUIRE_EQUAL(projinfo_3["icon"].as_string(), proj_3_icon);
   BOOST_REQUIRE_EQUAL(projinfo_3["helper"].as_string(), proj_3_helper);   
   
   ////////////////////
   // drop is failed //
   ////////////////////
   BOOST_REQUIRE_EQUAL("missing authority of alice1111111", 
                        drop_eb(name(proj_2_owner), project_1_index, "created"));   
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("project doesn't exist"), 
                        drop_eb(name(proj_1_owner), project_4_index, "created"));   
                    
   //////////////////                   
   // drop & check //
   //////////////////
   BOOST_REQUIRE_EQUAL(success(), 
                        drop_eb(name(proj_1_owner), project_1_index, "created"));   
   
   auto proj_1_dropped = get_created_project(project_1_index);
   BOOST_REQUIRE_EQUAL(true, proj_1_dropped.is_null());   
   
   auto projinfo_1_dropped = get_projectinfo(project_1_index, projectinfo_1_index);
   BOOST_REQUIRE_EQUAL(true, projinfo_1_dropped.is_null());
   auto projinfo_1_2_dropped = get_projectinfo(project_1_index, projectinfo_1_2_index);
   BOOST_REQUIRE_EQUAL(true, projinfo_1_2_dropped.is_null());   
   
   auto resource_1_dropped = get_resource(project_1_index, resource_1_index);
   BOOST_REQUIRE_EQUAL(true, resource_1_dropped.is_null());
   auto resource_1_3_dropped = get_resource(project_1_index, resource_1_3_index);
   BOOST_REQUIRE_EQUAL(true, resource_1_3_dropped.is_null());     

   auto payment_1_dropped = get_payment(project_1_index, payment_1_index);
   BOOST_REQUIRE_EQUAL(true, payment_1_dropped.is_null());
   auto payment_1_3_dropped = get_payment(project_1_index, payment_1_3_index);
   BOOST_REQUIRE_EQUAL(true, payment_1_3_dropped.is_null());     
   
   ////////////////////                   
   // create & check //
   ////////////////////     
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
   BOOST_REQUIRE_EQUAL(proj_1_recreated["proj_name"].as_string(), "test_1_project");                                                
   
   /*
   std::cout << "proj_1_name_hash: " << proj_1["proj_name_hash"].as_string() << std::endl;
   std::cout << "proj_2_name_hash: " << proj_2["proj_name_hash"].as_string() << std::endl;
   std::cout << "proj_3_name_hash: " << proj_3["proj_name_hash"].as_string() << std::endl;
   
   std::cout << "proj_1_updatetime: " << projinfo_1["updatetime"].as_string() << std::endl;
   std::cout << "proj_2_updatetime: " << projinfo_2["updatetime"].as_string() << std::endl;   
   std::cout << "proj_3_updatetime: " << projinfo_3["updatetime"].as_string() << std::endl; 
   */
} FC_LOG_AND_RETHROW()


BOOST_AUTO_TEST_SUITE_END()

