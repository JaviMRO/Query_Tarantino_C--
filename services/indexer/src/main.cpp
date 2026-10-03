#include "tarantino/indexer/IndexerConfig.hpp"
#include "tarantino/indexer/IndexerFactory.hpp"
#include "tarantino/platform/Environment.hpp"
#include "tarantino/platform/Text.hpp"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using tarantino::indexer::commands::IndexResult;

/// The book ids of the arguments, all read before any work (SPEC §15).
std::vector<int> bookIds(const std::vector<std::string>& arguments) {
    std::vector<int> ids;
    for (const std::string& argument : arguments) {
        const auto bookId = tarantino::platform::parseDigits(argument);
        if (!bookId || *bookId <= 0) {
            throw std::invalid_argument("Not a book id: " + argument + " (expected a positive integer)");
        }
        ids.push_back(*bookId);
    }
    return ids;
}

/// One line per book, in the words of the control service (SPEC §15).
std::string line(const IndexResult& result) {
    return "[INDEXER] " + std::to_string(result.bookId) + ": "
           + (result.indexed ? std::to_string(result.uniqueTerms) + " unique terms indexed"
                             : std::string("skipped, not found in the datalake"));
}

}

int main(int argc, char** argv) {
    using namespace tarantino;
    platform::useUtf8Console();
    try {
        const std::vector<int> ids = bookIds(platform::commandLineArguments(argc, argv));
        indexer::commands::IndexBookCommand index = indexer::IndexerFactory::indexCommand(indexer::IndexerConfig::fromEnvironment());
        for (const IndexResult& result : index.execute(ids)) {
            std::cout << line(result) << '\n';
        }
        std::cout.flush();
        return 0;
    } catch (const std::exception& error) {
        std::cout.flush();
        std::cerr << "indexer: " << error.what() << std::endl;
        return 1;
    }
}
