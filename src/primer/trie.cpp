#include "primer/trie.h"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string_view>
#include "common/exception.h"
#include<stack>

namespace bustub {

template <class T>
auto Trie::Get(std::string_view key) const -> const T * {
  // throw NotImplementedException("Trie::Get is not implemented.");

  // You should walk through the trie to find the node corresponding to the key. If the node doesn't exist, return
  // nullptr. After you find the node, you should use `dynamic_cast` to cast it to `const TrieNodeWithValue<T> *`. If
  // dynamic_cast returns `nullptr`, it means the type of the value is mismatched, and you should return nullptr.
  // Otherwise, return the value.
  auto node_cur = root_;
  for(auto key_cur : key){
    auto  it = node_cur->children_.find(key_cur);
    if(it==node_cur->children_.end()){
      return nullptr;
    }
    node_cur = it->second;
  }
  auto node_with_value = dynamic_cast<const TrieNodeWithValue<T> *>(node_cur.get());
  if(node_with_value!=nullptr){
    return node_with_value->value_.get();
  }
  return nullptr;
}

template <class T>
auto Trie::Put(std::string_view key, T value) const -> Trie {
  // Note that `T` might be a non-copyable type. Always use `std::move` when creating `shared_ptr` on that value.
  // throw NotImplementedException("Trie::Put is not implemented.");
  // You should walk through the trie and create new nodes if necessary. If the node corresponding to the key already
  // exists, you should create a new `TrieNodeWithValue`.
  //构建新的Trie
  std::shared_ptr<T> val  = std::make_shared<T>(std::move(value));
  std::shared_ptr<Trie> new_trie;
  std::shared_ptr<TrieNode> new_root;
  if (root_ == nullptr) {
    new_root = std::make_shared<TrieNode>();
  }else{
    new_root = std::shared_ptr<TrieNode>(this->root_->Clone());
  }
  
  new_trie = std::make_shared<Trie>();
  new_trie->root_ = new_root;
  if (key.empty()) {
    std::shared_ptr<TrieNodeWithValue<T>> new_root = std::make_shared<TrieNodeWithValue<T>>(new_trie->root_->children_,val);
    new_trie->root_ = new_root;
    return *new_trie;
  }
  //遍历到最后一级目录
  auto node_cur = new_root;
  for( uint64_t i=0; i<key.size()-1; i++){
    char key_cur = key[i];
    auto  it = node_cur->children_.find(key_cur);
    if(it == node_cur->children_.end()){
      //之前不存在的路径节点，需要自己创建
      auto new_node   = std::make_shared<TrieNode>();
      node_cur->children_[key_cur] = new_node;
      node_cur = new_node;
    }else{
      //之前存在的路径节点，需要clone成新的节点
      std::shared_ptr<TrieNode> new_node = std::shared_ptr<TrieNode>(it->second->Clone());
      node_cur->children_[key_cur] = new_node;
      node_cur = new_node;
    }
  }

  //遍历到了最后一级目录
  char last_char =key[key.length()-1];
  auto  it = node_cur->children_.find(last_char);
  if(it == node_cur->children_.end()){
      //之前不存在的路径节点，需要自己创建
      std::shared_ptr<TrieNodeWithValue<T>> new_node   = std::make_shared<TrieNodeWithValue<T>>(val);
      node_cur->children_[last_char] = new_node;
  }else{
    //之前存在的路径节点，需要clone成新的节点
    std::shared_ptr<TrieNodeWithValue<T>> new_node = std::make_shared<TrieNodeWithValue<T>>(it->second->children_,val);
    node_cur->children_[last_char] = new_node;
  }
  return *new_trie;
  
}

auto Trie::Remove(std::string_view key) const -> Trie {
  // throw NotImplementedException("Trie::Remove is not implemented.");

  // You should walk through the trie and remove nodes if necessary. If the node doesn't contain a value any more,
  // you should convert it to `TrieNode`. If a node doesn't have children any more, you should remove it.
  
  //创建栈记录遍历节点的路径指针
  // auto stack_path_nodes = std::make_unique<std::stack<std::shared_ptr<TrieNode>>>(); 
  // auto stack_path_char = std::make_unique<std::stack<char>>();
  //构建新的Trie
  std::shared_ptr<TrieNode> new_root = std::shared_ptr<TrieNode>(root_->Clone());
  std::shared_ptr<Trie> new_trie = std::make_shared<Trie>();
  new_trie->root_ = new_root;
  
  //遍历到最后一级目录
  auto node_cur = new_root;
  // stack_path_nodes->push(node_cur);//路径节点指针压栈
  for( uint64_t i=0; i<key.size()-1; i++){
    char key_cur = key[i];
    auto  it = node_cur->children_.find(key_cur);
    if(it == node_cur->children_.end()){
      //不存在的路径节点，不需要删除
      return *new_trie;
    }
    //之前存在的路径节点，需要clone成新的节点
    std::shared_ptr<TrieNode> new_node = std::shared_ptr<TrieNode>(it->second->Clone());
    node_cur->children_[key_cur] = new_node;
    node_cur = new_node;
    // stack_path_char->push(key_cur);//路径字符压栈
    // stack_path_nodes->push(node_cur);//路径节点指针压栈
   
  }

  //遍历到了最后一级目录
  char last_char =key[key.length()-1];
  auto  it = node_cur->children_.find(last_char);
  
  if(it == node_cur->children_.end()){
      //不存在的路径节点，不需要删除
      return *new_trie;  
  }
  //把带value的节点转化为不带value 的节点
  std::shared_ptr<TrieNode> new_node = std::make_shared<TrieNode>(it->second->children_);
  node_cur->children_[last_char] = new_node;
  // stack_path_char->push(last_char);//路径字符压栈
  // stack_path_nodes->push(new_node);//路径节点指针压栈
  // //栈回溯，删除没有值和孩子的节点
  // auto this_node = stack_path_nodes->top();
  // stack_path_nodes->pop();
  // while (!stack_path_nodes->empty() && this_node->children_.empty() && !this_node->is_value_node_) {
  //   auto this_c = stack_path_char->top();
  //   stack_path_char->pop();
  //   auto parent_node = stack_path_nodes->top();
  //   stack_path_nodes->pop();
  //   parent_node->children_.erase(this_c);
  // }
  return *new_trie;
}

// Below are explicit instantiation of template functions.
//
// Generally people would write the implementation of template classes and functions in the header file. However, we
// separate the implementation into a .cpp file to make things clearer. In order to make the compiler know the
// implementation of the template functions, we need to explicitly instantiate them here, so that they can be picked up
// by the linker.

template auto Trie::Put(std::string_view key, uint32_t value) const -> Trie;
template auto Trie::Get(std::string_view key) const -> const uint32_t *;

template auto Trie::Put(std::string_view key, uint64_t value) const -> Trie;
template auto Trie::Get(std::string_view key) const -> const uint64_t *;

template auto Trie::Put(std::string_view key, std::string value) const -> Trie;
template auto Trie::Get(std::string_view key) const -> const std::string *;

// If your solution cannot compile for non-copy tests, you can remove the below lines to get partial score.

using Integer = std::unique_ptr<uint32_t>;

template auto Trie::Put(std::string_view key, Integer value) const -> Trie;
template auto Trie::Get(std::string_view key) const -> const Integer *;

template auto Trie::Put(std::string_view key, MoveBlocked value) const -> Trie;
template auto Trie::Get(std::string_view key) const -> const MoveBlocked *;

}  // namespace bustub
