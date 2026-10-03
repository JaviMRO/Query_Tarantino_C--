#include "indexer/tests/adapters/index/folders/TermFileRestore.hpp"

#include "tarantino/indexer/adapters/index/folders/FolderPerTermIndexAdapter.hpp"
#include "tarantino/platform/Files.hpp"
#include "tarantino/testing/TempDirectory.hpp"

#include <gtest/gtest.h>

#include <map>

namespace tarantino::indexer::adapters {
namespace {

namespace files = platform::files;

void indexBook(const std::filesystem::path& root, int bookId, const std::vector<std::string>& terms) {
    FolderPerTermIndexAdapter index(root);
    platform::StringMap<int> frequencies;
    for (const std::string& term : terms) {
        frequencies.emplace(term, 1);
    }
    index.add(model::TermOccurrences{bookId, frequencies});
    index.flush();
}

std::map<std::filesystem::path, std::string> contents(const std::filesystem::path& root) {
    std::map<std::filesystem::path, std::string> found;
    files::walk(root, 10, [&](const files::WalkedPath& walked) {
        if (walked.regularFile) {
            found.emplace(walked.path.lexically_relative(root), files::readString(walked.path));
        }
        return true;
    });
    return found;
}

TEST(TermFileRestoreTest, undoesAnUpdateByRestoringOnlyTheTouchedTerms) {
    testing::TempDirectory directory;
    const std::filesystem::path snapshot = directory / "snapshot";
    const std::filesystem::path index = directory / "index";
    indexBook(snapshot, 1, {"island", "whale"});
    indexBook(index, 1, {"island", "whale"});
    indexBook(index, 2, {"island", "écume"});

    TermFileRestore::restore(snapshot, index, {"island", "écume"});

    EXPECT_EQ(contents(snapshot), contents(index));
}

}
}
