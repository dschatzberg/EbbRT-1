//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef COMMON_SRC_INCLUDE_EBBRT_FUTURE_H_
#define COMMON_SRC_INCLUDE_EBBRT_FUTURE_H_

#include <atomic>
#include <exception>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

#include <ebbrt/EventManager.h>
#include <ebbrt/ExplicitlyConstructed.h>
#include <ebbrt/MoveLambda.h>

namespace ebbrt {
template <typename Res> class Future;
template <typename Res> class SharedFuture;
template <typename Res> class Promise;

enum class Launch { Sync, Async };

struct UnreadyFutureError : public std::logic_error {
  explicit UnreadyFutureError(const std::string& what_arg)
      : std::logic_error{what_arg} {}
  explicit UnreadyFutureError(const char* what_arg)
      : std::logic_error{what_arg} {}
};

template <typename T, typename... Args>
Future<T> MakeReadyFuture(Args&&... args);

template <typename T> Future<T> MakeFailedFuture(std::exception_ptr);

template <typename T> struct Flatten {
  typedef T type;
};

template <typename T> struct Flatten<Future<T>> {
  typedef typename Flatten<T>::type type;
};

template <typename F, typename... Args>
Future<typename Flatten<typename std::result_of<F(Args...)>::type>::type>
Async(F&& f, Args&&... args);

namespace __future_detail {
struct MakeReadyFutureTag {};

template <typename Res> class State {
  Res val_;
  std::exception_ptr eptr_;
  MovableFunction<void()> func_;
  std::atomic_bool ready_;
  std::mutex mutex_;

 public:
  State();

  explicit State(std::exception_ptr eptr);

  template <typename... Args> State(MakeReadyFutureTag tag, Args&&... args);

  template <typename F, typename R>
  Future<typename Flatten<typename std::result_of<F(R)>::type>::type>
  Then(Launch policy, F&& func, R fut);

  void SetValue(const Res& res);
  void SetValue(Res&& res);

  void SetException(std::exception_ptr eptr);

  Res& Get();

  bool Ready() const;

 private:
  // Non void return then
  template <typename F, typename R>
  typename std::enable_if<
      !std::is_void<typename std::result_of<F(R)>::type>::value,
      Future<typename Flatten<typename std::result_of<F(R)>::type>::type>>::type
  ThenHelp(Launch policy, F&& func, R fut);

  // void return then
  template <typename F, typename R>
  typename std::enable_if<
      std::is_void<typename std::result_of<F(R)>::type>::value,
      Future<typename Flatten<typename std::result_of<F(R)>::type>::type>>::type
  ThenHelp(Launch policy, F&& func, R fut);
};

template <> class State<void> {
  std::exception_ptr eptr_;
  MovableFunction<void()> func_;
  std::atomic_bool ready_;
  std::mutex mutex_;

 public:
  State();

  explicit State(std::exception_ptr eptr);

  explicit State(MakeReadyFutureTag tag);

  template <typename F, typename R>
  Future<typename Flatten<typename std::result_of<F(R)>::type>::type>
  Then(Launch policy, F&& func, R fut);

  void SetValue();

  void SetException(std::exception_ptr eptr);

  void Get();

  bool Ready() const;

 private:
  // Non void return then
  template <typename F, typename R>
  typename std::enable_if<
      !std::is_void<typename std::result_of<F(R)>::type>::value,
      Future<typename Flatten<typename std::result_of<F(R)>::type>::type>>::type
  ThenHelp(Launch policy, F&& func, R fut);

  // void return then
  template <typename F, typename R>
  typename std::enable_if<
      std::is_void<typename std::result_of<F(R)>::type>::value,
      Future<typename Flatten<typename std::result_of<F(R)>::type>::type>>::type
  ThenHelp(Launch policy, F&& func, R fut);
};
}  // namespace __future_detail

template <typename Res> class Future {
  typedef __future_detail::State<Res> State;

  std::shared_ptr<State> state_;

 public:
  Future() = default;

  Future(const Future&) = delete;
  Future& operator=(const Future&) = delete;

  Future(Future&&) = default;
  Future& operator=(Future&&) = default;

  SharedFuture<Res> Share();

  template <typename F>
  Future<typename Flatten<typename std::result_of<F(Future<Res>)>::type>::type>
  Then(Launch policy, F&& func);

  template <typename F>
  Future<typename Flatten<typename std::result_of<F(Future<Res>)>::type>::type>
  Then(F&& func);

  bool Valid() const;

  Res& Get();

  bool Ready() const;

  Future<Res> Block();

 private:
  friend class Promise<Res>;
  template <typename T, typename... Args>
  friend Future<T> MakeReadyFuture(Args&&... args);

  template <typename T> friend Future<T> MakeFailedFuture(std::exception_ptr);

  explicit Future(const std::shared_ptr<State>&);

  template <typename... Args>
  explicit Future(__future_detail::MakeReadyFutureTag tag, Args&&... args);

  explicit Future(std::exception_ptr);
};

template <> class Future<void> {
  typedef __future_detail::State<void> State;

  std::shared_ptr<State> state_;

 public:
  Future() = default;

  Future(const Future&) = delete;
  Future& operator=(const Future&) = delete;

  Future(Future&&) = default;
  Future& operator=(Future&&) = default;

  SharedFuture<void> Share();

  template <typename F>
  Future<typename Flatten<typename std::result_of<F(Future<void>)>::type>::type>
  Then(Launch policy, F&& func);

  template <typename F>
  Future<typename Flatten<typename std::result_of<F(Future<void>)>::type>::type>
  Then(F&& func);

  bool Valid() const;

  void Get();

  bool Ready() const;

  Future<void> Block();

 private:
  friend class Promise<void>;
  template <typename T, typename... Args>
  friend Future<T> MakeReadyFuture(Args&&... args);

  template <typename T> friend Future<T> MakeFailedFuture(std::exception_ptr);

  explicit Future(const std::shared_ptr<State>&);

  explicit Future(__future_detail::MakeReadyFutureTag tag);

  explicit Future(std::exception_ptr);
};

template <typename Res> class SharedFuture {
  typedef __future_detail::State<Res> State;

  std::shared_ptr<State> state_;

 public:
  SharedFuture() = default;

  SharedFuture(const SharedFuture&) = default;
  SharedFuture& operator=(const SharedFuture&) = default;

  SharedFuture(SharedFuture&&) = default;
  SharedFuture& operator=(SharedFuture&&) = default;

  template <typename F>
  Future<typename Flatten<
      typename std::result_of<F(SharedFuture<Res>)>::type>::type>
  Then(Launch policy, F&& func);

  template <typename F>
  Future<typename Flatten<
      typename std::result_of<F(SharedFuture<Res>)>::type>::type>
  Then(F&& func);

  bool Valid() const;

  Res& Get();

  bool Ready() const;

  void Block();

 private:
  explicit SharedFuture(const std::shared_ptr<State>&);

  friend class Future<Res>;
};

template <> class SharedFuture<void> {
  typedef __future_detail::State<void> State;

  std::shared_ptr<State> state_;

 public:
  SharedFuture() = default;

  SharedFuture(const SharedFuture&) = default;
  SharedFuture& operator=(const SharedFuture&) = default;

  SharedFuture(SharedFuture&&) = default;
  SharedFuture& operator=(SharedFuture&&) = default;

  template <typename F>
  Future<typename Flatten<
      typename std::result_of<F(SharedFuture<void>)>::type>::type>
  Then(Launch policy, F&& func);

  template <typename F>
  Future<typename Flatten<
      typename std::result_of<F(SharedFuture<void>)>::type>::type>
  Then(F&& func);

  bool Valid() const;

  void Get();

  bool Ready() const;

  void Block();

 private:
  friend class Future<void>;
  explicit SharedFuture(const std::shared_ptr<State>&);
};

template <typename Res> class Promise {
  typedef __future_detail::State<Res> State;

  std::shared_ptr<State> state_;

 public:
  Promise();

  Promise(const Promise&) = delete;
  Promise& operator=(const Promise&) = delete;

  Promise(Promise&&) = default;
  Promise& operator=(Promise&&) = default;

  void SetValue(const Res&);

  void SetValue(Res&&);

  void SetException(std::exception_ptr);

  Future<Res> GetFuture();
};

template <> class Promise<void> {
  typedef __future_detail::State<void> State;

  std::shared_ptr<State> state_;

 public:
  Promise();

  Promise(const Promise&) = delete;
  Promise& operator=(const Promise&) = delete;

  Promise(Promise&&) = default;
  Promise& operator=(Promise&&) = default;

  void SetValue();

  void SetException(std::exception_ptr);

  Future<void> GetFuture();
};

template <typename T, typename... Args>
Future<T> MakeReadyFuture(Args&&... args) {
  return Future<T>{typename __future_detail::MakeReadyFutureTag(),
                   std::forward<Args>(args)...};
}

template <typename T> Future<T> MakeFailedFuture(std::exception_ptr eptr) {
  return Future<T>{std::move(eptr)};
}

template <typename T>
Future<std::vector<T>> when_all(std::vector<Future<T>>& vec) {
  if (vec.empty()) {
    return MakeReadyFuture<std::vector<T>>();
  }

  auto retvec = std::make_shared<std::vector<T>>(vec.size());
  auto count = std::make_shared<std::atomic_size_t>(vec.size());
  auto promise = std::make_shared<Promise<std::vector<T>>>();
  auto ret = promise->GetFuture();
  for (int i = 0; i < vec.size(); ++i) {
    vec[i].Then([=](Future<T> val) {
      try {
        (*retvec)[i] = std::move(val.Get());
      }
      catch (...) {
        promise->SetException(std::current_exception());
        return;
      }
      if (count->fetch_sub(1) == 1) {
        // we are the last to fulfill
        promise->SetValue(std::move(*retvec));
      }
    });
  }
  return std::move(ret);
}

template <typename Res>
Future<typename Flatten<Res>::type> flatten(Future<Res> fut) {
  return std::move(fut);
}

template <typename Res>
Future<typename Flatten<Res>::type> flatten(Future<Future<Res>> fut) {
  auto p = Promise<Res>();
  auto ret = p.GetFuture();
  fut.Then(MoveBind([](Promise<Res> prom, Future<Future<Res>> fut) {
                      Future<Res> inner;
                      try {
                        inner = std::move(fut.Get());
                      }
                      catch (...) {
                        prom.SetException(std::current_exception());
                        return;
                      }

                      inner.Then(
                          MoveBind([](Promise<Res> prom, Future<Res> fut) {
                                     try {
                                       prom.SetValue(std::move(fut.Get()));
                                     }
                                     catch (...) {
                                       prom.SetException(
                                           std::current_exception());
                                     }
                                   },
                                   std::move(prom)));
                    },
                    std::move(p)));
  return flatten(std::move(ret));
}

inline Future<void> flatten(Future<Future<void>> fut) {
  auto p = Promise<void>();
  auto ret = p.GetFuture();
  fut.Then(MoveBind([](Promise<void> prom, Future<Future<void>> fut) {
                      Future<void> inner;
                      try {
                        inner = std::move(fut.Get());
                      }
                      catch (...) {
                        prom.SetException(std::current_exception());
                        return;
                      }

                      inner.Then(
                          MoveBind([](Promise<void> prom, Future<void> fut) {
                                     try {
                                       fut.Get();
                                       prom.SetValue();
                                     }
                                     catch (...) {
                                       prom.SetException(
                                           std::current_exception());
                                     }
                                   },
                                   std::move(prom)));
                    },
                    std::move(p)));
  return ret;
}

// Non void return async implementation
template <typename F, typename... Args>
typename std::enable_if<
    !std::is_void<typename std::result_of<F(Args...)>::type>::value,
    Future<typename Flatten<typename std::result_of<F(Args...)>::type>::type>>::
    type
    AsyncHelper(F&& f, Args&&... args) {
  typedef typename std::result_of<F(Args...)>::type result_type;
  typedef decltype(std::bind(std::forward<F>(f), std::forward<Args>(args)...))
      bound_fn_type;
  auto p = Promise<result_type>();
  auto ret = p.GetFuture();
  auto bound_f = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
  ebbrt::event_manager->Spawn(
      MoveBind([](Promise<result_type> prom, bound_fn_type fn) {
                 try {
                   prom.SetValue(fn());
                 }
                 catch (...) {
                   prom.SetException(std::current_exception());
                 }
               },
               std::move(p), std::move(bound_f)));
  return flatten(std::move(ret));
}

// void return async implementation
template <typename F, typename... Args>
typename std::enable_if<
    std::is_void<typename std::result_of<F(Args...)>::type>::value,
    Future<typename Flatten<typename std::result_of<F(Args...)>::type>::type>>::
    type
    AsyncHelper(F&& f, Args&&... args) {
  typedef decltype(std::bind(std::forward<F>(f), std::forward<Args>(args)...))
      bound_fn_type;
  auto p = Promise<void>();
  auto ret = p.GetFuture();
  auto bound_f = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
  ebbrt::event_manager->Spawn(
      MoveBind([](Promise<void> prom, bound_fn_type fn) {
                 try {
                   fn();
                   prom.SetValue();
                 }
                 catch (...) {
                   prom.SetException(std::current_exception());
                 }
               },
               std::move(p), std::move(bound_f)));
  return flatten(std::move(ret));
}

template <typename F, typename... Args>
Future<typename Flatten<typename std::result_of<F(Args...)>::type>::type>
Async(F&& f, Args&&... args) {
  return AsyncHelper(std::forward<F>(f), std::forward<Args>(args)...);
}

template <typename Res>
Future<Res>::Future(const std::shared_ptr<__future_detail::State<Res>>& ptr)
    : state_{ptr} {}

inline Future<void>::Future(
    const std::shared_ptr<__future_detail::State<void>>& ptr)
    : state_{ptr} {}

template <typename Res>
template <typename... Args>
Future<Res>::Future(__future_detail::MakeReadyFutureTag tag, Args&&... args)
    : state_{std::make_shared<State>(tag, std::forward<Args>(args)...)} {}

inline Future<void>::Future(__future_detail::MakeReadyFutureTag tag)
    : state_{std::make_shared<State>(tag)} {}

template <typename Res>
Future<Res>::Future(std::exception_ptr eptr)
    : state_{std::make_shared<State>(std::move(eptr))} {}

inline Future<void>::Future(std::exception_ptr eptr)
    : state_{std::make_shared<State>(std::move(eptr))} {}

template <typename Res> SharedFuture<Res> Future<Res>::Share() {
  return SharedFuture<Res>(std::move(state_));
}

inline SharedFuture<void> Future<void>::Share() {
  return SharedFuture<void>(std::move(state_));
}

template <typename Res>
SharedFuture<Res>::SharedFuture(
    const std::shared_ptr<__future_detail::State<Res>>& ptr)
    : state_{ptr} {}

inline SharedFuture<void>::SharedFuture(
    const std::shared_ptr<__future_detail::State<void>>& ptr)
    : state_{ptr} {}

template <typename Res> Res& SharedFuture<Res>::Get() { return state_->Get(); }

inline void SharedFuture<void>::Get() { state_->Get(); }

template <typename Res>
template <typename F>
Future<
    typename Flatten<typename std::result_of<F(SharedFuture<Res>)>::type>::type>
SharedFuture<Res>::Then(Launch policy, F&& func) {
  auto& state = *state_;
  return state.Then(policy, std::forward<F>(func), *this);
}

template <typename F>
Future<typename Flatten<
    typename std::result_of<F(SharedFuture<void>)>::type>::type>
SharedFuture<void>::Then(Launch policy, F&& func) {
  auto& state = *state_;
  return state.Then(policy, std::forward<F>(func), *this);
}

template <typename Res>
template <typename F>
Future<
    typename Flatten<typename std::result_of<F(SharedFuture<Res>)>::type>::type>
SharedFuture<Res>::Then(F&& func) {
  return Then(Launch::Sync, std::forward<F>(func));
}

template <typename F>
Future<typename Flatten<
    typename std::result_of<F(SharedFuture<void>)>::type>::type>
SharedFuture<void>::Then(F&& func) {
  return Then(Launch::Sync, std::forward<F>(func));
}

template <typename Res>
template <typename F>
Future<typename Flatten<typename std::result_of<F(Future<Res>)>::type>::type>
Future<Res>::Then(Launch policy, F&& func) {
  auto& state = *state_;
  return state.Then(policy, std::forward<F>(func), std::move(*this));
}

template <typename F>
Future<typename Flatten<typename std::result_of<F(Future<void>)>::type>::type>
Future<void>::Then(Launch policy, F&& func) {
  auto& state = *state_;
  return state.Then(policy, std::forward<F>(func), std::move(*this));
}

template <typename Res>
template <typename F>
Future<typename Flatten<typename std::result_of<F(Future<Res>)>::type>::type>
Future<Res>::Then(F&& func) {
  return Then(Launch::Sync, std::forward<F>(func));
}

template <typename F>
Future<typename Flatten<typename std::result_of<F(Future<void>)>::type>::type>
Future<void>::Then(F&& func) {
  return Then(Launch::Sync, std::forward<F>(func));
}

template <typename Res> Res& Future<Res>::Get() { return state_->Get(); }

inline void Future<void>::Get() { state_->Get(); }

template <typename Res> bool Future<Res>::Ready() const {
  return state_->Ready();
}

inline bool Future<void>::Ready() const { return state_->Ready(); }

template <typename Res> Future<Res> Future<Res>::Block() {
  if (Ready())
    return std::move(*this);

  ebbrt::EventManager::EventContext context;
  ExplicitlyConstructed<Future<Res>> ret;
  Then(Launch::Async, [&context, &ret](ebbrt::Future<Res> fut) {
    ret.construct(std::move(fut));
    ebbrt::event_manager->ActivateContext(std::move(context));
  });
  ebbrt::event_manager->SaveContext(context);
  return std::move(*ret);
}

template <typename Res> bool Future<Res>::Valid() const {
  return static_cast<bool>(state_);
}

inline bool Future<void>::Valid() const { return static_cast<bool>(state_); }

template <typename Res>
Promise<Res>::Promise()
    : state_{std::make_shared<State>()} {}

inline Promise<void>::Promise() : state_{std::make_shared<State>()} {}

template <typename Res> void Promise<Res>::SetValue(Res&& res) {
  state_->SetValue(std::move(res));
}

template <typename Res> void Promise<Res>::SetValue(const Res& res) {
  state_->SetValue(res);
}

inline void Promise<void>::SetValue() { state_->SetValue(); }

template <typename Res>
void Promise<Res>::SetException(std::exception_ptr eptr) {
  state_->SetException(std::move(eptr));
}

inline void Promise<void>::SetException(std::exception_ptr eptr) {
  state_->SetException(std::move(eptr));
}

template <typename Res> Future<Res> Promise<Res>::GetFuture() {
  return Future<Res>{state_};
}

inline Future<void> Promise<void>::GetFuture() {
  return Future<void>{state_};
}

template <typename Res> __future_detail::State<Res>::State() : ready_{false} {}

inline __future_detail::State<void>::State() : ready_{false} {}

template <typename Res>
template <typename... Args>
__future_detail::State<Res>::State(__future_detail::MakeReadyFutureTag tag,
                                   Args&&... args)
    : val_(std::forward<Args>(args)...), ready_{true} {}

inline __future_detail::State<void>::State(
    __future_detail::MakeReadyFutureTag tag)
    : ready_{true} {}

template <typename Res>
__future_detail::State<Res>::State(std::exception_ptr eptr)
    : eptr_{std::move(eptr)}, ready_{true} {}

inline __future_detail::State<void>::State(std::exception_ptr eptr)
    : eptr_{std::move(eptr)}, ready_{true} {}

template <typename Res>
template <typename F, typename R>
Future<typename Flatten<typename std::result_of<F(R)>::type>::type>
__future_detail::State<Res>::Then(Launch policy, F&& func, R fut) {
  return ThenHelp(policy, std::forward<F>(func), std::move(fut));
}

template <typename F, typename R>
Future<typename Flatten<typename std::result_of<F(R)>::type>::type>
__future_detail::State<void>::Then(Launch policy, F&& func, R fut) {
  return ThenHelp(policy, std::forward<F>(func), std::move(fut));
}

// Non void return thenhelp
template <typename Res>
template <typename F, typename R>
typename std::enable_if<
    !std::is_void<typename std::result_of<F(R)>::type>::value,
    Future<typename Flatten<typename std::result_of<F(R)>::type>::type>>::type
__future_detail::State<Res>::ThenHelp(Launch policy, F&& func, R fut) {
  typedef typename std::result_of<F(R)>::type result_type;
  if (!Ready()) {
    std::lock_guard<std::mutex> lock{mutex_};
    // Have to check ready again to avoid a race
    if (!Ready()) {
      // helper sets function to be called
      auto prom = Promise<result_type>();
      auto ret = prom.GetFuture();
      if (func_)
        throw std::runtime_error("Multiple thens on a future!");
      func_ = MoveBind([](Promise<result_type> prom, R fut, F fn) {
                         try {
                           prom.SetValue(fn(std::move(fut)));
                         }
                         catch (...) {
                           prom.SetException(std::current_exception());
                         }
                       },
                       std::move(prom), std::move(fut), std::move(func));
      return flatten(std::move(ret));
    }
  }
  // We only get here if Ready is true
  if (policy == Launch::Sync) {
    try {
      return flatten(MakeReadyFuture<result_type>(func(std::move(fut))));
    }
    catch (...) {
      return flatten(MakeFailedFuture<result_type>(std::current_exception()));
    }
  } else {
    return Async(MoveBind([](F fn, R fut) { return fn(std::move(fut)); },
                          std::move(func), std::move(fut)));
  }
}

// Non void return thenhelp
template <typename F, typename R>
typename std::enable_if<
    !std::is_void<typename std::result_of<F(R)>::type>::value,
    Future<typename Flatten<typename std::result_of<F(R)>::type>::type>>::type
__future_detail::State<void>::ThenHelp(Launch policy, F&& func, R fut) {
  typedef typename std::result_of<F(R)>::type result_type;
  if (!Ready()) {
    std::lock_guard<std::mutex> lock{mutex_};
    // Have to check ready again to avoid a race
    if (!Ready()) {
      // helper sets function to be called
      auto prom = Promise<result_type>();
      auto ret = prom.GetFuture();
      if (func_)
        throw std::runtime_error("Multiple thens on a future!");
      func_ = MoveBind([](Promise<result_type> prom, R fut, F fn) {
                         try {
                           prom.SetValue(fn(std::move(fut)));
                         }
                         catch (...) {
                           prom.SetException(std::current_exception());
                         }
                       },
                       std::move(prom), std::move(fut), std::move(func));
      return flatten(std::move(ret));
    }
  }
  // We only get here if Ready is true
  if (policy == Launch::Sync) {
    try {
      return flatten(MakeReadyFuture<result_type>(func(std::move(fut))));
    }
    catch (...) {
      return flatten(MakeFailedFuture<result_type>(std::current_exception()));
    }
  } else {
    return Async(MoveBind([](F fn, R fut) { return fn(std::move(fut)); },
                          std::move(func), std::move(fut)));
  }
}

// void return thenhelp
template <typename Res>
template <typename F, typename R>
typename std::enable_if<
    std::is_void<typename std::result_of<F(R)>::type>::value,
    Future<typename Flatten<typename std::result_of<F(R)>::type>::type>>::type
__future_detail::State<Res>::ThenHelp(Launch policy, F&& func, R fut) {
  typedef typename std::result_of<F(R)>::type result_type;
  if (!Ready()) {
    std::lock_guard<std::mutex> lock{mutex_};
    // Have to check ready again to avoid a race
    if (!Ready()) {
      // helper sets function to be called
      auto prom = Promise<result_type>();
      auto ret = prom.GetFuture();
      if (func_)
        throw std::runtime_error("Multiple thens on a future!");
      func_ = MoveBind([](Promise<result_type> prom, R fut, F fn) {
                         try {
                           fn(std::move(fut));
                           prom.SetValue();
                         }
                         catch (...) {
                           prom.SetException(std::current_exception());
                         }
                       },
                       std::move(prom), std::move(fut), std::move(func));
      return flatten(std::move(ret));
    }
  }
  // We only get here if Ready is true
  if (policy == Launch::Sync) {
    try {
      func(std::move(fut));
      return flatten(MakeReadyFuture<result_type>());
    }
    catch (...) {
      return flatten(MakeFailedFuture<result_type>(std::current_exception()));
    }
  } else {
    return Async(MoveBind([](F fn, R fut) { return fn(std::move(fut)); },
                          std::move(func), std::move(fut)));
  }
}

// void return thenhelp
template <typename F, typename R>
typename std::enable_if<
    std::is_void<typename std::result_of<F(R)>::type>::value,
    Future<typename Flatten<typename std::result_of<F(R)>::type>::type>>::type
__future_detail::State<void>::ThenHelp(Launch policy, F&& func, R fut) {
  typedef typename std::result_of<F(R)>::type result_type;
  if (!Ready()) {
    std::lock_guard<std::mutex> lock{mutex_};
    // Have to check ready again to avoid a race
    if (!Ready()) {
      // helper sets function to be called
      auto prom = Promise<result_type>();
      auto ret = prom.GetFuture();
      if (func_)
        throw std::runtime_error("Multiple thens on a future!");
      func_ = MoveBind([](Promise<result_type> prom, R fut, F fn) {
                         try {
                           fn(std::move(fut));
                           prom.SetValue();
                         }
                         catch (...) {
                           prom.SetException(std::current_exception());
                         }
                       },
                       std::move(prom), std::move(fut), std::move(func));
      return flatten(std::move(ret));
    }
  }
  // We only get here if Ready is true
  if (policy == Launch::Sync) {
    try {
      func(std::move(fut));
      return flatten(MakeReadyFuture<result_type>());
    }
    catch (...) {
      return flatten(MakeFailedFuture<result_type>(std::current_exception()));
    }
  } else {
    return Async(MoveBind([](F fn, R fut) { return fn(std::move(fut)); },
                          std::move(func), std::move(fut)));
  }
}

template <typename Res>
void __future_detail::State<Res>::SetValue(const Res& res) {
  val_ = res;
  ready_ = true;
  if (func_) {
    ebbrt::event_manager->Spawn(std::move(func_));
  }
}

template <typename Res> void __future_detail::State<Res>::SetValue(Res&& res) {
  val_ = std::move(res);
  ready_ = true;
  if (func_) {
    ebbrt::event_manager->Spawn(std::move(func_));
  }
}

inline void __future_detail::State<void>::SetValue() {
  ready_ = true;
  if (func_) {
    ebbrt::event_manager->Spawn(std::move(func_));
  }
}

template <typename Res>
void __future_detail::State<Res>::SetException(std::exception_ptr eptr) {
  std::lock_guard<std::mutex> lock{mutex_};
  eptr_ = std::move(eptr);
  ready_ = true;
  if (func_) {
    ebbrt::event_manager->Spawn(std::move(func_));
  }
}

inline void
__future_detail::State<void>::SetException(std::exception_ptr eptr) {
  std::lock_guard<std::mutex> lock{mutex_};
  eptr_ = std::move(eptr);
  ready_ = true;
  if (func_) {
    ebbrt::event_manager->Spawn(std::move(func_));
  }
}

template <typename Res> Res& __future_detail::State<Res>::Get() {
  if (!Ready()) {
    throw UnreadyFutureError("Get() called on unready future");
  }

  if (eptr_ != nullptr) {
    std::rethrow_exception(eptr_);
  }

  return val_;
}

inline void __future_detail::State<void>::Get() {
  if (!Ready()) {
    throw UnreadyFutureError("Get() called on unready future");
  }

  if (eptr_ != nullptr) {
    std::rethrow_exception(eptr_);
  }
}

template <typename Res> bool __future_detail::State<Res>::Ready() const {
  return ready_;
}

inline bool __future_detail::State<void>::Ready() const { return ready_; }
}  // namespace ebbrt

#endif  // COMMON_SRC_INCLUDE_EBBRT_FUTURE_H_
