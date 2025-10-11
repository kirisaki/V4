#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include <v4/vm_api.h>

TEST_CASE("vm version")
{
  CHECK(v4_vm_version() == 1);
}

TEST_CASE("basic arithmetic")
{
  // ここに VM のミニ実行例を書いていく
  CHECK(2 + 3 == 5);
}
