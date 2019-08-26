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
    
    member::ismember_action ismember("eb.member"_n, {get_self(), "active"_n});
    ismember.send(owner);    

    checksum256 prj_name_hash = sha256(project_name.c_str(), project_name.length());

    // TODO BY SOULHAMMER 
    // check project name collision in all scope

    project_table project_t(_self, SCOPE_STATE_CREATED);
    
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
        check(false, "already project name exist");
    }
}

void factory::addinfo(uint64_t project_index,
                        string url,
                        string hash,
                        string homepage,
                        string icon,
                        name helper) {
    uint64_t detectedScope = getProjectScope(project_index);
    
    if (detectedScope == SCOPE_STATE_CREATED || detectedScope == SCOPE_STATE_SELECTED) {
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
    
    if (detectedScope == SCOPE_STATE_CREATED || detectedScope == SCOPE_STATE_SELECTED) {
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
    
    if (detectedScope == SCOPE_STATE_CREATED || detectedScope == SCOPE_STATE_SELECTED) {
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
    
    if (detectedScope == SCOPE_STATE_CREATED || detectedScope == SCOPE_STATE_SELECTED) {
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
    
    if (detectedScope == SCOPE_STATE_CREATED || detectedScope == SCOPE_STATE_SELECTED) {
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
    
    if (detectedScope == SCOPE_STATE_CREATED) {
        project_table project_t(_self, detectedScope);
        auto itr = project_t.find(project_index);
        
        require_auth(itr->owner);
        project_t.erase(itr);
    
        cleanProject(project_index);
    } else if (detectedScope == SCOPE_STATE_SELECTED || detectedScope == SCOPE_STATE_READIED) {
        // need msig by representatives
        require_auth( _self );
        
        project_table project_t(_self, detectedScope);
        auto itr = project_t.find(project_index);
        project_t.erase(itr);
    
        cleanProject(project_index);
    } else if (detectedScope == SCOPE_STATE_STARTED) {
        check(false, "already project was started");
    } else if (detectedScope == SCOPE_STATE_DROPPED) {
        check(false, "already project was dropped");
    } else if (detectedScope == SCOPE_STATE_NONE) {
        check(false, "project doesn't exist");
    }
}

void factory::setready(uint64_t project_index,
                        uint64_t detail_index,
                        uint64_t resource_index) {
    // check existence of project (scope: selected)                            
    project_table project_t_selected(_self, SCOPE_STATE_SELECTED);
    auto itr_selected = project_t_selected.find(project_index);  
    if (itr_selected != project_t_selected.end()) {
        // only project owner has permission
        require_auth(itr_selected->owner);      
        
        // move data in projects (scope: selected -> readied)
        project_table project_t_readied(_self, SCOPE_STATE_READIED);    
        auto itr_readied = project_t_readied.find(project_index);
        if (itr_readied == project_t_readied.end()) {
            project_t_readied.emplace(_self, [&](auto& a) {
                a.index = itr_selected->index;
                a.owner = itr_selected->owner;
                a.proj_name = itr_selected->proj_name;
                a.proj_name_hash = itr_selected->proj_name_hash;
                a.selected_detail_index = itr_selected->selected_detail_index;
                a.selected_resource_index = itr_selected->selected_resource_index;                
                a.started_detail_index = detail_index;
                a.started_resource_index = resource_index;
            });  
            
            project_t_selected.erase(itr_selected);
        } else {
            check(false, "already project was readied");
        }         
    } else {
        check(false, "non-project exist");
    }
}

void factory::cancelready(uint64_t project_index) {
    // need msig by representatives
    require_auth( _self );    
    
    // check existence of project (scope: readied)                            
    project_table project_t_readied(_self, SCOPE_STATE_READIED);
    auto itr_readied = project_t_readied.find(project_index);  
    if (itr_readied != project_t_readied.end()) {
        // only project owner has permission
        //require_auth(itr_readied->owner);
        
        // move data in projects (scope: readied -> selected)
        project_table project_t_selected(_self, SCOPE_STATE_SELECTED);    
        auto itr_selected = project_t_selected.find(project_index);
        if (itr_selected == project_t_selected.end()) {
            project_t_selected.emplace(_self, [&](auto& a) {
                a.index = itr_readied->index;
                a.owner = itr_readied->owner;
                a.proj_name = itr_readied->proj_name;
                a.proj_name_hash = itr_readied->proj_name_hash;
                a.selected_detail_index = itr_readied->selected_detail_index;
                a.selected_resource_index = itr_readied->selected_resource_index;                
                a.started_detail_index = itr_readied->started_detail_index;
                a.started_resource_index = itr_readied->started_resource_index;
            });  
            
            project_t_readied.erase(itr_readied);
        } else {
            check(false, "already project was readied");
        }         
    } else {
        check(false, "non-project exist");
    }
}

void factory::select(uint64_t project_index,
                     uint64_t detail_index,
                     uint64_t resource_index,
                     name helper) {
    // need msig by representatives
    require_auth( _self );
    
    // check existence of project (scope: created)
    project_table project_t_created(_self, SCOPE_STATE_CREATED);
    auto itr_created = project_t_created.find(project_index);   
    if (itr_created == project_t_created.end()) {
        check(false, "non-project exist");
    } else {
        // check existence of projectinfo
        projectinfo_table projectinfo_t(_self, project_index);
        auto itr_projectinfo = projectinfo_t.find(detail_index);
        if (itr_projectinfo == projectinfo_t.end()) {
            check(false, "non-projectinfo exist");
        } else {
            // check projectinfos' helper
            if ((itr_projectinfo->helper).to_string().empty()) {
                projectinfo_t.modify( itr_projectinfo, _self, [&]( auto& v ) {
                    v.helper = helper;
                });
            } else {
                if (itr_projectinfo->helper != helper) {
                    check(false, "existed helper is diff");        
                }
            }
        }
        
        // move data in projects (scope: created -> selected)
        project_table project_t_selected(_self, SCOPE_STATE_SELECTED);    
        auto itr_selected = project_t_selected.find(project_index);
        if (itr_selected == project_t_selected.end()) {
            project_t_selected.emplace(_self, [&](auto& a) {
                a.index = itr_created->index;
                a.owner = itr_created->owner;
                a.proj_name = itr_created->proj_name;
                a.proj_name_hash = itr_created->proj_name_hash;
                a.selected_detail_index = detail_index;
                a.selected_resource_index = resource_index;
            });  
            
            project_t_created.erase(itr_created);
            
            // TODO BY SOULHAMMER
            // add data in "helpers" table
        } else {
            check(false, "already project was selected");
        }        
    }
}

void factory::start(uint64_t project_index,
                     uint64_t detail_index,
                     uint64_t resource_index,
                     name helper) {
    // need msig by representatives
    require_auth( _self );
    
    // check existence of project (scope: readied)
    project_table project_t_readied(_self, SCOPE_STATE_READIED);
    auto itr_readied = project_t_readied.find(project_index);   
    if (itr_readied == project_t_readied.end()) {
        check(false, "non-project exist");
    } else {
        // check existence of projectinfo
        projectinfo_table projectinfo_t(_self, project_index);
        auto itr_projectinfo = projectinfo_t.find(detail_index);
        if (itr_projectinfo == projectinfo_t.end()) {
            check(false, "non-projectinfo exist");
        } else {
            // check projectinfos' helper
            if ((itr_projectinfo->helper).to_string().empty()) {
                projectinfo_t.modify( itr_projectinfo, _self, [&]( auto& v ) {
                    v.helper = helper;
                });
            } else {
                if (itr_projectinfo->helper != helper) {
                    check(false, "existed helper is diff");        
                }
            }
        }
        
        // move data in projects (scope: readied -> started)
        project_table project_t_started(_self, SCOPE_STATE_STARTED);    
        auto itr_started = project_t_started.find(project_index);
        if (itr_started == project_t_started.end()) {
            project_t_started.emplace(_self, [&](auto& a) {
                a.index = itr_readied->index;
                a.owner = itr_readied->owner;
                a.proj_name = itr_readied->proj_name;
                a.proj_name_hash = itr_readied->proj_name_hash;
                a.selected_detail_index = itr_readied->selected_detail_index;
                a.selected_resource_index = itr_readied->selected_resource_index;                
                a.started_detail_index = detail_index;
                a.started_resource_index = resource_index;
            });  
            
            project_t_readied.erase(itr_readied);
            
            // TODO BY SOULHAMMER
            // add data in "helpers" table            
        } else {
            check(false, "already project was selected");
        }        
    }
}

void factory::pushnotice(uint64_t project_index,
                           name proposer,
                           string proposal,
                           string target) {
    require_auth( proposer );
    
    // TODO BY SOULHAMMER
    // checking existed proposer & proposal
    
    uint64_t selectedScope = target2Scope(target);
    notice_table notice_t(_self, selectedScope);

    notice_t.emplace(proposer, [&](auto& a) {
        a.index = notice_t.available_primary_key();
        a.project_index = project_index;
        a.proposer = proposer;
        a.proposal = proposal;
    });  
}

void factory::popnotice(uint64_t index, 
                        string target) {
    uint64_t selectedScope = target2Scope(target);
    notice_table notice_t(_self, selectedScope);
    auto itr = notice_t.find(index);
    if (itr != notice_t.end()) {
        require_auth(itr->proposer);
        
        notice_t.erase(itr);
    } else {
        check(false, "non-project exist");   
    } 
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
    helper_table helper_t(_self, SCOPE_STATE_SELECTED);
    
    auto itr = helper_t.find(project_index);
    if (itr != helper_t.end()) {
        helper_t.erase(itr);
    }
}

uint64_t factory::getProjectScope(uint64_t project_index) {
    uint64_t result;
    
    project_table project_t_created(_self, SCOPE_STATE_CREATED);
    project_table project_t_selected(_self, SCOPE_STATE_SELECTED);
    project_table project_t_readied(_self, SCOPE_STATE_READIED);
    project_table project_t_started(_self, SCOPE_STATE_STARTED);
    project_table project_t_dropped(_self, SCOPE_STATE_DROPPED);
    
    if (project_t_created.find(project_index) != project_t_created.end()) {
        result = SCOPE_STATE_CREATED;
    } else if (project_t_selected.find(project_index) != project_t_selected.end()) {
        result = SCOPE_STATE_SELECTED;
    } else if (project_t_readied.find(project_index) != project_t_readied.end()) {
        result = SCOPE_STATE_READIED;
    } else if (project_t_started.find(project_index) != project_t_started.end()) {
        result = SCOPE_STATE_STARTED;
    } else if (project_t_dropped.find(project_index) != project_t_dropped.end()) {
        result = SCOPE_STATE_DROPPED;
    } else {
        result = SCOPE_STATE_NONE;
    }
    
    return result;
}

uint64_t factory::target2Scope(string target) {
    uint64_t result;
    
    if (!target.compare(SUGGEST_SELECT)) {
        result = SCOPE_SUGGEST_SELECT;
    } else if (!target.compare(SUGGEST_START)) {
        result = SCOPE_SUGGEST_START;
    } else if (!target.compare(SUGGEST_CANCELREADY)) {
        result = SCOPE_SUGGEST_CANCELREADY;
    } else if (!target.compare(SUGGEST_DROP)) {
        result = SCOPE_SUGGEST_DROP;
    } else {
        check(false, "target is incorrect");
    }
    
    return result;
}

} /// namespace eosio

EOSIO_DISPATCH( eosio::factory, (create)(addinfo)(addresource)(rmresource)(addpayment)(rmpayment)
                                (setready)(cancelready)(drop)(select)(start)(pushnotice)(popnotice) )

