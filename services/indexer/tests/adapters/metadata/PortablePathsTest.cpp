#include "indexer/src/adapters/metadata/PortablePaths.hpp"

#include <gtest/gtest.h>

namespace tarantino::indexer::adapters {
namespace {

TEST(PortablePathsTest, useForwardSlashes) {
    EXPECT_EQ("datalake/20250925/14/5.body.txt",
              PortablePaths::of(std::filesystem::path("datalake") / "20250925" / "14" / "5.body.txt"));
}

}
}
