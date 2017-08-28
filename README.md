# runas

A tool that runs a binary in a non-interactive session, with different permissions.

# Why

Think about the following scenario:
- You have a binary you want to run in a non-interactive session.
- You want the binary to run with different permissions then the current user
- You don't want the user to be able to run *any binary* with *any permission*, only the one you want
- You don't want a child process to get created, because that can create TTY and signaling issues.


`sudo` and `su` execute the target binary as a child, which causes signaling issues.
`sudo` is somewhat complex to configure, and problematic when all you need is to "run this specific binary with these specific permissions and get out of the pipeline".

## Solution

A tool that is easy to set up, and does the most minimal thing possible -> run a binary with requested permissions and gets out of the way:


```console
$ runas
Usage: bin/runas user-spec command [args]

version: 0.1, license: MIT

# try to run bash as root as 'odedlaz'
$ runas root:root bash -c 'whoami && id'
'odedlaz' can't run 'bash -c whoami && id' as 'root:root': Operation not permitted

# allow odedlaz to run bash as root
# notice we're adding '/usr/bin/bash' which is a symlink to '/bin/bash'
$ echo "odedlaz -> root :: /usr/bin/bash -c 'whoami && id'" | sudo tee --append /etc/runas.conf
[sudo] password for odedlaz:
odedlaz -> root :: /usr/bin/bash

# and finally, let's try to run bash as root...
$ runas root:root bash -c 'whoami && id'
root
uid=0(root) gid=0(root) groups=0(root)

# but, we can't run anything else ->
$ runas root:root bash -c id
You can't execute '/bin/bash -c id' as 'root:root': Operation not permitted

# if you want to run *any* argument, just pass the binary, without args.
$ echo "odedlaz -> root :: /usr/bin/bash" | sudo tee --append /etc/runas.conf
odedlaz -> root :: /usr/bin/bash

# and now it'll work with *any* arguments. 
$ runas root:root bash -c id
uid=0(root) gid=0(root) groups=0(root)

# command line arguments are compiled as a ECMAScript flavored regex
# which allows fine-grained permission manipulation. for instance:
$ echo "odedlaz -> root :: /bin/systemctl (start|stop|restart|cat) .*" | sudo tee --append /etc/runas.conf
odedlaz -> root :: /bin/systemctl (start|stop|restart|cat) .* 

# now a user can execute start, stop, restart and cat operations for any systemd unit. i.e:
$ runas root systemctl cat docker
[Unit]
Description=Docker Application Container Engine
Documentation=https://docs.docker.com
After=network-online.target docker.socket firewalld.service
Wants=network-online.target
Requires=docker.socket
...
```

More examples can be found in the `runas.conf.example` file.

## Build

The following steps are needed to install:

```console
$ cmake . && sudo make install -j $(nproc)
```

To uninstall, simply run `sudo rm -f /usr/local/bin/runas`

Alternatively, you can download the latest binary from the release page. 

## Options

The configuration file contains lines in the following format: 
```
<ORIGIN-USER-OR-GROUP> -> <DEST-USER>:<DEST-GROUP> :: <PATH-TO-EXECUTABLE> <EXECUTABLE-ARGS>
```

- A group is specified by adding `%` at the beginning of the row.
- A destination group is not mandatory. One is inferred from the supplied destination user.
- Executable path is canonicalized (like readlink -f) and are validated
- Executable args are not mandatory. If supplied, they are validated as well.
  - The argument string is compiled as an [ECMAScript](https://en.wikipedia.org/wiki/ECMAScript) flavored regex.  
    if no arguments are added, *any* command args are allowed.

## FAQ

1. When running as root, the configuration is bypassed

## Guidelines

Follow the [Unix Philosophy](https://en.wikipedia.org/wiki/Unix_philosophy). Specifically:
- Small is beautiful.
- Make each program do one thing well.
- Store data in flat text files.

If you have any issues or suggestions -> feel free to open an [issue](https://github.com/odedlaz/runas/issues) or send a [pull request](https://github.com/odedlaz/runas/pulls)!

### Why reinvent gosu?

This does more or less exactly the same thing as [gosu](https://github.com/tianon/gosu), except that:
1. It adds a [much needed] permissions mechanism
2. The entire binary weighs 200kb, and ~60kb with [UPX](https://upx.github.io)
3. It was a fun exercise :)
