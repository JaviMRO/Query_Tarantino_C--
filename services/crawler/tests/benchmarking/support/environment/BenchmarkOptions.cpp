#include "crawler/tests/benchmarking/support/environment/BenchmarkOptions.hpp"

#include "tarantino/platform/Environment.hpp"
#include "tarantino/platform/Text.hpp"

#include <algorithm>
#include <stdexcept>

namespace tarantino::crawler::benchmarking {

namespace {

constexpr std::chrono::milliseconds QUICK_ITERATION{200};

std::optional<std::string> variable(const BenchmarkOptions::Environment& environment, const std::string& name) {
    auto value = environment(name);
    if (!value || platform::strip(*value).empty()) {
        return std::nullopt;
    }
    return std::string(platform::strip(*value));
}

/// Java's Boolean.parseBoolean: true in any case, anything else false.
bool isTrue(const std::optional<std::string>& value) {
    if (!value) {
        return false;
    }
    std::string lowered = *value;
    std::transform(lowered.begin(), lowered.end(), lowered.begin(),
                   [](char c) { return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c; });
    return lowered == "true";
}

template <typename Value>
std::vector<Value> inOrder(std::vector<Value> values, const Pass& pass) {
    if (pass.reversed) {
        std::reverse(values.begin(), values.end());
    }
    return values;
}

}

BenchmarkOptions BenchmarkOptions::fromEnvironment() {
    return of([](const std::string& name) { return platform::environmentVariable(name); });
}

BenchmarkOptions BenchmarkOptions::of(const Environment& environment) {
    BenchmarkOptions options;
    options.quick_ = isTrue(variable(environment, "TARANTINO_BENCHMARK_QUICK"));
    options.skipMongo_ = isTrue(variable(environment, "TARANTINO_BENCHMARK_SKIP_MONGO"));
    if (const auto books = variable(environment, "TARANTINO_BENCHMARK_BOOKS")) {
        std::vector<int> sizes;
        for (std::size_t start = 0; start <= books->size();) {
            const std::size_t comma = std::min(books->find(',', start), books->size());
            const auto size = platform::parseDigits(platform::strip(std::string_view(*books).substr(start, comma - start)));
            if (!size || *size <= 0) {
                throw std::invalid_argument("Unknown TARANTINO_BENCHMARK_BOOKS: " + *books + " (expected sizes such as 100,300)");
            }
            sizes.push_back(*size);
            start = comma + 1;
        }
        options.sizes_ = sizes;
    }
    return options;
}

std::vector<Pass> BenchmarkOptions::passes() const {
    if (quick_) {
        return {Pass{1, false}};
    }
    return {Pass{1, false}, Pass{2, true}};
}

Iterations BenchmarkOptions::iterations(const Iterations& declared) const {
    if (quick_) {
        return Iterations{1, 1, QUICK_ITERATION};
    }
    return declared;
}

std::vector<int> BenchmarkOptions::sizes(const std::vector<int>& declared, const Pass& pass) const {
    return inOrder(sizes_.value_or(declared), pass);
}

std::vector<std::string> BenchmarkOptions::structures(const std::vector<std::string>& declared, const Pass& pass) const {
    std::vector<std::string> structures;
    std::copy_if(declared.begin(), declared.end(), std::back_inserter(structures),
                 [this](const std::string& structure) { return !(skipMongo_ && structure == "mongo"); });
    return inOrder(std::move(structures), pass);
}

}
