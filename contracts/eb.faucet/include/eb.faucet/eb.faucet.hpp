#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/ignore.hpp>
#include <eosiolib/transaction.hpp>
#include <eosio.token/eosio.token.hpp>
#include <eosio.system/eosio.system.hpp>

#include <string>

#include "exchange_state.hpp"
#include "public_key.hpp"

namespace eosio {
   
   using std::string;
   
    struct [[eosio::table,eosio::contract("eb.faucet")]] faucet_limit {
        name account;
        uint32_t last_time_sec;

        uint64_t primary_key() const {return account.value;}
      
        EOSLIB_SERIALIZE( faucet_limit, (account)(last_time_sec) )
    };    

    struct permission_level_weight {
        permission_level  permission;
        uint16_t          weight;

        // explicit serialization macro is not necessary, used here only to improve compilation time
        EOSLIB_SERIALIZE( permission_level_weight, (permission)(weight) )
    };

    struct key_weight {
        eosio::public_key  key;
        uint16_t           weight;

        // explicit serialization macro is not necessary, used here only to improve compilation time
        EOSLIB_SERIALIZE( key_weight, (key)(weight) )
    };   
   
    struct wait_weight {
        uint32_t           wait_sec;
        uint16_t           weight;

        // explicit serialization macro is not necessary, used here only to improve compilation time
        EOSLIB_SERIALIZE( wait_weight, (wait_sec)(weight) )
    };   
   
    struct authority {
        uint32_t                              threshold = 0;
        std::vector<key_weight>               keys;
        std::vector<permission_level_weight>  accounts;
        std::vector<wait_weight>              waits;

        // explicit serialization macro is not necessary, used here only to improve compilation time
        EOSLIB_SERIALIZE( authority, (threshold)(keys)(accounts)(waits) )
    };   
   
    struct newaccount_t {
        name creator;
        name name;
        authority owner;
        authority active;
    };   

    struct account_create_info_t {
        name create;
        name name;
        eosio::public_key owner_key;
        eosio::public_key active_key;
        asset stake_cpu;
        asset stake_net;
        uint32_t ram_amount_bytes;
    };
    
    typedef eosio::multi_index< "faucetlimit"_n, faucet_limit > faucet_limit_table;

    class [[eosio::contract("eb.faucet")]] faucet : public contract {
        public:
            using contract::contract;
         
            static constexpr symbol core_symbol{"BLACK", 4};

            const asset default_cpu_stake{10000, core_symbol};
            const asset default_net_stake{10000, core_symbol};
            const uint32_t default_ram_amount_bytes{4096};

            [[eosio::action]]
            void newaccount(const name sender,
                            const eosio::public_key owner_key, 
                            const eosio::public_key active_key );

            [[eosio::action]]
            void sendtoken( name receiver );            

            using newaccount_action = eosio::action_wrapper<"newaccount"_n, &faucet::newaccount>;
            using unstaking_action = eosio::action_wrapper<"sendtoken"_n, &faucet::sendtoken>;
         
        private:
            void create_account(struct account_create_info_t& data);    
   };

} /// namespace eosio
