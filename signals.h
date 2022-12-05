#pragma once
#include "intrusive_list.h"
#include <functional>

namespace signals {

template <typename T>
struct signal;

struct connection_tag {};

template <typename... Args>
struct signal<void(Args...)> {
  using slot = std::function<void(Args...)>;

  struct connection : intrusive::list_element<connection_tag> {
    connection() = default;

    connection(connection&& other) noexcept {
      replace_connection(std::move(other));
    }

    connection& operator=(connection&& other) noexcept {
      if (this != &other) {
        replace_connection(std::move(other));
      }
      return *this;
    }

    void disconnect() {
      if (is_linked()) {
        unlink();
        replace_connections_by_iterator(this);
        sig = nullptr;
      }
    }

    void operator()(Args... args) const {
      func(std::forward<Args>(args)...);
    }

    ~connection() {
      disconnect();
    }

  private:
    void replace_connections_by_iterator(const connection* it_replace) {
      for (auto it = sig->tail; it != nullptr; it = it->prev) {
        if (&*it->current == it_replace) {
          it->current = sig->connections.get_iterator(*this);
        }
      }
    }

    void replace_connection(connection&& other) {
      func = std::move(other.func);
      sig = other.sig;

      if (other.is_linked()) {
        auto other_it = sig->connections.get_iterator(other);
        sig->connections.insert(other_it, *this);
        replace_connections_by_iterator(&*other_it);
        sig->connections.erase(other_it);
      }
    }

    connection(signal* sig_, slot func_) : sig(sig_), func(std::move(func_)) {
      sig->connections.push_front(*this);
    }

    friend struct signal;

    signal* sig;
    mutable slot func;
  };

  using connections_list = intrusive::list<connection, connection_tag>;

  signal() = default;

  signal(signal const&) = delete;
  signal& operator=(signal const&) = delete;

  ~signal() {
    for (iterator_holder* it = tail; it != nullptr; it = it->prev) {
      it->sig = nullptr;
      (*it->current).func = {};
    }
  }

  connection connect(slot func) noexcept {
    return connection(this, std::move(func));
  }

  void operator()(Args... args) const {
    iterator_holder holder(this);
    while (holder.current != connections.end()) {
      auto copy = holder.current;
      holder.current++;
      (*copy)(std::forward<Args>(args)...);
      if (!holder.sig) {
        return;
      }
    }
  }

private:
  struct iterator_holder {
    explicit iterator_holder(const signal* sig_) : sig(sig_), current(sig_->connections.begin()), prev(sig_->tail) {
      sig->tail = this;
    }

    ~iterator_holder() {
      if (sig) {
        sig->tail = sig->tail->prev;
      }
    }

    const signal* sig;
    typename connections_list::const_iterator current;
    iterator_holder* prev;
  };

  connections_list connections;
  mutable iterator_holder* tail = nullptr;
};

} // namespace signals
