#include "ServerState.h"
#include "MsgType.h"
#include <catch2/catch_all.hpp>
#include <nlohmann/json.hpp>

TEST_CASE("Connect/Disconnect", "[ServerState]")
{
  ServerState st;

  REQUIRE(!st.userInfo[1]);
  st.Connect(1);
  REQUIRE(st.userInfo[1]);
  st.Disconnect(1);
  REQUIRE(!st.userInfo[1]);
}

TEST_CASE("maxConnectedId", "[ServerState]")
{
  ServerState st;

  REQUIRE(st.maxConnectedId == 0);
  st.Connect(0);
  REQUIRE(st.maxConnectedId == 0);
  st.Connect(1);
  REQUIRE(st.maxConnectedId == 1);
  st.Connect(2);
  REQUIRE(st.maxConnectedId == 2);
  st.Connect(3);
  REQUIRE(st.maxConnectedId == 3);

  st.Disconnect(2);
  REQUIRE(st.maxConnectedId == 3);
  st.Disconnect(3);
  REQUIRE(st.maxConnectedId == 1);
  st.Disconnect(1);
  REQUIRE(st.maxConnectedId == 0);
  st.Disconnect(0);
  REQUIRE(st.maxConnectedId == 0);
}
