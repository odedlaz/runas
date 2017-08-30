#pragma once

#include <unistd.h>
#include <fstream>

void validate_runas_binary(const std::string &path);

bool hasperm(const Permissions &permissions, User &user, Group &grp, char *const cmdargs[]);

bool bypass_perms(User &running_user, User &dest_user, Group &dest_group);
