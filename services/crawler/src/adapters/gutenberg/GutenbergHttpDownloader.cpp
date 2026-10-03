#include "tarantino/crawler/adapters/gutenberg/GutenbergHttpDownloader.hpp"

#include "tarantino/crawler/model/failure/DownloadException.hpp"
#include "tarantino/platform/Text.hpp"
#include "tarantino/platform/Utf8.hpp"

#include <curl/curl.h>

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <thread>

namespace tarantino::crawler::adapters {

using model::DownloadException;
using model::FailureReason;
using Response = GutenbergHttpDownloader::Response;

namespace {

constexpr const char* MIRROR_URL_TEMPLATE = "https://mirror.cs.odu.edu/gutenberg-epub/%d/pg%d.txt";
constexpr const char* USER_AGENT = "query-tarantino/1.0 (ULPGC Big Data course project)";
constexpr long TIMEOUT_SECONDS = 30;
constexpr int MAX_REDIRECTS = 5;
constexpr int MAX_RETRIES = 5;
constexpr std::chrono::seconds FIRST_WAIT{1};
constexpr std::chrono::minutes LONGEST_WAIT{5};

bool isRedirect(long status) {
    return status == 301 || status == 302 || status == 303 || status == 307 || status == 308;
}

bool isBusy(long status) {
    return status == 429 || status == 503;
}

std::string lowercaseAscii(std::string_view text) {
    std::string lowered(text);
    std::transform(lowered.begin(), lowered.end(), lowered.begin(),
                   [](char character) { return character >= 'A' && character <= 'Z' ? static_cast<char>(character - 'A' + 'a') : character; });
    return lowered;
}

DownloadException networkError(const std::string& message) {
    return DownloadException(FailureReason::NetworkError, message);
}

void startCurl() {
    static const CURLcode started = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (started != CURLE_OK) {
        throw std::runtime_error(std::string("libcurl could not start: ") + curl_easy_strerror(started));
    }
}

/// One handle per thread, reused so that each thread keeps its connection to the mirror open from one book to the next.
CURL* handle() {
    struct Handle {
        CURL* curl = curl_easy_init();

        ~Handle() {
            curl_easy_cleanup(curl);
        }
    };
    thread_local Handle handle;
    if (handle.curl == nullptr) {
        throw networkError("libcurl could not create a handle");
    }
    curl_easy_reset(handle.curl);
    return handle.curl;
}

std::size_t appendBody(char* data, std::size_t size, std::size_t count, void* body) {
    static_cast<std::string*>(body)->append(data, size * count);
    return size * count;
}

std::size_t readHeader(char* data, std::size_t size, std::size_t count, void* target) {
    auto& response = *static_cast<Response*>(target);
    const std::string_view line(data, size * count);
    if (line.starts_with("HTTP/")) {
        response.retryAfter.reset();
    }
    const std::size_t colon = line.find(':');
    if (colon != std::string_view::npos && !response.retryAfter && lowercaseAscii(line.substr(0, colon)) == "retry-after") {
        response.retryAfter = std::string(platform::strip(line.substr(colon + 1)));
    }
    return size * count;
}

std::optional<std::string> hostOf(const std::string& url) {
    const std::unique_ptr<CURLU, decltype(&curl_url_cleanup)> parsed(curl_url(), curl_url_cleanup);
    char* host = nullptr;
    if (!parsed || curl_url_set(parsed.get(), CURLUPART_URL, url.c_str(), 0) != CURLUE_OK
        || curl_url_get(parsed.get(), CURLUPART_HOST, &host, 0) != CURLUE_OK || host == nullptr) {
        return std::nullopt;
    }
    std::string text = lowercaseAscii(host);
    curl_free(host);
    return text;
}

Response send(const std::string& url, int bookId) {
    CURL* curl = handle();
    Response response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, USER_AGENT);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, TIMEOUT_SECONDS);
    // To receive the answer: a transfer that receives nothing for as long is given up
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1L);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, TIMEOUT_SECONDS);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
#ifdef CURLSSLOPT_REVOKE_BEST_EFFORT
    // As Java, which does not check revocation by default: Schannel would fail where revocation servers are unreachable
    curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, static_cast<long>(CURLSSLOPT_REVOKE_BEST_EFFORT));
