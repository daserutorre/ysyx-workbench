# "一生一芯"工程项目

这是"一生一芯"的工程项目. 通过运行
```bash
bash init.sh subproject-name
```
进行初始化, 具体请参考[实验讲义][lecture note].

[lecture note]: https://ysyx.oscc.cc/docs/

## NVBoard

`nvboard` 备份在我自己的 GitHub 仓库中, 并作为 **git submodule** 注册在本仓库里
(`git@github.com:daserutorre/nvboard.git`), 所以 clone 本仓库时需要加上
`--recurse-submodules`:

```bash
git clone --recurse-submodules git@github.com:daserutorre/ysyx-workbench.git
```

如果已经 clone 过了(忘记加 `--recurse-submodules`), 可以事后拉取子模块:

```bash
cd ysyx-workbench
git submodule update --init --recursive
```

原始上游仓库(如需同步更新)地址为:
```
https://github.com/NJU-ProjectN/nvboard
```

`NVBOARD_HOME` 环境变量仍需手动设置(子模块只负责把代码拉下来, 不会自动导出环境变量):

```bash
echo 'export NVBOARD_HOME=/home/daserutorre/Documents/ysyx-workbench/nvboard' >> ~/.bashrc
source ~/.bashrc
echo $NVBOARD_HOME
```

> **换设备/换用户名时注意:** `NVBOARD_HOME`(以及 `NPC_HOME`)在 `~/.bashrc`
> 中是以绝对路径硬编码的, 包含具体用户名(如 `/home/daserutorre/...`)。
> 如果把 `.bashrc` 拷贝到用户名不同的设备上(例如变成 `nur`), 这些路径会
> 静默失效, 构建时会报错, 例如:
> ```
> No such file or directory
> No rule to make target '.../nvboard/scripts/nvboard.mk'
> ```
> 修复方法: 检查并替换旧路径
> ```bash
> grep -n "daserutorre" ~/.bashrc      # 换成你的旧用户名
> sed -i 's|/home/OLD_USER|/home/NEW_USER|g' ~/.bashrc
> source ~/.bashrc
> echo $NVBOARD_HOME
> echo $NPC_HOME
> ```