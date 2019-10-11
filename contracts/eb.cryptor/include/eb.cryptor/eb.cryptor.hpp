/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <eosio.system/eosio.system.hpp>
#include <eosio.token/eosio.token.hpp>
#include <eb.cryptob/eb.cryptob.hpp>

#include <string>

namespace eosiosystem {
   class system_contract;
}

namespace eosio {

   using std::string;
   
   struct [[eosio::table,eosio::contract("eb.cryptor")]] rewardtoken {
      uint64_t index;
      bool is_rewarded;

      uint64_t primary_key() const {return index;}
      
      EOSLIB_SERIALIZE( rewardtoken, (index)(is_rewarded) )
   };  
   
   struct [[eosio::table,eosio::contract("eb.cryptor")]] rewardram {
      uint64_t index;
      bool is_rewarded;

      uint64_t primary_key() const {return index;}
      
      EOSLIB_SERIALIZE( rewardram, (index)(is_rewarded) )
   };      
   
   struct [[eosio::table,eosio::contract("eb.factory")]] project {
      uint64_t index;
      name owner;
      string proj_name;
      checksum256 proj_name_hash;
      uint64_t selected_detail_index;
      uint64_t selected_resource_index;
      uint64_t started_detail_index;
      uint64_t started_resource_index;      
      
      uint64_t primary_key() const {return index;}
      checksum256 by_proj_name_hash() const {return proj_name_hash;}
      
      EOSLIB_SERIALIZE( project, (index)(owner)(proj_name)(proj_name_hash)(selected_detail_index)(selected_resource_index)(started_detail_index)(started_resource_index) )
   };   
   
   struct [[eosio::table,eosio::contract("eb.factory")]] payment {
      uint64_t index;
      uint64_t aftertime;
      int8_t percentage;

      uint64_t primary_key() const {return index;}
      
      EOSLIB_SERIALIZE( payment, (index)(aftertime)(percentage) )
   };     
   
   struct [[eosio::table,eosio::contract("eb.factory")]] resource {
      uint64_t index;
      uint64_t ram_max_size;
      asset black_max_count;

      uint64_t primary_key() const {return index;}
      
      EOSLIB_SERIALIZE( resource, (index)(ram_max_size)(black_max_count) )
   };   
   
   typedef eosio::multi_index< "rewardtokens"_n, rewardtoken > rewardtoken_table;
   typedef eosio::multi_index< "rewardrams"_n, rewardram > rewardram_table;
   
   typedef eosio::multi_index< "payments"_n, payment > payment_table;
   typedef eosio::multi_index< "resources"_n, resource > resource_table;
   typedef eosio::multi_index< "projects"_n, project, indexed_by<name("projnamehash"), const_mem_fun<project, checksum256, &project::by_proj_name_hash>> > project_table;

   class [[eosio::contract("eb.cryptor")]] cryptor : public contract {
      public:
         using contract::contract;

         [[eosio::action]]
         void gettoken( uint64_t project_index,
                      name receiver);
         
         [[eosio::action]]
         void getram( uint64_t project_index, name receiver );

         using create_action = eosio::action_wrapper<"gettoken"_n, &cryptor::gettoken>;
         using getrambytes_action = eosio::action_wrapper<"getram"_n, &cryptor::getram>;
      
      private:
         const string STATE_CREATED = "created";
         const string STATE_SELECTED = "selected";
         const string STATE_READIED = "readied";
         const string STATE_STARTED = "started";
         const string STATE_DROPPED = "dropped";
         const string STATE_NONE = "none";
         
         const string SUGGEST_SELECT = "select";
         const string SUGGEST_START = "start";
         const string SUGGEST_CANCELREADY = "cancelready";
         const string SUGGEST_DROP = "drop";
         
         const uint64_t SCOPE_STATE_CREATED = name(STATE_CREATED).value;
         const uint64_t SCOPE_STATE_SELECTED = name(STATE_SELECTED).value;
         const uint64_t SCOPE_STATE_READIED = name(STATE_READIED).value;
         const uint64_t SCOPE_STATE_STARTED = name(STATE_STARTED).value;
         const uint64_t SCOPE_STATE_DROPPED = name(STATE_DROPPED).value;
         const uint64_t SCOPE_STATE_NONE  = name(STATE_NONE).value;
         
         const uint64_t SCOPE_SUGGEST_SELECT = name(SUGGEST_SELECT).value;
         const uint64_t SCOPE_SUGGEST_START = name(SUGGEST_START).value;
         const uint64_t SCOPE_SUGGEST_CANCELREADY = name(SUGGEST_CANCELREADY).value;
         const uint64_t SCOPE_SUGGEST_DROP = name(SUGGEST_DROP).value;      

   };

} /// namespace eosio
