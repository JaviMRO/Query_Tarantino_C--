#include "tarantino/platform/Unicode.hpp"

#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>

#ifdef TARANTINO_WINDOWS_ICU
// Windows 10 (version 1903) and later ship ICU as icu.dll, with an unversioned C API. MinGW-w64 has its import library
// but not the header of the Windows SDK, so the few functions used here are declared as that header declares them.
extern "C" {
typedef struct UCaseMap UCaseMap;
typedef int UErrorCode;
typedef std::int32_t UChar32;
typedef std::uint8_t UVersionInfo[4];
UCaseMap* ucasemap_open(const char* locale, std::uint32_t options, UErrorCode* status);
void ucasemap_close(UCaseMap* caseMap);
std::int32_t ucasemap_utf8ToLower(const UCaseMap* caseMap, char* destination, std::int32_t destinationCapacity,
                                  const char* source, std::int32_t sourceLength, UErrorCode* status);
std::int8_t u_isalpha(UChar32 codePoint);
void u_getUnicodeVersion(UVersionInfo version);
void u_getVersion(UVersionInfo version);
const char* u_errorName(UErrorCode code);
}

namespace {
constexpr UErrorCode U_ZERO_ERROR = 0;
constexpr UErrorCode U_BUFFER_OVERFLOW_ERROR = 15;
bool U_FAILURE(UErrorCode code) {
    return code > U_ZERO_ERROR;
}
}
#else
#include <unicode/uchar.h>
#include <unicode/ucasemap.h>
#include <unicode/utypes.h>
#include <unicode/uversion.h>
#endif

namespace tarantino::platform::unicode {

namespace {

struct CaseMapCloser {
    void operator()(UCaseMap* caseMap) const {
        ucasemap_close(caseMap);
    }
};

/// The case map of the root locale ("" in ICU), opened once and shared: case mapping does not change it.
const UCaseMap* rootCaseMap() {
    static const std::unique_ptr<UCaseMap, CaseMapCloser> caseMap = [] {
        UErrorCode status = U_ZERO_ERROR;
        UCaseMap* opened = ucasemap_open("", 0, &status);
        if (U_FAILURE(status)) {
            throw std::runtime_error(std::string("ICU could not open the case map of the root locale: ") + u_errorName(status));
        }
        return std::unique_ptr<UCaseMap, CaseMapCloser>(opened);
    }();
    return caseMap.get();
}

std::string versionText(const std::uint8_t* version) {
    int last = 3;
    while (last > 1 && version[last] == 0) {
        --last;
    }
    std::string text;
    for (int part = 0; part <= last; ++part) {
        if (part > 0) {
            text += '.';
        }
        text += std::to_string(version[part]);
    }
    return text;
}

}

std::string toLowerCase(std::string_view text) {
    if (text.empty()) {
        return {};
    }
    if (text.size() > static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max() / 2)) {
        throw std::length_error("Text too long for ICU to lowercase");
    }
    const auto length = static_cast<std::int32_t>(text.size());
    std::string lowered(text.size(), '\0');
    UErrorCode status = U_ZERO_ERROR;
    std::int32_t loweredLength =
            ucasemap_utf8ToLower(rootCaseMap(), lowered.data(), length, text.data(), length, &status);
    if (status == U_BUFFER_OVERFLOW_ERROR) {
        // Some lowercase forms are longer, such as İ, which becomes i and a combining dot above
        lowered.assign(static_cast<std::size_t>(loweredLength), '\0');
        status = U_ZERO_ERROR;
        loweredLength = ucasemap_utf8ToLower(rootCaseMap(), lowered.data(), loweredLength, text.data(), length, &status);
    }
    if (U_FAILURE(status)) {
        throw std::runtime_error(std::string("ICU could not lowercase the text: ") + u_errorName(status));
    }
    lowered.resize(static_cast<std::size_t>(loweredLength));
    return lowered;
}

bool isLetterBeyondAscii(char32_t codePoint) {
    return u_isalpha(static_cast<UChar32>(codePoint)) != 0;
}

std::string unicodeVersion() {
    UVersionInfo version;
    u_getUnicodeVersion(version);
    return versionText(version);
}

std::string icuVersion() {
    UVersionInfo version;
    u_getVersion(version);
    return versionText(version);
}

}
