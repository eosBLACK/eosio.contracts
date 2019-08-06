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
      int64_t index;
      name owner;
      string name;
      int64_t current_detail_index;
      int64_t current_resource_index;
      
      uint64_t primary_key() const {return index;}
      
      EOSLIB_SERIALIZE( project, (index)(owner)(name)(current_detail_index)(current_resource_index) )
   };
   
   struct [[eosio::table,eosio::contract("eb.factory")]] projectinfo {
      int64_t index;
      int64_t updatetime;
      string url;
      string hash;
      string homepage;
      string icon;
      name helper;

      uint64_t primary_key() const {return index;}
      
      EOSLIB_SERIALIZE( projectinfo, (index)(updatetime)(url)(hash)(homepage)(icon)(helper) )
   };   
   
   struct [[eosio::table,eosio::contract("eb.factory")]] resource {
      int64_t index;
      int64_t ram_max_size;
      int64_t black_max_count;

      uint64_t primary_key() const {return index;}
      
      EOSLIB_SERIALIZE( resource, (index)(ram_max_size)(black_max_count) )
   };
   
   struct [[eosio::table,eosio::contract("eb.factory")]] payment {
      int64_t aftertime;
      int8_t percentage;

      uint64_t primary_key() const {return aftertime;}
      
      EOSLIB_SERIALIZE( payment, (aftertime)(percentage) )
   };  
   
   struct [[eosio::table,eosio::contract("eb.factory")]] helper {
      int64_t project_index;
      name helper_name;
      bool isrewarded;

      uint64_t primary_key() const {return project_index;}
      
      EOSLIB_SERIALIZE( helper, (project_index)(helper_name)(isrewarded) )
   };   
   
   struct [[eosio::table,eosio::contract("eb.factory")]] suggest {
      int64_t project_index;
      name proposer;
      string proposal;

      uint64_t primary_key() const {return project_index;}
      
      EOSLIB_SERIALIZE( suggest, (project_index)(proposer)(proposal) )
   };   

   typedef eosio::multi_index< "projects"_n, project > project_table;
   typedef eosio::multi_index< "projectinfos"_n, projectinfo > projectinfo_table;
   typedef eosio::multi_index< "resources"_n, resource > resource_table;
   typedef eosio::multi_index< "payments"_n, payment > payment_table;
   typedef eosio::multi_index< "helpers"_n, helper > helper_table;
   typedef eosio::multi_index< "suggests"_n, suggest > suggest_table;

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
         void addinfo(int64_t project_index,
                        string url,
                        string hash,
                        string homepage,
                        string icon,
                        name helper);
         
         [[eosio::action]]
         void addresource(int64_t project_index,
                           int64_t ram_max_size,
                           int64_t black_max_count);
         
         [[eosio::action]]
         void rmresource(int64_t project_index,
                           int64_t resource_index);
         
         [[eosio::action]]
         void addpayment(int64_t project_index,
                           int64_t aftertime,
                           int8_t percentage);
         
         [[eosio::action]]
         void rmpayment(int64_t project_index,
                        int64_t aftertime);
         
         [[eosio::action]]
         void setready(int64_t project_index);
         
         [[eosio::action]]
         void cancelready(int64_t project_index);
         
         [[eosio::action]]
         void drop(int64_t project_index);
         
         [[eosio::action]]
         void select(int64_t project_index,
                     int64_t detail_index,
                     int64_t resource_index,
                     name helper);
         
         [[eosio::action]]
         void start(int64_t project_index,
                     int64_t detail_index,
                     int64_t resource_index,
                     name helper);
         
         [[eosio::action]]
         void addsuggest(int64_t project_index,
                           name proposer,
                           string proposal);
         
         [[eosio::action]]
         void rmsuggest(int64_t project_index);

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
         using addsuggest_action = eosio::action_wrapper<"addsuggest"_n, &factory::addsuggest>;
         using rmsuggest_action = eosio::action_wrapper<"rmsuggest"_n, &factory::rmsuggest>;         
         
      private:
         void test2();
   };

} /// namespace eosio
