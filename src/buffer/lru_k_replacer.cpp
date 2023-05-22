//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// lru_k_replacer.cpp
//
// Identification: src/buffer/lru_k_replacer.cpp
//
// Copyright (c) 2015-2022, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/lru_k_replacer.h"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <list>
#include "common/config.h"
#include "common/exception.h"
#include "fmt/ranges.h"
namespace bustub {


 auto InsertOrder(std::list<LRUKNode>& over_k_nodes, LRUKNode& node)->std::list<LRUKNode>::iterator
 {
    size_t this_min_timestap = node.min_history_size_;
    for (auto it = over_k_nodes.begin(); it != over_k_nodes.end(); ++it) {
        if (this_min_timestap < it->min_history_size_) {
            return over_k_nodes.insert(it, node);
        }
    }
    return over_k_nodes.insert(over_k_nodes.end(),node);
}

LRUKReplacer::LRUKReplacer(size_t num_frames, size_t k) : replacer_size_(num_frames), k_(k) {}

auto LRUKReplacer::Evict(frame_id_t *frame_id) -> bool { 
    for (auto it = less_k_nodes_.begin(); it != less_k_nodes_.end();it++) {
        if (it->is_evictable_) {
            *frame_id = it->fid_;
            
            curr_size_--;
            evictable_size_--;
            hash_cache_.erase(it->fid_);
            less_k_nodes_.erase(it);
            return true;
        }
    }
    for (auto it = over_k_nodes_.begin(); it != over_k_nodes_.end(); it++) {
        if (it->is_evictable_) {
            *frame_id = it->fid_;
            curr_size_--;
            evictable_size_--;
            hash_cache_.erase(it->fid_);
            over_k_nodes_.erase(it);
            return true;
        }
    }
    return false;
}

void LRUKReplacer::RecordAccess(frame_id_t frame_id, [[maybe_unused]] AccessType access_type) {
    if (frame_id > static_cast<int32_t>(replacer_size_)) {
        throw bustub::Exception(fmt::format("RecordAccess frame id: {}  > replacer_size_{}", frame_id,replacer_size_));
    }
    current_timestamp_++;
    if (hash_cache_.find(frame_id) == hash_cache_.end()) {
        LRUKNode node;
        node.fid_ = frame_id;
        node.history_.push_front(current_timestamp_);
        auto it = less_k_nodes_.insert(less_k_nodes_.end(),node);
        hash_cache_[frame_id] = it;
        curr_size_++;
        return;
    }
    auto thisnode = hash_cache_[frame_id];
    
    if (thisnode->over_k_) {
        thisnode->history_.push_front(current_timestamp_);
        thisnode->history_.pop_back();
        thisnode->min_history_size_ = thisnode->history_.back();
        auto node = *thisnode;
        over_k_nodes_.erase(thisnode);
        auto new_it = InsertOrder(over_k_nodes_, node);
        // for(auto &it : over_k_nodes_){
        //     std::cout<<it.fid_<<"  |  ";
        // }
        // std::cout<<std::endl;
        hash_cache_[frame_id] = new_it;
    }else{
        if (thisnode->history_.size() == k_-1) {
            thisnode->history_.push_front(current_timestamp_);
            thisnode->over_k_ = true;
            thisnode->min_history_size_ = thisnode->history_.back();
            LRUKNode node = *thisnode;
            less_k_nodes_.erase(thisnode);
            auto new_it = InsertOrder(over_k_nodes_, node);
            hash_cache_[frame_id] = new_it;
           
        }else {
            thisnode->history_.push_front(current_timestamp_);
        }
    }

}

void LRUKReplacer::SetEvictable(frame_id_t frame_id, bool set_evictable) {
    // if (frame_id > static_cast<int32_t>(replacer_size_)) {
    //     throw bustub::Exception(fmt::format("SetEvictable frame id: {}  > replacer_size_{}", frame_id,replacer_size_));
    // }
    if (hash_cache_.find(frame_id) == hash_cache_.end()) {
        throw bustub::Exception(fmt::format("SetEvictable frame id: {} is invalid", frame_id));
    }
    auto thisnode = hash_cache_[frame_id];
    
    if(thisnode->is_evictable_ && !set_evictable){
        evictable_size_--;
    }else if (!thisnode->is_evictable_ && set_evictable) {
        evictable_size_++;
    }
    thisnode->is_evictable_ = set_evictable;
    
}

void LRUKReplacer::Remove(frame_id_t frame_id) {
    // if (frame_id > static_cast<int32_t>(replacer_size_)) {
    //     throw bustub::Exception(fmt::format("Remove frame id: {}  > replacer_size_{}", frame_id,replacer_size_));
    // }
    if (hash_cache_.find(frame_id) == hash_cache_.end()) {
        throw bustub::Exception(fmt::format("Remove frame id: {} is invalid", frame_id));
    }
    auto thisnode = hash_cache_[frame_id];
    if(!thisnode->is_evictable_){
         throw bustub::Exception(fmt::format("Remove frame id: {} is not evictable", frame_id));
    }
    if(thisnode->over_k_){
        over_k_nodes_.erase(thisnode);
    }else {
        less_k_nodes_.erase(thisnode);
    }
    hash_cache_.erase(frame_id);
    evictable_size_--;
    curr_size_--;
}

auto LRUKReplacer::Size() -> size_t { return evictable_size_; }

}  // namespace bustub
