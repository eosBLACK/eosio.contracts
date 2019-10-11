/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eb.cryptor/eb.cryptor.hpp>

namespace eosio {
    
using namespace eosiosystem;    
    
time_point current_time_point() {
   const static time_point ct{ microseconds{ static_cast<int64_t>( current_time() ) } };
   return ct;
}    

void cryptor::gettoken( uint64_t project_index,
                        name receiver ) {
    project_table project_t_started(name("eb.factory"), SCOPE_STATE_STARTED);      
    payment_table payment_t(name("eb.factory"), project_index);
    resource_table resource_t(name("eb.factory"), project_index);
    rewardtoken_table rewardtoken_t(_self, project_index);
    
    // 1. check pre-requirements
    // a. project state == started
    // b. project owner == transaction signer
    auto itr_started = project_t_started.find(project_index);
    if (itr_started == project_t_started.end()) {
        check(false, "The project wasn't launched");
    } else {
        require_auth(itr_started->owner);
    }     
    
    
    // 2. check payment info
    for (auto itr_payment = payment_t.begin(); itr_payment != payment_t.end(); itr_payment++) {
        // check payment time
        if (current_time_point().sec_since_epoch() > itr_payment->aftertime) {
            auto itr_rewardtoken = rewardtoken_t.find(itr_payment->index);
            if (itr_rewardtoken == rewardtoken_t.end()) {
                
                
                rewardtoken_t.emplace(_self, [&](auto& a) {
                    a.index = itr_payment->index;
                    a.is_rewarded = true;
                });
                
                // pay reward
                auto itr_resource = resource_t.find(itr_started->started_resource_index);
                asset pay_amount = itr_resource->black_max_count;
                pay_amount.amount = pay_amount.amount * itr_payment->percentage / 100;
                
                /*
                char strarray[128];
                
                
                sprintf(strarray, "itr_started->started_resource_index: %lld, pay_amount.amount: %lld, itr_payment->percentage: %lld, itr_payment->index: %lld",  
                                    itr_started->started_resource_index, \
                                    pay_amount.amount, \
                                    itr_payment->percentage, \
                                    itr_payment->index);           
                
                
                sprintf(strarray, "current_time_point().sec_since_epoch(): %lld, itr_payment->aftertime: %lld", 
                                    current_time_point().sec_since_epoch(), \
                                    itr_payment->aftertime);
                
                check(false, strarray);
                */
                
                token::transfer_action transfer("eosio.token"_n, {_self, "active"_n});
                //create.send("eosio"_n, asset( 10000000000, symbol("PROA", 4)));
                transfer.send(_self, receiver, pay_amount, "reward");
                //transfer.send(_self, receiver, asset( 100000000000, symbol("BLACK", 4)), "reward");
                
            }
        }
    }
}

void cryptor::getram( uint64_t project_index, 
                        name receiver ) {
    project_table project_t_started(name("eb.factory"), SCOPE_STATE_STARTED);   
    resource_table resource_t(name("eb.factory"), project_index);
    rewardram_table rewardram_t(_self, project_index);
    
    // 1. check pre-requirements
    // a. project state == started
    // b. project owner == transaction signer
    auto itr_started = project_t_started.find(project_index);
    if (itr_started == project_t_started.end()) {
        check(false, "The project wasn't launched");
    } else {
        require_auth(itr_started->owner);
    }                                  
                                
    auto itr_resource = resource_t.find(itr_started->started_resource_index);      
    
    auto itr_rewardram = rewardram_t.find(itr_resource->index);
    if (itr_rewardram == rewardram_t.end()) {
        rewardram_t.emplace(_self, [&](auto& a) {
            a.index = itr_resource->index;
            a.is_rewarded = true;
        });
        
        system_contract::buyrambytes_action buyrambytes("eosio"_n, {_self, "active"_n});
        buyrambytes.send( _self, receiver, itr_resource->ram_max_size);  
    }    
}
                        

} /// namespace eosio

EOSIO_DISPATCH( eosio::cryptor, (gettoken)(getram) )

