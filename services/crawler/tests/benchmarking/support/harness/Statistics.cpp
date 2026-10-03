#include "crawler/tests/benchmarking/support/harness/Statistics.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

namespace tarantino::crawler::benchmarking {

namespace {

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

/// The continued fraction of the incomplete beta function (Numerical Recipes, betacf), by modified Lentz's method.
double betaContinuedFraction(double a, double b, double x) {
    constexpr int maxIterations = 300;
    constexpr double epsilon = 1e-15;
    constexpr double tiny = 1e-300;
    const double qab = a + b;
    const double qap = a + 1.0;
    const double qam = a - 1.0;
    double c = 1.0;
    double d = 1.0 - qab * x / qap;
    d = std::fabs(d) < tiny ? tiny : d;
    d = 1.0 / d;
    double h = d;
    for (int m = 1; m <= maxIterations; ++m) {
        const int m2 = 2 * m;
        double aa = m * (b - m) * x / ((qam + m2) * (a + m2));
        d = 1.0 + aa * d;
        d = std::fabs(d) < tiny ? tiny : d;
        c = 1.0 + aa / c;
        c = std::fabs(c) < tiny ? tiny : c;
        d = 1.0 / d;
        h *= d * c;
        aa = -(a + m) * (qab + m) * x / ((a + m2) * (qap + m2));
        d = 1.0 + aa * d;
        d = std::fabs(d) < tiny ? tiny : d;
        c = 1.0 + aa / c;
        c = std::fabs(c) < tiny ? tiny : c;
        d = 1.0 / d;
        const double delta = d * c;
        h *= delta;
        if (std::fabs(delta - 1.0) < epsilon) {
            break;
        }
    }
    return h;
}

/// The regularized incomplete beta function I_x(a, b).
double regularizedBeta(double x, double a, double b) {
    if (x <= 0.0) {
        return 0.0;
    }
    if (x >= 1.0) {
        return 1.0;
    }
    const double front = std::exp(std::lgamma(a + b) - std::lgamma(a) - std::lgamma(b) + a * std::log(x) + b * std::log(1.0 - x));
    if (x < (a + 1.0) / (a + b + 2.0)) {
        return front * betaContinuedFraction(a, b, x) / a;
    }
    return 1.0 - front * betaContinuedFraction(b, a, 1.0 - x) / b;
}

/// The cumulative probability of Student's t distribution at t, for t >= 0.
double studentCumulative(double t, double degreesOfFreedom) {
    return 1.0 - 0.5 * regularizedBeta(degreesOfFreedom / (degreesOfFreedom + t * t), degreesOfFreedom / 2.0, 0.5);
}

}

Statistics::Statistics(std::vector<double> values) : values_(std::move(values)) {
}

void Statistics::add(double value) {
    values_.push_back(value);
}

std::size_t Statistics::count() const {
    return values_.size();
}

double Statistics::mean() const {
    if (values_.empty()) {
        return NOT_A_NUMBER;
    }
    return std::accumulate(values_.begin(), values_.end(), 0.0) / static_cast<double>(values_.size());
}

double Statistics::standardDeviation() const {
    if (values_.size() < 2) {
        return NOT_A_NUMBER;
    }
    const double average = mean();
    double squares = 0.0;
    for (const double value : values_) {
        squares += (value - average) * (value - average);
    }
    return std::sqrt(squares / static_cast<double>(values_.size() - 1));
}

double Statistics::meanErrorAt(double confidence) const {
    if (values_.size() < 2) {
        return NOT_A_NUMBER;
    }
    const auto samples = static_cast<double>(values_.size());
    return studentQuantile(1.0 - (1.0 - confidence) / 2.0, samples - 1.0) * standardDeviation() / std::sqrt(samples);
}

double Statistics::percentile(double rank) const {
    if (values_.empty()) {
        return NOT_A_NUMBER;
    }
    std::vector<double> sorted = values_;
    std::sort(sorted.begin(), sorted.end());
    const double threshold = static_cast<double>(sorted.size()) * rank / 100.0;
    const auto position = static_cast<std::size_t>(std::max(1.0, std::ceil(threshold)));
    return sorted[std::min(position, sorted.size()) - 1];
}

double Statistics::studentQuantile(double probability, double degreesOfFreedom) {
    if (probability == 0.5) {
        return 0.0;
    }
    if (probability < 0.5) {
        return -studentQuantile(1.0 - probability, degreesOfFreedom);
    }
    // Bisection on the cumulative probability, which grows with t
    double low = 0.0;
    double high = 1.0;
    while (studentCumulative(high, degreesOfFreedom) < probability) {
        high *= 2.0;
    }
    for (int iteration = 0; iteration < 200 && high - low > 1e-13 * high; ++iteration) {
        const double middle = (low + high) / 2.0;
        (studentCumulative(middle, degreesOfFreedom) < probability ? low : high) = middle;
    }
    return (low + high) / 2.0;
}

}
