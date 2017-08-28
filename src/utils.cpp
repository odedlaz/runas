#include <conf.h>
#include <utils.h>

void validate_runas_binary(const std::string &path) {
    struct stat fstat{};

    if (stat(path.c_str(), &fstat) != 0) {
        throw std::runtime_error(path + " : " + std::strerror(errno));
    }

    if ((fstat.st_mode & S_ISUID) == 0) {
        throw std::runtime_error("SUID is not set on runas");
    }
    if (fstat.st_uid != 0 || fstat.st_gid != 0) {
        throw std::runtime_error("runas owner should be 'root:root'");
    }
}

bool can_execute(const User &user, const Group &group, const std::string &cmd,
                 const ExecutablePermissions &perm) {
    if (perm.user().id() != getuid()) {
        return false;
    }

    if (perm.dest_user().id() != user.id()) {
        return false;
    }

    if (perm.dest_group().id() != group.id()) {
        return false;
    }

    return perm.cmdcmp(cmd);

}

bool hasperm(User &user, Group &group, char *const cmdargs[]) {
    Permissions config;
    std::string path = std::string(DEFAULT_CONFIG_PATH);
    std::string cmd = cmdargs[0];

    config.load(path);

    struct stat fstat{};
    if (stat(DEFAULT_CONFIG_PATH, &fstat) != 0) {
        throw std::runtime_error(cmd + " : " + std::strerror(errno));
    }

    std::stringstream ss{cmd, std::ios_base::app | std::ios_base::out};
    for (int i = 1; cmdargs[i] != nullptr; i++) {
        ss << " " << cmdargs[i];
    }

    for (const ExecutablePermissions &perm : config) {
        if (can_execute(user, group, ss.str(), perm)) {
            return true;
        }
    }
    return false;
}


bool bypass_perms(User &running_user, User &dest_user, Group &dest_group) {

    // if the user / grp is root, just let them run.
    if (running_user.id() == 0 && running_user.gid() == 0) {
        return true;
    }

    // if the user / grp are the same as the running user,
    // just run the app without performing any runas
    return running_user.id() == dest_user.id() && running_user.gid() == dest_group.id();

}
