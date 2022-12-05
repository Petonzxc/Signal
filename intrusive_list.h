#pragma once

#include <cstddef>
#include <iterator>

namespace intrusive {
struct default_tag;

// list_base
struct list_base {
  template <typename T, typename Tag>
  friend struct list;

public:
  list_base() = default;

  list_base(const list_base&) = delete;
  list_base& operator=(list_base const& other) = delete;

  list_base(list_base&& other) : list_base() {
    prev = other.prev;
    next = other.next;
    if (other.prev != nullptr) {
      other.prev->next = this;
    }
    if (other.next != nullptr) {
      other.next->prev = this;
    }
    other.prev = other.next = nullptr;
  }

  list_base& operator=(list_base&& other) {
    if (this != &other) {
      list_base(std::move(other)).swap(*this);
    }
    return *this;
  }

  void unlink() {
    if (prev != nullptr && next != nullptr) {
      prev->next = next;
      next->prev = prev;
      next = prev = nullptr;
    }
  }

  bool is_linked() {
    return !(prev == nullptr && next == nullptr);
  }

  void link(list_base* x) {
    next = x;
    x->prev = this;
  }

  ~list_base() {
    unlink();
  }

  void swap(list_base& other) noexcept {
    std::swap(prev, other.prev);
    std::swap(next, other.next);
  }

private:
  list_base* prev{nullptr};
  list_base* next{nullptr};
};

// list_element
template <typename Tag = default_tag>
struct list_element : public list_base {};

// list
template <typename T, typename Tag = default_tag>
struct list {
private:
  template <typename IT>
  class list_iterator {
  public:
    using value_type = IT;
    using reference = value_type&;
    using pointer = value_type*;
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = std::ptrdiff_t;

    list_iterator() = default;
    explicit list_iterator(list_element<Tag>* ptr) : ptr(ptr) {}
    explicit list_iterator(list_base* ptr) : ptr(static_cast<list_element<Tag>*>(ptr)) {}

    template <typename IT_, typename = std::enable_if_t<std::is_const_v<IT> && !std::is_const_v<IT_>>>
    list_iterator(list_iterator<IT_> const& other) : ptr(other.get_ptr()) {}

    list_element<Tag>* get_ptr() const {
      return ptr;
    }

    reference operator*() const {
      return static_cast<reference>(*ptr);
    }

    pointer operator->() const {
      return static_cast<pointer>(ptr);
    }

    list_iterator& operator++() noexcept {
      ptr = static_cast<list_element<Tag>*>(ptr->next);
      return *this;
    }

    list_iterator operator++(int) noexcept {
      list_iterator tmp(*this);
      ++(*this);
      return tmp;
    }

    list_iterator& operator--() noexcept {
      ptr = static_cast<list_element<Tag>*>(ptr->prev);
      return *this;
    }

    list_iterator operator--(int) noexcept {
      list_iterator tmp(*this);
      --(*this);
      return tmp;
    }

    friend bool operator==(const list_iterator& a, const list_iterator& b) {
      return a.ptr == b.ptr;
    }

    friend bool operator!=(const list_iterator& a, const list_iterator& b) {
      return a.ptr != b.ptr;
    }

  private:
    list_element<Tag>* ptr;
  };

public:
  using iterator = list_iterator<T>;
  using const_iterator = list_iterator<const T>;

  list() noexcept : fake_() {
    fake_.next = &fake_;
    fake_.prev = &fake_;
  }

  list(list&& other) noexcept : list() {
    if (!other.empty()) {
      (&fake_)->link(other.fake_.next);
      (other.fake_.prev)->link(&fake_);
      other.fake_.next = other.fake_.prev = &other.fake_;
    }
  }

  list& operator=(list&& other) noexcept {
    if (this != &other) {
      clear();
      list(std::move(other)).swap(*this);
    }
    return *this;
  }

  list(list const& other) = delete;
  list& operator=(list const& other) = delete;

  iterator begin() noexcept {
    return iterator(fake_.next);
  }

  const_iterator begin() const noexcept {
    return const_iterator(const_cast<list_base*>(fake_.next));
  }

  iterator end() noexcept {
    return iterator(&fake_);
  }

  const_iterator end() const noexcept {
    return const_iterator(const_cast<list_element<Tag>*>(&fake_));
  }

  ~list() {
    clear();
  }

  bool empty() const noexcept {
    return &fake_ == fake_.next;
  }

  T& front() noexcept {
    return *begin();
  }

  T const& front() const noexcept {
    return *begin();
  }

  T& back() noexcept {
    return *(--end());
  }

  T const& back() const noexcept {
    return *(--end());
  }

  void push_back(T& x) {
    insert(end(), x);
  }

  void push_front(T& x) {
    insert(begin(), x);
  }
  void pop_front() noexcept {
    erase(begin());
  }

  void pop_back() noexcept {
    erase(--end());
  }

  void clear() noexcept {
    while (!empty()) {
      pop_front();
    }
  }

  iterator erase(const_iterator pos) noexcept {
    iterator tmp = iterator(pos.get_ptr()->next);
    pos.get_ptr()->unlink();
    return tmp;
  }

  iterator insert(const_iterator pos, T& val) {
    list_element<Tag>* tmp = &static_cast<list_element<Tag>&>(val);
    if (pos.get_ptr() != tmp) {
      tmp->unlink();
      pos.get_ptr()->prev->link(tmp);
      tmp->link(pos.get_ptr());
    }
    return iterator(tmp);
  }

  void splice(const_iterator pos, list&, const_iterator first, const_iterator last) noexcept {
    if (first != last) {
      list_element<Tag>* cur = pos.get_ptr();
      auto* prev = static_cast<list_element<Tag>*>(cur->prev);
      auto* pr = static_cast<list_element<Tag>*>(first->prev);
      list_element<Tag>* tr = last.get_ptr();
      prev->link(first.get_ptr());
      static_cast<list_element<Tag>*>(last->prev)->link(cur);
      pr->link(tr);
    }
  }

  void swap(list& other) noexcept {
    std::swap(fake_.next->prev, other.fake_.next->prev);
    std::swap(fake_.next, other.fake_.next);
    std::swap(fake_.prev->next, other.fake_.prev->next);
    std::swap(fake_.prev, other.fake_.prev);
  }

  const_iterator get_iterator(T& val) {
    return const_iterator(static_cast<list_element<Tag>*>(&val));
  }

private:
  list_element<Tag> fake_;
};
} // namespace intrusive