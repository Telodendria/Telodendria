# Hosting Telodendria

These are just my own personal notes for hosting Telodendria's code infrastructure. This document is not intended to be used by normal Telodendria users or developers. It may be useful if you are *forking* Telodendria, but I sincerely hope you'll contribute to the upstream project instead. I'm writing this document solely for my own reference, but I am placing it into Telodendria's code repository in the name of transparency.

## Runners

The general sequence of steps required for setting up a CI runner is as follows:

1. Install the runner OS with all the defaults. I typically install my runners in virtual machines with 1 vcpu and 512mb RAM. Only Debian complained about this configuration, but since I didn't install a desktop environment, it worked out fine.
2. Install the packages required to build and execute the runner. These are:
    - Git for checking out the source code.
    - NodeJS for running `actions/checkout`, I think. Not really sure, all I know is that the runner will fail all jobs without NodeJS.
    - Go for compiling the runner itself.

    Run these commands to install the packages:
    - **OpenBSD:** `pkg_add git go node`
    - **FreeBSD:** `pkg install git go node`
    - **NetBSD:** `pkgin install git go nodejs openssl mozilla-rootcerts-openssl`
      (Note that the `go` executable is `go121` or whatever version was installed. and that NetBSD has no root certificates installed by default)
    - **Debian:** `apt install git golang nodejs`
    - **Alpine:** `apk add git go nodejs`

3. Install any development packages required to build Telodendria. For the BSDs, all development tools are built in so no additional packages are necessary. For the Linux distributions I've messed with, install these additional packages:
    - **Debian:** `apt install make gcc libssl-dev`
    - **Alpine:** `apk add make gcc musl-dev openssl-dev`
4. Clone `https://git.telodendria.io/Telodendria/act_runner.git`.
5. Run `go build` in the `act_runner` directory. On NetBSD, you may have to `umount /tmp` first because `/tmp` is by default very small. Otherwise, make `/tmp` larger during installation. 2GB should be plenty.
6. Run `./act_runner register` to register the runner. When prompted for the tags, follow following convention:
    - **Linux Distros:** `linux`, `<distro>-v<version>`, `<arch>`
    - **BSD Derivatives:** `bsd`, `<osname>-v<version>`, `<arch>`
    - **Windows:** `windows`, `windows-v<version>`, `<arch>`
    - **MacOS:** `macos`, `macos-v<version>`, `<arch>`
    - **Others:** `other`, `<osname>-v<version>`, `<arch>`

    Where `<arch>` is one of `x86` or `x64` for now. ARM runners will be a future project.
7. Run `./act_runner daemon`.

### Startup Scripts

We will obviously want `act_runner` to execute on bootup. Here are the start scripts I used:

#### Alpine

In `/etc/init.d/act_runner`:

```shell
#!/sbin/openrc-run

directory="/home/runner/act_runner"
command="/home/runner/act_runner/act_runner"
command_args="daemon"
command_user="runner:runner"
command_background="true"
pidfile="/run/act_runner.pid"
```

Don't forget to `chmod +x /etc/init.d/act_runner`.

Then just `rc-update add act_runner` and `rc-service act_runner start`.

#### Debian

In `/etc/systemd/system/act_runner.service`:

```
[Unit]
Description=Gitea Actions runner

[Service]
ExecStart=/home/runner/act_runner/act_runner daemon
ExecReload=/bin/kill -s HUP $MAINPID
WorkingDirectory=/home/runner/act_runner
TimeoutSec=0
RestartSec=10
Restart=always
User=runner

[Install]
WantedBy=multi-user.target
```

Then just `systemctl enable act_runner` and `systemctl start act_runner`.

#### Other

Eventually I got sick of writing init scripts for all the various operating systems.

Just put this in `runner`'s `crontab`:

```
@reboot cd /home/runner/act_runner && ./act_runner daemon
```

That seems to do the job good enough, and it's cross platform.