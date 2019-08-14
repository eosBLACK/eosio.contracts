#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/ignore.hpp>
#include <eosiolib/transaction.hpp>
#include <eosio.system/eosio.system.hpp>

#include <string>

namespace eosio {
   
   using std::string;
   
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
   
   struct [[eosio::table,eosio::contract("eb.factory")]] projectinfo {
      uint64_t index;
      uint32_t updatetime;
      string url;
      string hash;
      string homepage;
      string icon;
      name helper;

      uint64_t primary_key() const {return index;}
      
      EOSLIB_SERIALIZE( projectinfo, (index)(updatetime)(url)(hash)(homepage)(icon)(helper) )
   };   
   
   struct [[eosio::table,eosio::contract("eb.factory")]] resource {
      uint64_t index;
      uint64_t ram_max_size;
      asset black_max_count;

      uint64_t primary_key() const {return index;}
      
      EOSLIB_SERIALIZE( resource, (index)(ram_max_size)(black_max_count) )
   };
   
   struct [[eosio::table,eosio::contract("eb.factory")]] payment {
      uint64_t index;
      uint64_t aftertime;
      int8_t percentage;

      uint64_t primary_key() const {return index;}
      
      EOSLIB_SERIALIZE( payment, (index)(aftertime)(percentage) )
   };  
   
   struct [[eosio::table,eosio::contract("eb.factory")]] helper {
      uint64_t project_index;
      name helper_name;
      bool isrewarded;

      uint64_t primary_key() const {return project_index;}
      
      EOSLIB_SERIALIZE( helper, (project_index)(helper_name)(isrewarded) )
   };   
   
   struct [[eosio::table,eosio::contract("eb.factory")]] notice {
      uint64_t index;
      uint64_t project_index;
      name proposer;
      string proposal;

      uint64_t primary_key() const {return index;}
      uint64_t by_proj_index() const {return project_index;}
      
      EOSLIB_SERIALIZE( notice, (index)(project_index)(proposer)(proposal) )
   };   

   typedef eosio::multi_index< "projects"_n, project, indexed_by<name("projnamehash"), const_mem_fun<project, checksum256, &project::by_proj_name_hash>> > project_table;
   typedef eosio::multi_index< "projectinfos"_n, projectinfo > projectinfo_table;
   typedef eosio::multi_index< "resources"_n, resource > resource_table;
   typedef eosio::multi_index< "payments"_n, payment > payment_table;
   typedef eosio::multi_index< "helpers"_n, helper > helper_table;
   typedef eosio::multi_index< "notices"_n, notice, indexed_by<name("projindex"), const_mem_fun<notice, uint64_t, &notice::by_proj_index>> > notice_table;

   class [[eosio::contract("eb.factory")]] factory : public contract {
      public:
         using contract::contract;
         
         [[eosio::action]]
         void create(name owner,
                     string project_name,
                     string url,
                     string hash,
                     string homepage,
                     string icon,
                     name helper);
         
         [[eosio::action]]
         void addinfo(uint64_t project_index,
                        string url,
                        string hash,
                        string homepage,
                        string icon,
                        name helper);
         
         [[eosio::action]]
         void addresource(uint64_t project_index,
                           uint64_t ram_max_size,
                           asset black_max_count);
         
         [[eosio::action]]
         void rmresource(uint64_t project_index,
                           uint64_t resource_index);
         
         [[eosio::action]]
         void addpayment(uint64_t project_index,
                           uint64_t aftertime,
                           uint8_t percentage);
         
         [[eosio::action]]
         void rmpayment(uint64_t project_index,
                        uint64_t payment_index);
         
         [[eosio::action]]
         void setready(uint64_t project_index,
                        uint64_t detail_index,
                        uint64_t resource_index);
         
         [[eosio::action]]
         void cancelready(uint64_t project_index);
         
         [[eosio::action]]
         void drop(uint64_t project_index);
         
         [[eosio::action]]
         void select(uint64_t project_index,
                     uint64_t detail_index,
                     uint64_t resource_index,
                     name helper);
         
         [[eosio::action]]
         void start(uint64_t project_index,
                     uint64_t detail_index,
                     uint64_t resource_index,
                     name helper);
         
         [[eosio::action]]
         void pushnotice(uint64_t project_index,
                           name proposer,
                           string proposal,
                           string target);
         
         [[eosio::action]]
         void popnotice(uint64_t project_index,
                        string target);

         using create_action = eosio::action_wrapper<"create"_n, &factory::create>;
         using addinfo_action = eosio::action_wrapper<"addinfo"_n, &factory::addinfo>;
         using addresource_action = eosio::action_wrapper<"addresource"_n, &factory::addresource>;
         using rmresource_action = eosio::action_wrapper<"rmresource"_n, &factory::rmresource>;
         using addpayment_action = eosio::action_wrapper<"addpayment"_n, &factory::addpayment>;
         using rmpayment_action = eosio::action_wrapper<"rmpayment"_n, &factory::rmpayment>;
         using setready_action = eosio::action_wrapper<"setready"_n, &factory::setready>;
         using cancelready_action = eosio::action_wrapper<"cancelready"_n, &factory::cancelready>;
         using drop_action = eosio::action_wrapper<"drop"_n, &factory::drop>;
         using select_action = eosio::action_wrapper<"select"_n, &factory::select>;
         using start_action = eosio::action_wrapper<"start"_n, &factory::start>;
         using pushnotice_action = eosio::action_wrapper<"pushnotice"_n, &factory::pushnotice>;
         using popnotice_action = eosio::action_wrapper<"popnotice"_n, &factory::popnotice>;         
         
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
         
         void addProjectInfo(name owner,
                              uint64_t project_index,
                              string url,
                              string hash,
                              string homepage,
                              string icon,
                              name helper);
         
         void cleanProject(uint64_t project_index);
         void deleteProjectInfo(uint64_t project_index);
         void deleteProjectResource(uint64_t project_index);
         void deleteProjectPayment(uint64_t project_index);
         void deleteProjectHelper(uint64_t project_index);
         
         uint64_t getProjectScope(uint64_t project_index);
         uint64_t target2Scope(string target);
   };

} /// namespace eosio
