# "One Student One Chip" Engineering Project

This is the engineering project for "One Student One Chip" (一生一芯). Initialize
subprojects by running:
```bash
bash init.sh subproject-name
```
See the [lecture notes][lecture note] for details.

[lecture note]: https://ysyx.oscc.cc/docs/

## NVBoard

`nvboard` is backed up in my own GitHub repository and registered as a
**git submodule** in this repo (`git@github.com:daserutorre/nvboard.git`), so
cloning this repository requires the `--recurse-submodules` flag:

```bash
git clone --recurse-submodules git@github.com:daserutorre/ysyx-workbench.git
```

If you already cloned without that flag, fetch the submodule afterwards:

```bash
cd ysyx-workbench
git submodule update --init --recursive
```

The original upstream repo (if you need to sync updates) is at:
```
https://github.com/NJU-ProjectN/nvboard
```

The `NVBOARD_HOME` environment variable still needs to be set manually (the
submodule only pulls the code down — it doesn't export environment variables
on its own):

```bash
echo 'export NVBOARD_HOME=/home/daserutorre/Documents/ysyx-workbench/nvboard' >> ~/.bashrc
source ~/.bashrc
echo $NVBOARD_HOME
```

> **Note (new machine / new username):** `NVBOARD_HOME` (and `NPC_HOME`) are
> exported in `~/.bashrc` with an **absolute, hardcoded path** including your
> username (e.g. `/home/daserutorre/...`). If you copy `.bashrc` to a
> different machine where your username differs (e.g. `nur` instead of
> `daserutorre`), these paths will silently point nowhere and builds will
> fail with errors like:
> ```
> No such file or directory
> No rule to make target '.../nvboard/scripts/nvboard.mk'
> ```
> Fix by checking for stale paths and replacing them:
> ```bash
> grep -n "daserutorre" ~/.bashrc      # or your old username
> sed -i 's|/home/OLD_USER|/home/NEW_USER|g' ~/.bashrc
> source ~/.bashrc
> echo $NVBOARD_HOME
> echo $NPC_HOME
> ```