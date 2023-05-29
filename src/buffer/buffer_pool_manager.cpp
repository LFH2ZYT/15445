//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// buffer_pool_manager.cpp
//
// Identification: src/buffer/buffer_pool_manager.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/buffer_pool_manager.h"
#include <iostream>

#include "common/exception.h"
#include "common/macros.h"
#include "storage/page/page.h"
#include "storage/page/page_guard.h"

namespace bustub {

BufferPoolManager::BufferPoolManager(size_t pool_size, DiskManager *disk_manager, size_t replacer_k,
                                     LogManager *log_manager)
    : pool_size_(pool_size), disk_manager_(disk_manager), log_manager_(log_manager) {
  // TODO(students): remove this line after you have implemented the buffer pool manager

  // we allocate a consecutive memory space for the buffer pool
  pages_ = new Page[pool_size_];
  replacer_ = std::make_unique<LRUKReplacer>(pool_size, replacer_k);

  // Initially, every page is in the free list.
  for (size_t i = 0; i < pool_size_; ++i) {
    free_list_.emplace_back(static_cast<int>(i));
  }
}

BufferPoolManager::~BufferPoolManager() { delete[] pages_; }

auto BufferPoolManager::NewPage(page_id_t *page_id) -> Page * {
  frame_id_t new_frame_id;

  latch_.lock();

  if (!free_list_.empty()) {
    new_frame_id = free_list_.front();
    free_list_.pop_front();

  } else if (replacer_->Evict(&new_frame_id)) {
    if (pages_[new_frame_id].IsDirty()) {
      disk_manager_->WritePage(pages_[new_frame_id].GetPageId(), pages_[new_frame_id].GetData());
      pages_[new_frame_id].is_dirty_ = false;
    }
    page_table_.erase(pages_[new_frame_id].page_id_);
    pages_[new_frame_id].ResetMemory();
  } else {
    page_id = nullptr;
    latch_.unlock();
    return nullptr;
  }

  *page_id = AllocatePage();
  page_table_[*page_id] = new_frame_id;
  pages_[new_frame_id].page_id_ = *page_id;
  pages_[new_frame_id].pin_count_++;             // pin count++
  replacer_->RecordAccess(new_frame_id);         // 相当于第一次访问当前页
  replacer_->SetEvictable(new_frame_id, false);  // 不能被替换

  latch_.unlock();
  return &pages_[new_frame_id];
}

auto BufferPoolManager::FetchPage(page_id_t page_id, [[maybe_unused]] AccessType access_type) -> Page * {
  latch_.lock();
  auto it = page_table_.find(page_id);
  frame_id_t new_frame_id;
  if (it == page_table_.end()) {
    if (!free_list_.empty()) {
      new_frame_id = free_list_.front();
      free_list_.pop_front();
    } else if (replacer_->Evict(&new_frame_id)) {
      if (pages_[new_frame_id].IsDirty()) {
        disk_manager_->WritePage(pages_[new_frame_id].GetPageId(), pages_[new_frame_id].GetData());
        pages_[new_frame_id].is_dirty_ = false;
      }
      // pages_[new_frame_id].ResetMemory();
      page_table_.erase(pages_[new_frame_id].page_id_);
      // //  std::cout<<"FetchPage ResetMemory";
    } else {
      latch_.unlock();
      return nullptr;
    }
    page_table_[page_id] = new_frame_id;
    pages_[new_frame_id].page_id_ = page_id;
    disk_manager_->ReadPage(page_id, pages_[new_frame_id].data_);
  } else {
    new_frame_id = it->second;
  }
  replacer_->RecordAccess(new_frame_id);         // 相当于第一次访问当前页
  replacer_->SetEvictable(new_frame_id, false);  // 不能被替换
  pages_[new_frame_id].pin_count_++;             // pin count++
  latch_.unlock();
  return &pages_[new_frame_id];
}

auto BufferPoolManager::UnpinPage(page_id_t page_id, bool is_dirty, [[maybe_unused]] AccessType access_type) -> bool {
  latch_.lock();
  auto it = page_table_.find(page_id);
  if (it == page_table_.end()) {
    latch_.unlock();
    return false;
  }
  if (pages_[it->second].GetPinCount() == 0) {
    latch_.unlock();
    return false;
  }
  if (is_dirty) {
    pages_[it->second].is_dirty_ = true;
  }
  pages_[it->second].pin_count_--;
  if (pages_[it->second].GetPinCount() == 0) {
    replacer_->SetEvictable(it->second, true);
  }
  latch_.unlock();
  return true;
}

auto BufferPoolManager::FlushPage(page_id_t page_id) -> bool {
  latch_.lock();
  auto it = page_table_.find(page_id);
  if (it == page_table_.end()) {
    return false;
  }
  if (pages_[it->second].is_dirty_) {
    pages_[it->second].is_dirty_ = false;
    disk_manager_->WritePage(pages_[it->second].GetPageId(), pages_[it->second].GetData());
  }
  latch_.unlock();
  return true;
}

void BufferPoolManager::FlushAllPages() {
  latch_.lock();
  for (auto it : page_table_) {
    if (pages_[it.second].is_dirty_) {
      pages_[it.second].is_dirty_ = false;
      disk_manager_->WritePage(pages_[it.second].GetPageId(), pages_[it.second].GetData());
    }
  }
  latch_.unlock();
}

auto BufferPoolManager::DeletePage(page_id_t page_id) -> bool {
  latch_.lock();
  auto it = page_table_.find(page_id);
  if (it == page_table_.end()) {
    DeallocatePage(page_id);
    latch_.unlock();
    return true;
  }
  if (pages_[it->second].GetPinCount() != 0) {
    latch_.unlock();
    return false;
  }
  replacer_->Remove(it->second);
  if (pages_[it->second].is_dirty_) {
    pages_[it->second].is_dirty_ = false;
    disk_manager_->WritePage(pages_[it->second].GetPageId(), pages_[it->second].GetData());
  }
  pages_[it->second].page_id_ = INVALID_PAGE_ID;
  pages_[it->second].ResetMemory();  // reset memory
  free_list_.push_back(it->second);
  page_table_.erase(page_id);
  DeallocatePage(page_id);
  latch_.unlock();
  return true;
}

auto BufferPoolManager::AllocatePage() -> page_id_t {
  page_id_t page_id = next_page_id_;
  next_page_id_++;
  return page_id;
}

auto BufferPoolManager::FetchPageBasic(page_id_t page_id) -> BasicPageGuard {
  return BasicPageGuard{this, FetchPage(page_id)};
}

auto BufferPoolManager::FetchPageRead(page_id_t page_id) -> ReadPageGuard {
  Page *page = FetchPage(page_id);
  if (page == nullptr) {
    return ReadPageGuard{this, nullptr};
  }
  page->RLatch();
  return ReadPageGuard{this, page};
}

auto BufferPoolManager::FetchPageWrite(page_id_t page_id) -> WritePageGuard {
  Page *page = FetchPage(page_id);
  if (page == nullptr) {
    return WritePageGuard{this, nullptr};
  }
  page->WLatch();
  return WritePageGuard{this, page};
}

auto BufferPoolManager::NewPageGuarded(page_id_t *page_id) -> BasicPageGuard {
  return BasicPageGuard{this, NewPage(page_id)};
}

}  // namespace bustub
