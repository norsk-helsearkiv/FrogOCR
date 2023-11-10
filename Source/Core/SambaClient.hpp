#pragma once

#include "Config.hpp"

#include <filesystem>
#include <optional>

#include <samba-4.0/libsmbclient.h>

namespace frog {

enum class DirectoryEntryFileType { directory, file, symbolic_link, other };

class SambaClient {
public:

    SambaClient(std::vector<SambaCredentialsConfig> configs);
    SambaClient(const SambaClient&) = delete;
    SambaClient(SambaClient&&) = delete;

    ~SambaClient();

    SambaClient& operator=(const SambaClient&) = delete;
    SambaClient& operator=(SambaClient&&) = delete;

    std::optional<std::string> readFile(const std::string& path);
    bool writeFile(const std::string& path, std::string_view data);

    bool createDirectory(const std::string& path);

    bool exists(const std::string& path);

    std::optional<DirectoryEntryFileType> getFileType(const std::string& path);
    std::vector<std::string> getDirectoryFiles(const std::string& path, bool recursive);

    const std::vector<SambaCredentialsConfig>& getConfigs() const;

private:

    bool createDirectories(const std::string& path);

    SMBCCTX* context{};
    std::vector<SambaCredentialsConfig> configs;
    smbc_open_fn sambaOpen{};
    smbc_read_fn sambaRead{};
    smbc_write_fn sambaWrite{};
    smbc_close_fn sambaClose{};
    smbc_stat_fn sambaStat{};
    smbc_fstat_fn sambaFStat{};
    smbc_opendir_fn sambaOpenDir{};
    smbc_readdir_fn sambaReadDir{};
    smbc_closedir_fn sambaCloseDir{};
    smbc_mkdir_fn sambaMkDir{};
    smbc_ftruncate_fn sambaFTruncate{};

};

void initialize_samba_client(std::vector<SambaCredentialsConfig> configs);
SambaClient* acquire_samba_client();
void release_samba_client();

}
