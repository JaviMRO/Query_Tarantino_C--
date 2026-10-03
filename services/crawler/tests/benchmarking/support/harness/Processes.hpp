#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace tarantino::crawler::benchmarking {

/// The operating-system processes of the benchmarks: one per benchmark, structure, size and pass (SPEC §11).
class Processes {
public:
    Processes() = delete;

    /// The executable of this process.
    static std::filesystem::path currentExecutable();

    /// Runs the executable with the arguments, with this process's standard output and error, and returns its exit
    /// status: fork and exec on Unix (SPEC §14), CreateProcess on Windows.
    static int run(const std::filesystem::path& executable, const std::vector<std::string>& arguments);
};

}
