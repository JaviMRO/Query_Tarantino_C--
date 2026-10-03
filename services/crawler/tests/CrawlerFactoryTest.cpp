#include "tarantino/crawler/CrawlerFactory.hpp"
#include "tarantino/crawler/adapters/datalake/batch/BatchBasedDatalakeAdapter.hpp"
#include "tarantino/crawler/adapters/datalake/book/BookBasedDatalakeAdapter.hpp"
#include "tarantino/crawler/adapters/datalake/time/TimeBasedDatalakeAdapter.hpp"
#include "tarantino/crawler/adapters/gutenberg/GutenbergHttpDownloader.hpp"
#include "tarantino/crawler/adapters/gutenberg/LocalMirrorDownloader.hpp"

#include <gtest/gtest.h>

namespace tarantino::crawler {
namespace {

CrawlerConfig config(const std::string& layout) {
    return CrawlerConfig{"datalake", layout, std::nullopt};
}

template <typename Adapter, typename Port>
bool isA(const std::shared_ptr<Port>& port) {
    return std::dynamic_pointer_cast<Adapter>(port) != nullptr;
}

TEST(CrawlerFactoryTest, selectsTheConfiguredDatalakeLayout) {
    EXPECT_TRUE(isA<adapters::TimeBasedDatalakeAdapter>(CrawlerFactory::datalake(config("time"))));
    EXPECT_TRUE(isA<adapters::BookBasedDatalakeAdapter>(CrawlerFactory::datalake(config("book"))));
    EXPECT_TRUE(isA<adapters::BatchBasedDatalakeAdapter>(CrawlerFactory::datalake(config("batch"))));
}

TEST(CrawlerFactoryTest, downloadsOverHttpUnlessALocalMirrorIsConfigured) {
    EXPECT_TRUE(isA<adapters::GutenbergHttpDownloader>(CrawlerFactory::downloader(config("batch"))));
    EXPECT_TRUE(isA<adapters::LocalMirrorDownloader>(CrawlerFactory::downloader(CrawlerConfig{"datalake", "batch", "mirror"})));
}

TEST(CrawlerFactoryTest, rejectsUnknownLayoutsNamingThem) {
    try {
        CrawlerFactory::datalake(config("hash"));
        FAIL() << "an unknown layout was accepted";
    } catch (const std::invalid_argument& error) {
        EXPECT_STREQ("Unknown datalake layout: hash", error.what());
    }
}

}
}
