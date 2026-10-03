#pragma once

#include <optional>
#include <string>
#include <vector>

/// The process's environment, arguments and console, in UTF-8 on every operating system (SPEC §2, §15).
namespace tarantino::platform {

/// The value of an environment variable, as System.getenv(name); empty when it is not set.
std::optional<std::string> environmentVariable(const std::string& name);

/// The value of an environment variable, or the default when it is not set, as System.getenv().getOrDefault.
std::string environmentVariable(const std::string& name, const std::string& defaultValue);

/// The arguments of the program, without its name. On Windows they are read again from the wide command line, since
/// argv holds them in the ANSI code page, where words such as rocín or cæsar may not fit.
std::vector<std::string> commandLineArguments(int argc, char** argv);

/// Makes the console show the UTF-8 the services write (SPEC §15) until the process exits; only Windows needs it.
void useUtf8Console();

}
