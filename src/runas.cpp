#include <conf.h>
#include <utils.h>
#include <path.h>
#include <version.h>

int runas(const std::string &username, const std::string &grpname, const std::string &cmd, char *const cmdargs[]) {

    User running_user = User(getuid());

    // load destination_user and check that it exists
    User dest_user = User(username);
    if (!dest_user.exists()) {
        std::stringstream ss;
        ss << "'" << running_user.name() << "' can't run: user '" << username << "' doesn't exist";
        throw std::runtime_error(ss.str());
    }

    // load destination group and check that it exists
    Group dest_group = Group(grpname, dest_user);
    if (!dest_group.exists()) {
        std::stringstream ss;
        ss << "'" << running_user.name() << "' can't run: group '" << grpname << " ""' doesn't exist";
        throw std::runtime_error(ss.str());
    }

    // check in the configuration if the destination user can run the command with the requested permissions
    if (!bypass_perms(running_user, dest_user, dest_group) &&
        !hasperm(dest_user, dest_group, cmd, cmdargs)) {
        std::stringstream ss;
        ss << "You can't execute '";
        for (int i = 0; cmdargs[i] != nullptr; i++) {
            std::string suffix = cmdargs[i + 1] != nullptr ? " " : "";
            ss << cmdargs[i] << suffix;
        }
        ss << "' as '" << dest_user.name() << ":" << dest_group.name()
           << "': " << std::strerror(EPERM);
        throw std::runtime_error(ss.str());
    }

    // update the HOME env according to the dest_user dir
    setenv("HOME", dest_user.dir().c_str(), 1);

    // set permissions to requested id and gid
    setperm(dest_user, dest_group);

    // execute with uid and gid. path lookup is done internally, so execvp is not needed.
    execv(cmd.c_str(), cmdargs);

    // will not get here unless execvp failed
    throw std::runtime_error(cmd + " : " + std::strerror(errno));
}

int main(int argc, char *argv[]) {
    try {
        // check that enough args were passed
        if (argc < 3) {
            std::cout << "Usage: " << argv[0]
                      << " user-spec command [args]" << std::endl << std::endl <<
                      "version: " << VERSION << ", license: MIT" << std::endl;
            return 0;
        }

        // check that the running binary has the right permissions
        // i.e: suid is set and owned by root:root
        validate_runas_binary(getpath(argv[0], true));

        // load the arguments into a vector, then add a null at the end,
        // because execvp needs a null pointer at the end of the argument array.
        std::vector<char *> args{argv, argv + argc};
        args.emplace_back((char *) nullptr);

        // get the permission text: <user>:<group> (also <uid>:<gid>)
        const std::string perms(args[1]);

        // extract the user and group
        unsigned long delimIdx = perms.find(':');
        const std::string group(delimIdx != perms.npos ? perms.substr(delimIdx + 1) : "");

        // if <user>:<group> passed, update user string to <user>
        const std::string user = group.empty() ? perms : perms.substr(0, delimIdx);

        // extract the cmd (try to locate the binary in $PATH)
        const std::string cmd{getpath(args[2], true)};

        return runas(user, group, cmd, &args[2]);

    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}
