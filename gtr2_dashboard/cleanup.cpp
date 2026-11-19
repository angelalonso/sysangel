#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;

struct CleanupEntry {
    fs::path resolved;
    bool exists = false;
    bool directory = false;
};

static std::string trim(const std::string& value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

static bool samePath(const fs::path& left, const fs::path& right) {
    std::error_code error;
    return fs::equivalent(left, right, error) && !error;
}

static int fail(const std::string& message) {
    std::cerr << "Error: " << message << '\n';
    return 1;
}

int main() {
    const fs::path listPath = fs::absolute("cleanup.list");
    std::ifstream input(listPath);
    if (!input) {
        return fail("cannot open " + listPath.string());
    }

    std::vector<CleanupEntry> entries;
    std::string line;
    std::size_t lineNumber = 0;
    while (std::getline(input, line)) {
        ++lineNumber;
        const std::string value = trim(line);
        if (value.empty() || value[0] == '#') {
            continue;
        }

        fs::path source(value);
        const fs::path resolved = source.is_absolute()
            ? source.lexically_normal()
            : (listPath.parent_path() / source).lexically_normal();

        if (samePath(resolved, listPath)) {
            std::cerr << "Error: cleanup.list cannot contain itself (line "
                      << lineNumber << ")\n";
            return 1;
        }

        std::error_code error;
        const bool exists = fs::exists(resolved, error);
        if (error) {
            std::cerr << "Error: cannot inspect line " << lineNumber << " ("
                      << resolved.string() << "): " << error.message() << '\n';
            return 1;
        }

        entries.push_back({resolved, exists, exists && fs::is_directory(resolved)});
    }

    if (entries.empty()) {
        std::cout << "cleanup.list contains no paths. Nothing to remove.\n";
        return 0;
    }

    std::cout << "The following paths are listed for removal:\n";
    for (std::size_t index = 0; index < entries.size(); ++index) {
        const CleanupEntry& entry = entries[index];
        std::cout << "  " << (index + 1) << ". " << entry.resolved.string();
        if (!entry.exists) {
            std::cout << " [missing - will be skipped]";
        } else {
            std::cout << (entry.directory ? " [folder]" : " [file]");
        }
        std::cout << '\n';
    }

    std::cout << "\nThis operation permanently deletes existing files and folders.\n"
              << "Type DELETE to continue, or anything else to cancel: ";
    std::string confirmation;
    std::getline(std::cin, confirmation);
    if (confirmation != "DELETE") {
        std::cout << "Cancelled. Nothing was removed.\n";
        return 0;
    }

    std::size_t removed = 0;
    std::size_t skipped = 0;
    std::size_t failed = 0;
    for (const CleanupEntry& entry : entries) {
        std::error_code error;
        if (!fs::exists(entry.resolved, error)) {
            ++skipped;
            continue;
        }

        const std::uintmax_t count = fs::remove_all(entry.resolved, error);
        if (error) {
            ++failed;
            std::cerr << "Failed: " << entry.resolved.string()
                      << " - " << error.message() << '\n';
        } else if (count > 0) {
            ++removed;
            std::cout << "Removed: " << entry.resolved.string() << '\n';
        } else {
            ++skipped;
        }
    }

    std::cout << "\nFinished: " << removed << " path(s) removed, "
              << skipped << " skipped, " << failed << " failed.\n";
    return failed == 0 ? 0 : 1;
}
