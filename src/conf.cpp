#include <conf.h>
#include <fstream>
#include <unistd.h>
#include <path.h>


void Permissions::validate_permissions(std::string &path) const {
    struct stat fstat{};
    stat(path.c_str(), &fstat);

    // config file can only have read permissions for user and group
    if (permbits(fstat) != 440) {
        std::stringstream ss;
        ss << "invalid permission bits: " << permbits(fstat);
        throw std::runtime_error(ss.str());
    }

    // config file has to be owned by root:root
    if (fstat.st_uid != 0 || fstat.st_gid != 0) {
        std::stringstream ss;
        ss << "invalid file owner: " << User(fstat.st_uid).name() << ":"
           << Group(fstat.st_gid).name();
        throw std::runtime_error(ss.str());
    }
}


const std::vector<ExecutablePermissions>::const_iterator Permissions::begin() const {
    return _perms.cbegin();
}

const std::vector<ExecutablePermissions>::const_iterator Permissions::end() const {
    return _perms.cend();
}

void Permissions::create(std::string &path) const {
    std::fstream fs;
    fs.open(path, std::ios::out);

    // chmod 440
    if (chmod(path.c_str(), S_IRUSR | S_IRGRP) < 0) {
        throw std::runtime_error(std::strerror(errno));
    }

    // chown root:root
    if (chown(path.c_str(), 0, 0) < 0) {
        throw std::runtime_error(std::strerror(errno));
    }
    fs.close();
}

bool Permissions::exists(std::string &path) const {
    std::ifstream f(path);
    bool exists = f.good();
    f.close();
    return exists;
}

void Permissions::load(std::string &path) {
    // check that the permissions file has the right
    // ownership and permissions
    validate_permissions(path);

    // create the file if it doesn't exist, and set the right
    // ownership and permission bits
    if (!exists(path)) {
        create(path);
    }

    // parse each line in the configuration file
    std::ifstream f(path);
    std::string line;
    while (std::getline(f, line)) {
        parse(line);
    }

    // TODO: RAII
    f.close();
}

std::vector<User> &addUsers(const std::string &user, std::vector<User> &users) {
    if (user[0] != '%') {
        users.emplace_back(User(user));
        return users;
    }

    // load the group members and add each one
    struct group *gr = getgrnam(user.substr(1, user.npos).c_str());

    if (gr == nullptr) {
        throw std::runtime_error("origin group doesn't exist");
    }

    for (auto it = gr->gr_mem; (*it) != nullptr; it++) {
        users.emplace_back(User(*it));
    }

    return users;
}

void populate_permissions(std::smatch &matches, std::vector<ExecutablePermissions> &perms) {

    // <user-or-group> -> <dest-user>:<dest-group> :: <path-to-executable-and-args>
    std::vector<User> users;

    // first match is a user or group this line refers to
    // a string that starts with a '%' is a group (like in /etc/sudoers)
    for (User &user : addUsers(matches[1], users)) {
        if (!user.exists()) {
            std::stringstream ss;
            throw std::runtime_error("origin user doesn't exist");
        }
    }

    // extract the destination user
    User dest_user = User(matches[2]);
    if (!dest_user.exists()) {
        throw std::runtime_error("dest user doesn't exist");
    }

    // extract the destination group
    Group dest_group = Group(matches[4], dest_user);
    if (!dest_group.exists()) {
        throw std::runtime_error("dest group doesn't exist");
    }

    // extract the executable path.
    // don't try to locate the path in $PATH

    std::string cmd = getpath(matches[5], false);
    std::string args = matches[7];

    // if no args are passed, the user can execute *any* args
    cmd += args.empty() ? ".*" : "\\s+" + args;

    std::regex cmd_re = std::regex(cmd);

    // populate the permissions vector
    for (User &user : users) {
        perms.emplace_back(ExecutablePermissions(user, dest_user, dest_group, cmd_re));
    }

}

void Permissions::parse(std::string &line) {

    try {
        std::smatch matches;
        if (std::regex_match(line, matches, comment_re)) {
            //  a comment, no need to parse
            return;
        }

        if (std::regex_match(line, matches, empty_re)) {
            //  an empty line, no need to parse
            return;
        }
        if (!std::regex_search(line, matches, line_re)) {
            throw std::runtime_error("couldn't parse line");
        }

        populate_permissions(matches, _perms);

    } catch (std::exception &e) {
        std::stringstream ss;
        ss << "config error - " << e.what() << " [" << line << "]";
        throw std::runtime_error(ss.str());
    }
}


const bool ExecutablePermissions::cmdcmp(const std::string &cmd) const {
    std::smatch matches;
    return std::regex_match(cmd, matches, _cmd_re);
}
