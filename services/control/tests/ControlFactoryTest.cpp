#include "tarantino/control/ControlFactory.hpp"
#include "tarantino/platform/Files.hpp"
#include "tarantino/testing/TempDirectory.hpp"

#include <gtest/gtest.h>

namespace tarantino::control {
namespace {

namespace files = platform::files;

class ControlFactoryTest : public ::testing::Test {
protected:
    ControlConfig config() const {
        return ControlConfig{root / "control", root / "workload", 8, 100};
    }

    crawler::CrawlerConfig crawlerConfig(std::optional<std::filesystem::path> mirror) const {
        return crawler::CrawlerConfig{root / "datalake", "time", std::move(mirror)};
    }

    void write(std::string_view file, std::string_view content) {
        files::createDirectories((root / file).parent_path());
        files::writeString(root / file, content);
    }

    testing::TempDirectory root;
};

TEST_F(ControlFactoryTest, takesTheCandidatesOfTheWorkloadFileGivenEvenWithAMirror) {
    write("workload/book_ids.txt", "84\n\n 1342 \n");
    write("mirror/11/pg11.txt", "text");

    EXPECT_EQ((std::vector<int>{84, 1342}), ControlFactory::candidates(config(), crawlerConfig(root / "mirror"), "book_ids.txt"));
}

TEST_F(ControlFactoryTest, takesEveryBookOfTheMirrorInIdOrderWithoutAFile) {
    write("mirror/1342/pg1342.txt", "text");
    write("mirror/84/pg84.txt", "text");

    EXPECT_EQ((std::vector<int>{84, 1342}), ControlFactory::candidates(config(), crawlerConfig(root / "mirror"), std::nullopt));
}

TEST_F(ControlFactoryTest, takesTheSampleWithoutAFileOrAMirror) {
    write("workload/sample_ids.txt", "1342\n84\n");

    EXPECT_EQ((std::vector<int>{1342, 84}), ControlFactory::candidates(config(), crawlerConfig(std::nullopt), std::nullopt));
}

TEST_F(ControlFactoryTest, rejectsACandidatesFileWithSomethingElseThanBookIds) {
    write("workload/book_ids.txt", "84\nabc\n");

    EXPECT_THROW(ControlFactory::candidates(config(), crawlerConfig(std::nullopt), "book_ids.txt"), std::invalid_argument);
}

}
}
