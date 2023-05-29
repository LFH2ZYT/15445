#include "storage/page/page_guard.h"
#include <utility>
#include "buffer/buffer_pool_manager.h"

namespace bustub {
BasicPageGuard::BasicPageGuard(BasicPageGuard &&that) noexcept {
  if (this != &that) {
    is_dirty_ = that.is_dirty_;
    page_ = that.page_;
    bpm_ = that.bpm_;
    that.page_ = nullptr;
    that.bpm_ = nullptr;
    that.is_dirty_ = false;
  }
};

void BasicPageGuard::Drop() {
  if (page_ != nullptr) {
    auto page_id = PageId();
    if (page_id != INVALID_PAGE_ID) {
      bpm_->UnpinPage(page_id, is_dirty_);
    }
  }
  page_ = nullptr;
  is_dirty_ = false;
  bpm_ = nullptr;
}

auto BasicPageGuard::operator=(BasicPageGuard &&that) noexcept -> BasicPageGuard & {
  if (this != &that) {
    Drop();
    is_dirty_ = that.is_dirty_;
    page_ = that.page_;
    bpm_ = that.bpm_;
    that.page_ = nullptr;
    that.bpm_ = nullptr;
    that.is_dirty_ = false;
  }
  return *this;
}

BasicPageGuard::~BasicPageGuard() { Drop(); };  // NOLINT

ReadPageGuard::ReadPageGuard(ReadPageGuard &&that) noexcept = default;

auto ReadPageGuard::operator=(ReadPageGuard &&that) noexcept -> ReadPageGuard & {
  if (this != &that) {
    if (guard_.page_ != nullptr) {
      guard_.page_->RUnlatch();
    }
    guard_ = std::move(that.guard_);
  }
  return *this;
}

void ReadPageGuard::Drop() {
  if (guard_.page_ != nullptr) {
    guard_.page_->RUnlatch();
  }
  guard_.Drop();
}

ReadPageGuard::~ReadPageGuard() {
  if (guard_.page_ != nullptr) {
    Drop();
  }
}  // NOLINT

WritePageGuard::WritePageGuard(WritePageGuard &&that) noexcept = default;

auto WritePageGuard::operator=(WritePageGuard &&that) noexcept -> WritePageGuard & {
  if (this != &that) {
    if (guard_.page_ != nullptr) {
      guard_.page_->WUnlatch();
    }
    guard_ = std::move(that.guard_);
  }
  return *this;
}

void WritePageGuard::Drop() {
  if (guard_.page_ != nullptr) {
    guard_.page_->WUnlatch();
  }
  guard_.Drop();
}

WritePageGuard::~WritePageGuard() {
  if (guard_.page_ != nullptr) {
    Drop();
  }
}  // NOLINT

}  // namespace bustub
