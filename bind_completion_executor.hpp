// Copyright 2022 Dennis Hezel
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef AGRPC_AGRPC_BIND_COMPLETION_EXECUTOR_HPP
#define AGRPC_AGRPC_BIND_COMPLETION_EXECUTOR_HPP

#include <boost/asio/async_result.hpp>
#include <boost/asio/execution.hpp>
#include <boost/asio/executor_work_guard.hpp>

namespace asio = boost::asio;

namespace agrpc_ext
{
template <class Target, class Executor>
class CompletionExecutorBinder
{
  public:
    template <class... Args>
    explicit CompletionExecutorBinder(const Executor& executor, Args&&... args)
        : target(std::forward<Args>(args)...), guard(executor)
    {
    }

    Target& get() noexcept { return target; }

    const Target& get() const noexcept { return target; }

    Executor get_executor() const noexcept { return guard.get_executor(); }

    template <class... Args>
    void operator()(Args&&... args) &&
    {
        auto executor = asio::prefer(guard.get_executor(), asio::execution::blocking_t::possibly,
                                     asio::execution::allocator(asio::get_associated_allocator(target)));
        asio::execution::execute(std::move(executor),
                                 [target = std::move(target), ... args = std::forward<Args>(args)]() mutable
                                 {
                                     std::move(target)(std::move(args)...);
                                 });
    }

  private:
    [[no_unique_address]] Target target;
    [[no_unique_address]] asio::executor_work_guard<Executor> guard;
};

template <class Executor, class Target>
CompletionExecutorBinder(const Executor&, Target&&) -> CompletionExecutorBinder<std::remove_cvref_t<Target>, Executor>;

// Implementation details
namespace detail
{
template <class Initiation, class Executor>
struct CompletionExecutorBinderAsyncResultInitWrapper
{
    template <class Handler, class... Args>
    void operator()(Handler&& handler, Args&&... args)
    {
        std::move(initiation)(agrpc_ext::CompletionExecutorBinder<std::remove_cvref_t<Handler>, Executor>(
                                  executor, std::forward<Handler>(handler)),
                              std::forward<Args>(args)...);
    }

    Executor executor;
    Initiation initiation;
};
}  // namespace detail
}

template <class CompletionToken, class Executor, class Signature>
class asio::async_result<agrpc_ext::CompletionExecutorBinder<CompletionToken, Executor>, Signature>
{
  public:
    template <class Initiation, class BoundCompletionToken, class... Args>
    static decltype(auto) initiate(Initiation&& initiation, BoundCompletionToken&& token, Args&&... args)
    {
        return asio::async_initiate<CompletionToken, Signature>(
            agrpc_ext::detail::CompletionExecutorBinderAsyncResultInitWrapper{token.get_executor(),
                                                                              std::forward<Initiation>(initiation)},
            std::forward<BoundCompletionToken>(token).get(), std::forward<Args>(args)...);
    }
};

template <template <class, class> class Associator, class Target, class Executor, class DefaultCandidate>
struct asio::associator<Associator, agrpc_ext::CompletionExecutorBinder<Target, Executor>, DefaultCandidate>
{
    using type = typename Associator<Target, DefaultCandidate>::type;

    static type get(const agrpc_ext::CompletionExecutorBinder<Target, Executor>& b,
                    const DefaultCandidate& c = DefaultCandidate()) noexcept
    {
        return Associator<Target, DefaultCandidate>::get(b.get(), c);
    }
};

#endif  // AGRPC_AGRPC_BIND_COMPLETION_EXECUTOR_HPP
