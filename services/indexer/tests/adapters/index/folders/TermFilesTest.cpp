#include "indexer/src/adapters/index/folders/TermFiles.hpp"

#include "tarantino/platform/Paths.hpp"
#include "tarantino/platform/Sha256.hpp"

#include <gtest/gtest.h>

namespace tarantino::indexer::adapters {
namespace {

const std::filesystem::path ROOT = "index";

std::string repeated(std::string_view text, int times) {
    std::string result;
    for (int time = 0; time < times; ++time) {
        result += text;
    }
    return result;
}

std::filesystem::path relative(std::string_view path) {
    return ROOT / platform::pathOf(path);
}

TEST(TermFilesTest, keepsAsciiLowercaseTermsReadableUnderTheirFirstLetter) {
    EXPECT_EQ(relative("w/whale.txt"), TermFiles::file(ROOT, "whale"));
}

TEST(TermFilesTest, encodesEveryOtherUtf8ByteAndUsesTheFirstCodePointAsDirectory) {
    EXPECT_EQ(relative("%C3%A9/%C3%A9cume.txt"), TermFiles::file(ROOT, "écume"));
    EXPECT_EQ(relative("%F0%9D%92%9C/%F0%9D%92%9Cb.txt"), TermFiles::file(ROOT, "\U0001D49Cb"));
}

TEST(TermFilesTest, namesTermsThatOnlyDifferInCaseFoldingOrNormalizationDifferently) {
    EXPECT_NE(TermFiles::file(ROOT, "shape"), TermFiles::file(ROOT, "ſhape"));
    EXPECT_NE(TermFiles::file(ROOT, "café"), TermFiles::file(ROOT, "café"));
}

TEST(TermFilesTest, hashesNamesLongerThan200Characters) {
    const std::string longTerm = repeated("漢", 100);
    const auto digest = platform::sha256(longTerm);

    EXPECT_EQ(relative("%E6%BC%A2/#" + platform::hex(digest.data(), digest.size()) + ".txt"), TermFiles::file(ROOT, longTerm));
    EXPECT_EQ(relative("l/" + std::string(200, 'l') + ".txt"), TermFiles::file(ROOT, std::string(200, 'l')));
}

}
}
