#include "tarantino/control/ControlConfig.hpp"
#include "tarantino/control/ControlFactory.hpp"
#include "tarantino/crawler/CrawlerConfig.hpp"
#include "tarantino/indexer/IndexerConfig.hpp"
#include "tarantino/platform/Environment.hpp"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    using namespace tarantino;
    platform::useUtf8Console();
    try {
        const std::vector<std::string> arguments = platform::commandLineArguments(argc, argv);
        const control::ControlConfig config = control::ControlConfig::fromEnvironment();
        const crawler::CrawlerConfig crawler = crawler::CrawlerConfig::fromEnvironment();
        const std::optional<std::string> file = arguments.empty() ? std::nullopt : std::optional(arguments.front());
        std::vector<int> candidates = control::ControlFactory::candidates(config, crawler, file);
        const auto pipeline = control::ControlFactory::pipeline(config, crawler, indexer::IndexerConfig::fromEnvironment(),
                                                                std::move(candidates), std::cout);
        for (control::model::StepReport report = pipeline->runStep(); !report.idle(); report = pipeline->runStep()) {
            std::cout << "[CONTROL] " << report.description() << std::endl;
        }
        std::cout << "[CONTROL] Nothing left to do" << std::endl;
        return 0;
    } catch (const std::exception& error) {
        std::cout.flush();
        std::cerr << "control: " << error.what() << std::endl;
        // Downloads still running are abandoned, as the JVM abandons its virtual threads
        std::fflush(nullptr);
        std::_Exit(1);
    }
}
