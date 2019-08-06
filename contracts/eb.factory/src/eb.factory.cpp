#include <eb.factory/eb.factory.hpp>


namespace eosio {
    
using namespace eosiosystem;

void factory::create(name owner,
                     string project_name,
                     string url,
                     string hash,
                     string homepage,
                     string icon,
                     name helper) {

}

void factory::addinfo(int64_t project_index,
                        string url,
                        string hash,
                        string homepage,
                        string icon,
                        name helper) {

}

void factory::addresource(int64_t project_index,
                           int64_t ram_max_size,
                           int64_t black_max_count) {

}

void factory::rmresource(int64_t project_index,
                           int64_t resource_index) {

}

void factory::addpayment(int64_t project_index,
                           int64_t aftertime,
                           int8_t percentage) {

}

void factory::rmpayment(int64_t project_index,
                        int64_t aftertime) {

}

void factory::setready(int64_t project_index) {

}

void factory::cancelready(int64_t project_index) {

}

void factory::drop(int64_t project_index) {

}

void factory::select(int64_t project_index,
                     int64_t detail_index,
                     int64_t resource_index,
                     name helper) {

}

void factory::start(int64_t project_index,
                     int64_t detail_index,
                     int64_t resource_index,
                     name helper) {

}

void factory::addsuggest(int64_t project_index,
                           name proposer,
                           string proposal) {

}

void factory::rmsuggest(int64_t project_index) {

}



} /// namespace eosio

EOSIO_DISPATCH( eosio::factory, (create)(addinfo)(addresource)(rmresource)(addpayment)(rmpayment)
                                (setready)(cancelready)(drop)(select)(start)(addsuggest)(rmsuggest) )

