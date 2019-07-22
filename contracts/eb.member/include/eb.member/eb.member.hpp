#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/ignore.hpp>
#include <eosiolib/transaction.hpp>
#include <eosio.system/eosio.system.hpp>

#include <string>

namespace eosio {
   
   using std::string;
   
   struct [[eosio::table,eosio::contract("eb.member")]] criteria {
      name member_type;
      
      asset low_quantity;
      asset high_quantity;
      
      uint64_t primary_key() const {return member_type.value;}
      
      EOSLIB_SERIALIZE( criteria, (member_type)(low_quantity)(high_quantity) )
   };

   struct [[eosio::table,eosio::contract("eb.member")]] participant {
      name account;

      uint64_t primary_key() const {return account.value;}
      
      EOSLIB_SERIALIZE( participant, (account) )
   };  
   
   struct [[eosio::table,eosio::contract("eb.member")]] supporter {
      name account;

      uint64_t primary_key() const {return account.value;}
      
      EOSLIB_SERIALIZE( supporter, (account) )
   };  
   
   struct [[eosio::table,eosio::contract("eb.member")]] representative_candidate {
      name account;

      uint64_t primary_key() const {return account.value;}
      
      EOSLIB_SERIALIZE( representative_candidate, (account) )
   };    

   struct [[eosio::table, eosio::contract("eosio.system")]] user_resources {
      name          owner;
      asset         net_weight;
      asset         cpu_weight;
      int64_t       ram_bytes = 0;

      bool is_empty()const { return net_weight.amount == 0 && cpu_weight.amount == 0 && ram_bytes == 0; }
      uint64_t primary_key()const { return owner.value; }

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( user_resources, (owner)(net_weight)(cpu_weight)(ram_bytes) )
   };

   typedef eosio::multi_index< "criterias"_n, criteria > criteria_table;
   typedef eosio::multi_index< "participants"_n, participant > participant_table;
   typedef eosio::multi_index< "supporters"_n, supporter > supporter_table;
   typedef eosio::multi_index< "reprecandi"_n, representative_candidate > representative_candidate_table;
   
   typedef eosio::multi_index< "userres"_n, user_resources > user_resources_table;

   class [[eosio::contract("eb.member")]] member : public contract {
      public:
         using contract::contract;
         
         enum members { participants, supporters, reprecandi };
         const char *members_str[4]={ "participants","supporters", "reprecandi" }; 

         [[eosio::action]]
         void setcriteria( name member_type,
                           asset   low_quantity,
                           asset   high_quantity);

         [[eosio::action]]
         void staking( name from, 
                        name receiver, 
                        asset stake_net_quantity, 
                        asset stake_cpu_quantity, 
                        bool transfer );

         [[eosio::action]]
         void unstaking( name from, 
                           name receiver, 
                           asset unstake_net_quantity, 
                           asset unstake_cpu_quantity );     
                           
         [[eosio::action]]
         void deletetables(name target_scope);              

         using setcriteria_action = eosio::action_wrapper<"setcriteria"_n, &member::setcriteria>;
         using staking_action = eosio::action_wrapper<"staking"_n, &member::staking>;
         using unstaking_action = eosio::action_wrapper<"unstaking"_n, &member::unstaking>;
         
         using deletetables_action = eosio::action_wrapper<"deletetables"_n, &member::deletetables>;
         
      private:
         std::array<uint64_t, 2> get_criteria_range(string target);
         string get_member_type(uint64_t staking_sum);
         uint64_t get_staking_sum(name account);
         void update_member_type(string old_type, string new_type, name account);
   };

} /// namespace eosio
