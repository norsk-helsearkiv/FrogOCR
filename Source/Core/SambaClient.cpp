#include "Core/SambaClient.hpp"
#include "Core/Log.hpp"

namespace frog {

static std::unique_ptr<SambaClient> globalSambaClient;
static std::mutex sambaMutex;

void initialize_samba_client(std::vector<SambaCredentialsConfig> configs) {
    if (configs.empty()) {
        return;
    }
    globalSambaClient = std::make_unique<SambaClient>(std::move(configs));
}

SambaClient* acquire_samba_client() {
    if (!globalSambaClient) {
        return nullptr;
    }
    sambaMutex.lock();
    return globalSambaClient.get();
}

void release_samba_client() {
    sambaMutex.unlock();
}

static std::optional<DirectoryEntryFileType> getDirectoryEntryFileTypeFromType(unsigned int type) {
    switch (type) {
    case SMBC_DIR:
        return DirectoryEntryFileType::directory;
    case SMBC_FILE:
        return DirectoryEntryFileType::file;
    case SMBC_LINK:
        return DirectoryEntryFileType::symbolic_link;
    case SMBC_WORKGROUP:
    case SMBC_SERVER:
    case SMBC_FILE_SHARE:
    case SMBC_PRINTER_SHARE:
    case SMBC_COMMS_SHARE:
    case SMBC_IPC_SHARE:
        return DirectoryEntryFileType::other;
    default:
        return std::nullopt;
    }
}

void get_samba_context_auth_data_callback(SMBCCTX* context, const char* server, const char* share, char* workgroup, int workgroupSize,
                                  char* username, int usernameSize, char* password, int passwordSize) {
    auto userData = smbc_getOptionUserData(context);
    auto sambaClient = static_cast<SambaClient*>(userData);
    if (!sambaClient) {
        return;
    }
    const auto& configs = sambaClient->getConfigs();
    if (configs.empty()) {
        return;
    }
    const auto& config = configs.front(); // TODO: Figure out auth based on server and share. This is not currently necessary for NHA.
    if (workgroupSize > config.workgroup.size() && !config.workgroup.empty()) {
        std::strcpy(workgroup, config.workgroup.c_str());
    }
    if (usernameSize > config.username.size() && !config.username.empty()) {
        std::strcpy(username, config.username.c_str());
    }
    if (passwordSize > config.password.size() && !config.password.empty()) {
        std::strcpy(password, config.password.c_str());
    }
}

SambaClient::SambaClient(std::vector<SambaCredentialsConfig> configs_) : configs{ std::move(configs_) } {
    context = smbc_new_context();
    if (!context) {
        fmt::print("Failed to create new samba client context.\n");
        return;
    }
#ifndef NDEBUG
    smbc_setDebug(context, 1);
#endif
    smbc_setFunctionAuthDataWithContext(context, get_samba_context_auth_data_callback);
    smbc_setOptionUserData(context, this);
    smbc_setOptionFullTimeNames(context, 1);
    if (!smbc_init_context(context)) {
        fmt::print("Failed to initialize samba client context.\n");
        return;
    }
    sambaOpen = smbc_getFunctionOpen(context);
    if (!sambaOpen) {
        fmt::print("No open function.\n");
    }
    sambaRead = smbc_getFunctionRead(context);
    if (!sambaRead) {
        fmt::print("No read function.\n");
    }
    sambaWrite = smbc_getFunctionWrite(context);
    if (!sambaWrite) {
        fmt::print("No write function.\n");
    }
    sambaClose = smbc_getFunctionClose(context);
    if (!sambaClose) {
        fmt::print("No close function.\n");
    }
    sambaStat = smbc_getFunctionStat(context);
    if (!sambaStat) {
        fmt::print("No stat function.\n");
    }
    sambaFStat = smbc_getFunctionFstat(context);
    if (!sambaFStat) {
        fmt::print("No fstat function.\n");
    }
    sambaOpenDir = smbc_getFunctionOpendir(context);
    if (!sambaOpenDir) {
        fmt::print("No opendir function.\n");
    }
    sambaReadDir = smbc_getFunctionReaddir(context);
    if (!sambaReadDir) {
        fmt::print("No readdir function.\n");
    }
    sambaCloseDir = smbc_getFunctionClosedir(context);
    if (!sambaCloseDir) {
        fmt::print("No closedir function.\n");
    }
    sambaMkDir = smbc_getFunctionMkdir(context);
    if (!sambaMkDir) {
        fmt::print("No mkdir function.\n");
    }
    sambaFTruncate = smbc_getFunctionFtruncate(context);
    if (!sambaFTruncate) {
        fmt::print("No ftruncate function.\n");
    }
}

SambaClient::~SambaClient() {
    if (context) {
        smbc_free_context(context, 0);
    }
}

std::optional<std::string> SambaClient::readFile(const std::string& path) {
    const auto file = sambaOpen(context, path.c_str(), O_RDONLY, 0666);
    if (!file) {
        fmt::print("Failed to open file: {}. Error: ", path);
        switch (errno) {
        case EINVAL:
            fmt::print("Invalid file handle.\n");
            break;
        case EISDIR:
            fmt::print("This is a directory.\n");
            break;
        case EACCES:
            fmt::print("No permission.\n");
            break;
        case ENODEV:
            fmt::print("Share does not exist.\n");
            break;
        case ENOENT:
            fmt::print("A directory component in the path does not exist.\n");
            break;
        default:
            fmt::print("{}\n", errno);
        }
        return std::nullopt;
    }
    std::string data;
    constexpr std::size_t bufferSize{ 1024 * 1024 };
    auto buffer = new char[bufferSize];
    bool success{ true };
    while (true) {
        const auto readSize = sambaRead(context, file, buffer, bufferSize);
        if (readSize == 0) {
            break;
        }
        if (readSize < 0) {
            log::error("Read error: {}", errno);
            success = false;
            break;
        }
        data.append(buffer, readSize);
    }
    delete[] buffer;
    sambaClose(context, file);
    return success ? data : std::optional<std::string>{};
}

bool SambaClient::writeFile(const std::string& path, std::string_view data) {
    const auto parentDirectory = path.substr(0, path.find_last_of('/'));
    if (!createDirectories(parentDirectory)) {
        return false;
    }
    const auto file = sambaOpen(context, path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (!file) {
        fmt::print("Failed to open file for writing: {}.\n", path);
        return false;
    }
    bool success{ true };
    std::size_t index{};
    while (data.size() > index) {
        const auto writeSize = sambaWrite(context, file, data.data() + index, data.size() - index);
        if (writeSize <= 0) {
            if (errno != 0) {
                success = false;
            }
            break;
        }
        index += writeSize;
    }
    sambaFTruncate(context, file, static_cast<off_t>(data.size()));
    sambaClose(context, file);
    return success;
}

bool SambaClient::createDirectory(const std::string& path) {
    if (sambaMkDir(context, path.c_str(), 0777) != 0) {
        fmt::print("Failed to create directory: {}\n", path);
        return false;
    }
    return true;
}

static std::optional<DirectoryEntryFileType> getDirectoryEntryFileTypeFromMode(mode_t mode) {
    switch (mode & S_IFMT) {
    case S_IFDIR: return DirectoryEntryFileType::directory;
    case S_IFREG: return DirectoryEntryFileType::file;
    case S_IFLNK: return DirectoryEntryFileType::symbolic_link;
    case S_IFSOCK:
    case S_IFBLK:
    case S_IFCHR:
    case S_IFIFO: return DirectoryEntryFileType::other;
    default: return std::nullopt;
    }
}

bool SambaClient::exists(const std::string& path) {
    return getFileType(path).has_value();
}

std::optional<DirectoryEntryFileType> SambaClient::getFileType(const std::string& path) {
    struct stat result{};
    const auto statResult = sambaStat(context, path.c_str(), &result);
    const auto statErrno = errno;
    if (statResult < 0) {
        switch (statErrno) {
        case EINVAL:
            fmt::print("Failed to stat {}. Invalid path.\n", path);
            break;
        case EACCES:
            fmt::print("Failed to stat {}. Permission denied.\n", path);
            break;
        case 0:
            break;
        default:
            fmt::print("Failed to stat {}. Error: {}.\n", path, statErrno);
            break;
        }
        return std::nullopt;
    }
    return getDirectoryEntryFileTypeFromMode(result.st_mode);
}

std::vector<std::string> SambaClient::getDirectoryFiles(const std::string& path, bool recursive) {
    const auto directory = sambaOpenDir(context, path.c_str());
    if (!directory) {
        fmt::print("Failed to open directory: {}.\n", path);
        return {};
    }
    std::vector<std::string> files;
    while (true) {
        const auto entry = sambaReadDir(context, directory);
        if (!entry) {
            break;
        }
        std::string name{ entry->name };
        if (name == "." || name == ".." || name.empty()) {
            continue;
        }
        const auto type = getDirectoryEntryFileTypeFromType(entry->smbc_type);
        if (type == DirectoryEntryFileType::directory && recursive) {
            auto subFiles = getDirectoryFiles(fmt::format("{}/{}", path, name), true);
            files.insert(files.end(), subFiles.begin(), subFiles.end());
        } else if (type == DirectoryEntryFileType::file) {
            files.emplace_back(fmt::format("{}/{}", path, name));
        }
    }
    if (sambaCloseDir(context, directory) < 0) {
        fmt::print("Failed closing directory. Error: {}\n", errno);
    }
    return files;
}

const std::vector<SambaCredentialsConfig>& SambaClient::getConfigs() const {
    return configs;
}

bool SambaClient::createDirectories(const std::string& path) {
    if (sambaMkDir(context, path.c_str(), 0777) != 0) {
        switch (errno) {
        case ENOENT:
            if (createDirectories(path.substr(0, path.find_last_of('/')))) {
                return createDirectory(path);
            }
            return false;
        case EEXIST:
            return true;
        case EACCES:
            log::error("Failed to create directory: {}. Permission denied.", path);
            return false;
        default:
            return false;
        }
    }
    return true;
}

}
