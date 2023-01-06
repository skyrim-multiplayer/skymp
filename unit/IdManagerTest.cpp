#include "IdManager.h"
#include "Networking.h"
#include <catch2/catch_all.hpp>
#include <chrono>
#include <thread>

using namespace std::chrono_literals;

TEST_CASE("Unit tests", "[IdManager]")
{
  IdManager idManager(3);

  // Trying to find unexisting values
  REQUIRE(idManager.find(9999) == RakNetGUID(-1));
  REQUIRE(idManager.find(RakNetGUID(204)) == Networking::InvalidUserId);

  // Allocate ids then find them
  auto id0 = idManager.allocateId(RakNetGUID(204));
  auto id1 = idManager.allocateId(RakNetGUID(1779));
  auto id2 = idManager.allocateId(RakNetGUID(79841));
  REQUIRE(idManager.find(RakNetGUID(204)) == 0);
  REQUIRE(idManager.find(0) == RakNetGUID(204));
  REQUIRE(id0 == 0);
  REQUIRE(idManager.find(RakNetGUID(1779)) == 1);
  REQUIRE(idManager.find(1) == RakNetGUID(1779));
  REQUIRE(id1 == 1);
  REQUIRE(idManager.find(RakNetGUID(79841)) == 2);
  REQUIRE(idManager.find(2) == RakNetGUID(79841));
  REQUIRE(id2 == 2);

  // freeId works
  idManager.freeId(id1);
  REQUIRE(idManager.find(1) == RakNetGUID(-1));
  REQUIRE(idManager.find(RakNetGUID(1779)) == Networking::InvalidUserId);

  // Allocation of prevoiosly freed id works
  auto newId1 = idManager.allocateId(RakNetGUID(919191));
  REQUIRE(newId1 == 1);
  REQUIRE(idManager.find(RakNetGUID(919191)) == 1);
  REQUIRE(idManager.find(1) == RakNetGUID(919191));

  // IdManager is full
  REQUIRE(idManager.allocateId(RakNetGUID(72)) == Networking::InvalidUserId);

  // freeId works
  idManager.freeId(newId1);
  idManager.freeId(id0);
  idManager.freeId(id2);
  REQUIRE(idManager.find(0) == RakNetGUID(-1));
  REQUIRE(idManager.find(1) == RakNetGUID(-1));
  REQUIRE(idManager.find(2) == RakNetGUID(-1));
}
