#ifdef __cpp_impl_coroutine

#define TROMPELOEIL_EXPERIMENTAL_COROUTINES
#include <trompeloeil.hpp>

#if defined(CATCH2_VERSION) && CATCH2_VERSION == 3
#include <catch2/catch_test_macros.hpp>
#else
#include <catch2/catch.hpp>
#endif
#include "micro_coro.hpp"

#include <optional>

using trompeloeil::_;

namespace {
  using iptr = std::unique_ptr<int>;

  struct co_mock {
    MAKE_MOCK0 (intret, coro::task<int>());
    MAKE_MOCK0 (voidret, coro::task<void>());
    MAKE_MOCK1 (unique, coro::task<iptr>(iptr));
  };
}

TEST_CASE("A CO_RETURNed value is obtained from co_await","[coro]")
{
  co_mock m;
  REQUIRE_CALL(m, intret()).CO_RETURN(3);

  int x = 0;
   std::invoke([&]()->coro::task<void>{ x = co_await m.intret();});

  REQUIRE(x == 3);
}

TEST_CASE("The coroutine can be executed after the expectation has been retired",
          "[coro]")
{
  co_mock m;
  std::optional<coro::task<int>> t;
  {
    REQUIRE_CALL(m, intret()).CO_RETURN(3);
    t.emplace(m.intret());
  }

  int x = 0;
  std::invoke([&]()->coro::task<void>{ x = co_await *t;});

  REQUIRE(x == 3);
}

TEST_CASE("A void co_routine is CO_RETURNed", "[coro]")
{
  co_mock m;
  REQUIRE_CALL(m, voidret()).CO_RETURN();

  int x = 0;
  std::invoke([&]()->coro::task<void>{ co_await m.voidret(); x = 3;});

  REQUIRE(x == 3);
}

TEST_CASE("If the CO_RETURN expression throws, the exception is thrown from co_await",
          "[coro]")
{
  co_mock m;
  std::optional<coro::task<int>> t;
  {
    REQUIRE_CALL(m, intret()).CO_RETURN(false ? 0 : throw "foo");
    t.emplace(m.intret());
  }
  int x = 0;
  std::invoke([&]()->coro::task<void>{
    REQUIRE_THROWS(co_await *t);
    x = 1;
  });
  REQUIRE(x == 1);
}

TEST_CASE("Exception from CO_THROW is thrown from co_await",
          "[coro]")
{
  co_mock m;
  REQUIRE_CALL(m, intret()).CO_THROW("foo");
  auto p = m.intret();
  int x = 0;
  std::invoke([&]()->coro::task<void>{
    REQUIRE_THROWS(co_await p);
    x = 1;
  });
  REQUIRE(x == 1);
}

TEST_CASE("A move-only type can be CO_RETURNed from the argument", "[coro]")
{
  co_mock m;
  REQUIRE_CALL(m, unique(_)).CO_RETURN(std::move(_1));
  auto p = m.unique(std::make_unique<int>(3));
  iptr x;
  std::invoke([&]() -> coro::task<void> { x = std::move(co_await p); });
  REQUIRE(x);
  REQUIRE(*x == 3);
}

TEST_CASE("SIDE_EFFECT runs on the call to co-routine function, not when the coro runs",
          "[coro]")
{
  co_mock m;
  int x = 0;
  int y = 0;
  REQUIRE_CALL(m, intret()).LR_SIDE_EFFECT(x=1).CO_RETURN(5);
  auto p = m.intret();
  REQUIRE(x == 1);
  REQUIRE(y == 0);
  x = 0;
  std::invoke([&]() -> coro::task<void> { y = co_await p;});
  REQUIRE(x == 0);
  REQUIRE(y == 5);
}

#endif