#endif
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, appendBody);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.body);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, readHeader);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response);
    const CURLcode result = curl_easy_perform(curl);
    if (result != CURLE_OK) {
        throw networkError("Could not download book " + std::to_string(bookId) + ": " + curl_easy_strerror(result));
    }
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response.status);
    char* effectiveUrl = nullptr;
    curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &effectiveUrl);
    response.url = effectiveUrl != nullptr ? effectiveUrl : url;
    // The Location of a redirect, resolved against the URL of the request
    char* location = nullptr;
    if (curl_easy_getinfo(curl, CURLINFO_REDIRECT_URL, &location) == CURLE_OK && location != nullptr) {
        response.location = location;
    }
    return response;
}

std::string redirection(const Response& response, int bookId) {
    const std::string book = std::to_string(bookId);
    if (!response.location) {
        throw networkError("HTTP " + std::to_string(response.status) + " without a valid Location for book " + book);
    }
    const auto mirror = hostOf(response.url);
    const auto target = hostOf(*response.location);
    if (!mirror || !target || *mirror != *target) {
        throw networkError("Book " + book + " was redirected to " + target.value_or(*response.location)
                           + ", which is not the mirror");
    }
    return *response.location;
}

std::string body(Response response, int bookId) {
    const std::string book = std::to_string(bookId);
    switch (response.status) {
        case 200:
            // Decoded as UTF-8 (SPEC §4): malformed sequences become U+FFFD, as Java's decoder makes them
            return platform::utf8::sanitized(std::move(response.body));
        case 404:
            throw DownloadException(FailureReason::NotFound, "Book " + book + " not found");
        default:
            throw networkError("HTTP " + std::to_string(response.status) + " for book " + book);
    }
}

}

GutenbergHttpDownloader::GutenbergHttpDownloader()
    : GutenbergHttpDownloader(MIRROR_URL_TEMPLATE, platform::systemClock(),
                              [](std::chrono::nanoseconds duration) { std::this_thread::sleep_for(duration); }) {
}

GutenbergHttpDownloader::GutenbergHttpDownloader(std::string urlTemplate, platform::Clock clock, Pause pause)
    : urlTemplate_(std::move(urlTemplate)), clock_(std::move(clock)), pause_(std::move(pause)) {
    startCurl();
}

std::string GutenbergHttpDownloader::rawText(int bookId) {
    const std::string url = platform::replaceAll(urlTemplate_, "%d", std::to_string(bookId));
    for (int retries = 0;; ++retries) {
        waitForTheMirror();
        Response response = followingRedirects(url, bookId);
        if (!isBusy(response.status)) {
            return body(std::move(response), bookId);
        }
        const std::chrono::nanoseconds wait = requestedWait(response).value_or(FIRST_WAIT * (1LL << retries));
        if (retries == MAX_RETRIES || wait > LONGEST_WAIT) {
            throw networkError("HTTP " + std::to_string(response.status) + " for book " + std::to_string(bookId)
                               + ": the mirror is busy");
        }
        pauseTheMirror(wait);
    }
}

void GutenbergHttpDownloader::waitForTheMirror() {
    platform::Instant until;
    {
        const std::lock_guard lock(pauseMutex_);
        until = pausedUntil_;
    }
    const std::chrono::nanoseconds left = until - clock_();
    if (left > std::chrono::nanoseconds::zero()) {
        pause_(left);
    }
}

void GutenbergHttpDownloader::pauseTheMirror(std::chrono::nanoseconds wait) {
    const platform::Instant until = clock_() + wait;
    const std::lock_guard lock(pauseMutex_);
    pausedUntil_ = std::max(pausedUntil_, until);
}

Response GutenbergHttpDownloader::followingRedirects(const std::string& url, int bookId) {
    Response response = send(url, bookId);
    for (int redirects = 0; isRedirect(response.status); ++redirects) {
        if (redirects == MAX_REDIRECTS) {
            throw networkError("Too many redirects for book " + std::to_string(bookId));
        }
        response = send(redirection(response, bookId), bookId);
    }
    return response;
}

/// The wait asked for by Retry-After, in seconds or as an HTTP date; empty without a readable one.
std::optional<std::chrono::nanoseconds> GutenbergHttpDownloader::requestedWait(const Response& response) const {
    if (!response.retryAfter) {
        return std::nullopt;
    }
    const std::string& retryAfter = *response.retryAfter;
    const bool seconds = !retryAfter.empty() && retryAfter.size() <= 9
                         && std::all_of(retryAfter.begin(), retryAfter.end(), [](char c) { return c >= '0' && c <= '9'; });
    if (seconds) {
        return std::chrono::seconds(std::stoll(retryAfter));
    }
    const auto date = platform::parseHttpDate(retryAfter);
    if (!date) {
        return std::nullopt;
    }
    const std::chrono::nanoseconds untilDate = *date - clock_();
    return std::max(untilDate, std::chrono::nanoseconds::zero());
}

}
