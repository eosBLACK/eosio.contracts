#include <eb.member/eb.member.hpp>


namespace eosio {
    
using namespace eosiosystem;

void member::setcriteria( name member_type, asset low_quantity, asset high_quantity) {
    // TODO BY SOULHAMMER
    // check overlap range, don't permit
    
    require_auth(_self);
    char buffer[256];
    sprintf(buffer, "member_type is %s or %s or %s", members_str[participants], members_str[supporters], members_str[reprecandi]);
    check( member_type.to_string().compare(members_str[participants]) == 0 ||
            member_type.to_string().compare(members_str[supporters]) == 0 ||
            member_type.to_string().compare(members_str[reprecandi]) == 0, buffer );
    
    criteria_table criteria_t(_self, _self.value);
    
    auto type =  criteria_t.find(member_type.value);
    if (type == criteria_t.end()) {
        criteria_t.emplace(_self, [&](auto& a) {
           a.member_type = member_type;
           a.low_quantity = low_quantity;
           a.high_quantity = high_quantity;
        });
    } else {
        criteria_t.modify(type, same_payer, [&](auto& a) {
           a.member_type = member_type;
           a.low_quantity = low_quantity;
           a.high_quantity = high_quantity;
        });        
    }
}

void member::staking( name from, 
                        name receiver, 
                        asset stake_net_quantity, 
                        asset stake_cpu_quantity, 
                        bool transfer ) {
    int64_t old_sum = get_staking_sum(receiver);
    
    system_contract::delegatebw_action delegatebw("eosio"_n, {from, "active"_n});
    delegatebw.send(from, receiver, stake_net_quantity, stake_cpu_quantity, transfer);
    
    int64_t new_sum = stake_net_quantity.amount + stake_cpu_quantity.amount;
    int64_t totol_sum = old_sum + new_sum;
    
    string old_mem_type = get_member_type(old_sum);
    string new_mem_type = get_member_type(totol_sum);
    
    update_member_type(old_mem_type, new_mem_type, receiver);
}

void member::unstaking( name from, 
                        name receiver, 
                        asset unstake_net_quantity,
                        asset unstake_cpu_quantity ) {
    int64_t old_sum = get_staking_sum(receiver);
    
    system_contract::undelegatebw_action undelegatebw("eosio"_n, {from, "active"_n});
    undelegatebw.send(from, receiver, unstake_net_quantity, unstake_cpu_quantity);
    
    int64_t new_sum = unstake_net_quantity.amount + unstake_cpu_quantity.amount;
    int64_t totol_sum = old_sum - new_sum;
    
    string old_mem_type = get_member_type(old_sum);
    string new_mem_type = get_member_type(totol_sum);
    
    update_member_type(old_mem_type, new_mem_type, receiver);
}

void member::deletetables(name target_scope) {
    participant_table participant_t(_self, target_scope.value);
    supporter_table supporter_t(_self, target_scope.value);
    representative_candidate_table representative_candidate_t(_self, target_scope.value);
    
    auto it = participant_t.begin();
    while (it != participant_t.end()) {
        it = participant_t.erase(it);
    }    
    
    auto it2 = supporter_t.begin();
    while (it2 != supporter_t.end()) {
        it2 = supporter_t.erase(it2);
    }
    
    auto it3 = representative_candidate_t.begin();
    while (it3 != representative_candidate_t.end()) {
        it3 = representative_candidate_t.erase(it3);
    }
    
    /*
    if (old_type.compare(members_str[participants]) == 0) {
        auto type =  participant_t.find(account.value);
        if (type == participant_t.end()) {
        } else {
            participant_t.erase(type);        
        }         
    } else if (old_type.compare(members_str[supporters]) == 0) {
        auto type =  supporter_t.find(account.value);
        if (type == supporter_t.end()) {
        } else {
            supporter_t.erase(type);        
        }          
    } else if (old_type.compare(members_str[reprecandi]) == 0) {
        auto type =  representative_candidate_t.find(account.value);
        if (type == representative_candidate_t.end()) {
        } else {
            representative_candidate_t.erase(type);        
        }          
    }    
    */
}

std::array<uint64_t, 2> member::get_criteria_range(string target) {
    std::array<uint64_t, 2> criterias;
    
    asset low_quantity;
    asset high_quantity;
    
    criteria_table criteria_t(_self, _self.value);
    auto iterator = criteria_t.find(name(target).value);
    if (iterator == criteria_t.end()) {
       
    } else {
       low_quantity = iterator->low_quantity;
       high_quantity = iterator->high_quantity;
    }    
    
    criterias.at(0) = low_quantity.amount;
    criterias.at(1) = high_quantity.amount;
    
    return criterias;
}

string member::get_member_type(uint64_t staking_sum) {
   std::array<uint64_t, 2> criterias_participants;
   std::array<uint64_t, 2> criterias_supporters;
   std::array<uint64_t, 2> criterias_represent_candidates;
   
   criterias_participants = get_criteria_range(members_str[participants]);
   criterias_supporters = get_criteria_range(members_str[supporters]);
   criterias_represent_candidates = get_criteria_range(members_str[reprecandi]);
   
   string update_mem_type;
   
   if (criterias_participants.at(0) <= staking_sum && staking_sum <= criterias_participants.at(1)) {
       update_mem_type = members_str[participants];
   } else if (criterias_supporters.at(0) <= staking_sum && staking_sum <= criterias_supporters.at(1)) {
       update_mem_type = members_str[supporters];
   } else if (criterias_represent_candidates.at(0) <= staking_sum) {
       update_mem_type = members_str[reprecandi];
   } 
   
   return update_mem_type;
}

uint64_t member::get_staking_sum(name account) {
    user_resources_table user_res_t(name("eosio"), account.value);
    
    asset net_weight;
    asset cpu_weight;
    
    auto iterator = user_res_t.find(account.value);
    if (iterator == user_res_t.end()) {
       
    } else {
       net_weight = iterator->net_weight;
       cpu_weight = iterator->cpu_weight;
    }  
    
    uint64_t sum = net_weight.amount + cpu_weight.amount;
    
    return sum;
}

void member::update_member_type(string old_type, string new_type, name account) {
    
    participant_table participant_t(_self, _self.value);
    supporter_table supporter_t(_self, _self.value);
    representative_candidate_table representative_candidate_t(_self, _self.value);
    
    /*
    participant_table participant_t(_self, account.value);
    supporter_table supporter_t(_self, account.value);
    representative_candidate_table representative_candidate_t(_self, account.value);
    */
    
    if (old_type.compare(members_str[participants]) == 0) {
        auto type =  participant_t.find(account.value);
        if (type == participant_t.end()) {
        } else {
            participant_t.erase(type);        
        }         
    } else if (old_type.compare(members_str[supporters]) == 0) {
        auto type =  supporter_t.find(account.value);
        if (type == supporter_t.end()) {
        } else {
            supporter_t.erase(type);        
        }          
    } else if (old_type.compare(members_str[reprecandi]) == 0) {
        auto type =  representative_candidate_t.find(account.value);
        if (type == representative_candidate_t.end()) {
        } else {
            representative_candidate_t.erase(type);        
        }          
    }
    
    if (new_type.compare(members_str[participants]) == 0) {
        auto type =  participant_t.find(account.value);
        if (type == participant_t.end()) {
            participant_t.emplace(_self, [&](auto& a) {
               a.account = account;
            });            
        }        
    } else if (new_type.compare(members_str[supporters]) == 0) {
        auto type =  supporter_t.find(account.value);
        if (type == supporter_t.end()) {
            supporter_t.emplace(_self, [&](auto& a) {
               a.account = account;
            });               
        }      
    } else if (new_type.compare(members_str[reprecandi]) == 0) {
        auto type =  representative_candidate_t.find(account.value);
        if (type == representative_candidate_t.end()) {
            representative_candidate_t.emplace(_self, [&](auto& a) {
               a.account = account;
            });             
        }      
    }   
}

} /// namespace eosio

EOSIO_DISPATCH( eosio::member, (staking)(unstaking)(setcriteria)(deletetables) )

