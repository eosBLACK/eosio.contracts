#include <eb.factory/eb.factory.hpp>

namespace eosio {
    
time_point current_time_point() {
   const static time_point ct{ microseconds{ static_cast<int64_t>( current_time() ) } };
   return ct;
}    
    
using namespace eosiosystem;

void factory::create(name owner,
                     string project_name,
                     string url,
                     string hash,
                     string homepage,
                     string icon,
                     name helper) {
    require_auth( owner );         
    check(project_name.length() <= 32, "max project name length is 32");

    checksum256 prj_name_hash = sha256(project_name.c_str(), project_name.length());

    // TODO BY SOULHAMMER 
    // check project name collision in all scope

    project_table project_t(_self, SCOPE_CREATED);
    
    auto projectItems = project_t.get_index<name("projnamehash")>();
    auto itr = projectItems.find(prj_name_hash);
    if (itr == projectItems.end()) {
        uint64_t project_index;
        
        project_t.emplace(owner, [&](auto& a) {
            a.index = project_t.available_primary_key();
            a.owner = owner;
            a.proj_name = project_name;
            a.proj_name_hash = prj_name_hash;
           
            project_index = a.index;
        });        
        
        addProjectInfo(owner, project_index, url, hash, homepage, icon, helper);
    } else {
        check(false, "already same project name exist");
    }
}

void factory::addinfo(uint64_t project_index,
                        string url,
                        string hash,
                        string homepage,
                        string icon,
                        name helper) {
    uint64_t detectedScope = getProjectScope(project_index);
    
    if (detectedScope == SCOPE_CREATED || detectedScope == SCOPE_SELECTED) {
        project_table project_t(_self, detectedScope);
        auto itr = project_t.find(project_index);  
        require_auth(itr->owner);
        
        addProjectInfo(itr->owner, project_index, url, hash, homepage, icon, helper);
    } else {
        check(false, "adding info is impossible in current state");
    }
}

void factory::addresource(uint64_t project_index,
                           uint64_t ram_max_size,
                           asset black_max_count) {
    uint64_t detectedScope = getProjectScope(project_index);
    
    if (detectedScope == SCOPE_CREATED || detectedScope == SCOPE_SELECTED) {
        project_table project_t(_self, detectedScope);
        auto itr = project_t.find(project_index);  
        require_auth(itr->owner);
        
        resource_table resource_t(_self, project_index);
        resource_t.emplace(itr->owner, [&](auto& a) {
            a.index = resource_t.available_primary_key();
            a.ram_max_size = ram_max_size;
            a.black_max_count = black_max_count;
        }); 
    } else {
        check(false, "adding resource is impossible in current state");
    }
}

void factory::rmresource(uint64_t project_index,
                           uint64_t resource_index) {
    uint64_t detectedScope = getProjectScope(project_index);
    
    if (detectedScope == SCOPE_CREATED || detectedScope == SCOPE_SELECTED) {
        project_table project_t(_self, detectedScope);
        auto itr = project_t.find(project_index);  
        require_auth(itr->owner);
        
        resource_table resource_t(_self, project_index);
        auto itr2 = resource_t.find(resource_index);
        resource_t.erase(itr2);
    } else {
        check(false, "remove resource is impossible in current state");
    }
}

void factory::addpayment(uint64_t project_index,
                           uint64_t aftertime,
                           uint8_t percentage) {
    uint64_t detectedScope = getProjectScope(project_index);
    
    if (detectedScope == SCOPE_CREATED || detectedScope == SCOPE_SELECTED) {
        project_table project_t(_self, detectedScope);
        auto itr = project_t.find(project_index);  
        require_auth(itr->owner);
        
        payment_table payment_t(_self, project_index);
        payment_t.emplace(itr->owner, [&](auto& a) {
            a.index = payment_t.available_primary_key();
            a.aftertime = aftertime;
            a.percentage = percentage;
        }); 
    } else {
        check(false, "adding payment is impossible in current state");
    }
}

void factory::rmpayment(uint64_t project_index,
                        uint64_t payment_index) {
    uint64_t detectedScope = getProjectScope(project_index);
    
    if (detectedScope == SCOPE_CREATED || detectedScope == SCOPE_SELECTED) {
        project_table project_t(_self, detectedScope);
        auto itr = project_t.find(project_index);  
        require_auth(itr->owner);
        
        payment_table payment_t(_self, project_index);
        auto itr2 = payment_t.find(payment_index);
        payment_t.erase(itr2);
    } else {
        check(false, "remove payment is impossible in current state");
    }
}

void factory::drop(uint64_t project_index) {
    uint64_t detectedScope = getProjectScope(project_index);
    
    if (detectedScope == SCOPE_CREATED) {
        project_table project_t(_self, SCOPE_CREATED);
        auto itr = project_t.find(project_index);
        
        require_auth(itr->owner);
        project_t.erase(itr);
        cleanProject(project_index);
    } else if (detectedScope == SCOPE_SELECTED || detectedScope == SCOPE_READIED) {
        // TODY BY SOULHAMMER
        // implement related logic
        check(false, "not implemented");
    } else if (detectedScope == SCOPE_STARTED) {
        check(false, "already project was started");
    } else if (detectedScope == SCOPE_DROPPED) {
        check(false, "already project was dropped");
    } else if (detectedScope == SCOPE_NONE) {
        check(false, "project doesn't exist");
    }
}

void factory::setready(uint64_t project_index) {

}

void factory::cancelready(uint64_t project_index) {

}

void factory::select(uint64_t project_index,
                     uint64_t detail_index,
                     uint64_t resource_index,
                     name helper) {

}

void factory::start(uint64_t project_index,
                     uint64_t detail_index,
                     uint64_t resource_index,
                     name helper) {

}

void factory::addsuggest(uint64_t project_index,
                           name proposer,
                           string proposal) {
    
}

void factory::rmsuggest(uint64_t project_index) {

}


///////////////////////
// private functions //
///////////////////////
void factory::addProjectInfo(name owner,
                    uint64_t project_index,
                    string url,
                    string hash,
                    string homepage,
                    string icon,
                    name helper) {
    projectinfo_table projectinfo_t(_self, project_index);
    projectinfo_t.emplace(owner, [&](auto& a) {
        a.index = projectinfo_t.available_primary_key();
        a.updatetime = current_time_point().sec_since_epoch();
        a.url = url;
        a.hash = hash;
        a.homepage = homepage;
        a.icon = icon;
        a.helper = helper;
    });                        
}

void factory::cleanProject(uint64_t project_index) {
    deleteProjectInfo(project_index);
    deleteProjectResource(project_index);
    deleteProjectPayment(project_index);
    deleteProjectHelper(project_index);
}

void factory::deleteProjectInfo(uint64_t project_index) {
    projectinfo_table projectinfo_t(_self, project_index);
    
    auto it = projectinfo_t.begin();
    while (it != projectinfo_t.end()) {
        it = projectinfo_t.erase(it);
    }
}

void factory::deleteProjectResource(uint64_t project_index) {
    resource_table resource_t(_self, project_index);
    
    auto it = resource_t.begin();
    while (it != resource_t.end()) {
        it = resource_t.erase(it);
    }    
}
 
void factory::deleteProjectPayment(uint64_t project_index) {
    payment_table payment_t(_self, project_index);
    
    auto it = payment_t.begin();
    while (it != payment_t.end()) {
        it = payment_t.erase(it);
    }  
} 

void factory::deleteProjectHelper(uint64_t project_index) {
    helper_table helper_t(_self, SCOPE_SELECTED);
    
    auto itr = helper_t.find(project_index);
    if (itr != helper_t.end()) {
        helper_t.erase(itr);
    }
}

uint64_t factory::getProjectScope(uint64_t project_index) {
    uint64_t result;
    
    project_table project_t_created(_self, SCOPE_CREATED);
    project_table project_t_selected(_self, SCOPE_SELECTED);
    project_table project_t_readied(_self, SCOPE_READIED);
    project_table project_t_started(_self, SCOPE_STARTED);
    project_table project_t_dropped(_self, SCOPE_DROPPED);
    
    if (project_t_created.find(project_index) != project_t_created.end()) {
        result = SCOPE_CREATED;
    } else if (project_t_selected.find(project_index) != project_t_selected.end()) {
        result = SCOPE_SELECTED;
    } else if (project_t_readied.find(project_index) != project_t_readied.end()) {
        result = SCOPE_READIED;
    } else if (project_t_started.find(project_index) != project_t_started.end()) {
        result = SCOPE_STARTED;
    } else if (project_t_dropped.find(project_index) != project_t_dropped.end()) {
        result = SCOPE_DROPPED;
    } else {
        result = SCOPE_NONE;
    }
    
    return result;
}

} /// namespace eosio

EOSIO_DISPATCH( eosio::factory, (create)(addinfo)(addresource)(rmresource)(addpayment)(rmpayment)
                                (setready)(cancelready)(drop)(select)(start)(addsuggest)(rmsuggest) )

