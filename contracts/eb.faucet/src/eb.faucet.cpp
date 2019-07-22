#include <eb.faucet/eb.faucet.hpp>


namespace eosio {
    
using namespace eosiosystem;

void faucet::newaccount(const name sender, 
                        const eosio::public_key owner_key, 
                        const eosio::public_key active_key ) {
    struct account_create_info_t data;

    data.create = name("eb.faucet");
    data.name = sender;
    data.owner_key = owner_key;
    data.active_key = active_key;
    data.stake_cpu = default_cpu_stake;
    data.stake_net = default_net_stake;
    data.ram_amount_bytes = default_ram_amount_bytes;

    create_account(data);
}

void faucet::sendtoken( name receiver ) {
    faucet_limit_table faucet_limit_t(_self, _self.value);
    
    bool skip = false;
    
    auto account =  faucet_limit_t.find(receiver.value);
    if (account == faucet_limit_t.end()) {
        faucet_limit_t.emplace(_self, [&](auto& a) {
           a.account = receiver;
           a.last_time_sec = now();
        });
    } else {
        faucet_limit_t.modify(account, same_payer, [&](auto& a) {
            auto now_time_sec = now();
            auto time_span_sec = now_time_sec - a.last_time_sec;
            uint32_t hour_6_sec = 6 * 60 * 60;
            if (time_span_sec > hour_6_sec) {
                a.last_time_sec = now_time_sec;
            } else {
                check(false, "Opportunity is only every six hours");
            }
        });        
    }
    
    token::transfer_action transfer("eosio.token"_n, {name("eb.faucet"), "active"_n});
    transfer.send(name("eb.faucet"), receiver, asset(1000000, core_symbol), "faucet");          
}

void faucet::create_account(struct account_create_info_t& data) {
    const key_weight owner_pubkey_weight{
      .key = data.owner_key,
      .weight = 1,
    };
    const key_weight active_pubkey_weight{
        .key = data.active_key,
        .weight = 1,
    };
    const authority owner{
        .threshold = 1,
        .keys = {owner_pubkey_weight},
        .accounts = {},
        .waits = {}
    };
    const authority active{
        .threshold = 1,
        .keys = {active_pubkey_weight},
        .accounts = {},
        .waits = {}
    };
    const newaccount_t new_account{
        .creator = _self,
        .name = data.name,
        .owner = owner,
        .active = active
    };
    
    const auto stake_cpu = data.stake_cpu;
    const auto stake_net = default_net_stake;
    const auto rent_cpu_amount = asset{10, core_symbol};
    
    action(
      permission_level{ _self, "active"_n, },
      "eosio"_n,
      "newaccount"_n,
      new_account
    ).send();
    
    action(
      permission_level{ data.create, "active"_n},
      "eosio"_n,
      "buyrambytes"_n,
      make_tuple(data.create, data.name, data.ram_amount_bytes)
    ).send();

    action(
      permission_level{ data.create, "active"_n},
      "eosio"_n,
      "delegatebw"_n,
      make_tuple(data.create, data.name, data.stake_net, data.stake_cpu, true)
    ).send();
} 

} /// namespace eosio

EOSIO_DISPATCH( eosio::faucet, (newaccount)(sendtoken) )

