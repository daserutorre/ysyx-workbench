# npc/nvboard-rtl

This folder contains warm-up RTL exercises using Verilator and NVBoard:
a two-way switch (combinational logic) and a running-lights circuit
(sequential logic), simulated and visualized on a virtual FPGA board.

## Files

```
nvboard-rtl/
├── constr/
│   └── top.nxdc        # pin constraints (maps Verilog ports to NVBoard pins)
├── csrc/
│   └── main.cpp         # C++ testbench: drives the simulated circuit + NVBoard
├── vsrc/
│   └── top.v             # the actual circuit (Verilog)
├── Makefile
└── run_nvboard.sh      # convenience launcher script
```

## Required dependencies

### 1. Verilator

Install a recent version (5.x). The course recommends building `stable`
from source rather than using the `apt` package, since that's usually
outdated:

```bash
verilator --version   # should report a recent version, e.g. 5.x
```

If not installed, build from source (outside `ysyx-workbench`, so git
doesn't track Verilator's own source inside the project):

```bash
cd ~
git clone https://github.com/verilator/verilator
cd verilator
git checkout stable
# follow the official build instructions on verilator.org
```

Build dependencies:
```bash
sudo apt-get update
sudo apt-get install git help2man perl python3 make autoconf g++ flex bison ccache
sudo apt-get install libgoogle-perftools-dev numactl perl-doc
sudo apt-get install libfl2
sudo apt-get install libfl-dev
sudo apt-get install zlibc zlib1g zlib1g-dev
```

### 2. SDL2 (required by NVBoard's GUI)

```bash
sudo apt-get install libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev
```

(macOS: `brew install sdl2 sdl2_image sdl2_ttf`)

### 3. GTKWave (for viewing FST/VCD waveforms)

```bash
sudo apt-get install gtkwave
```

> **Known issue:** if GTKWave fails to launch with
> `symbol lookup error: .../snap/core20/.../libpthread.so.0: undefined symbol ...`,
> it's usually caused by VS Code's snap environment injecting `GTK_PATH`
> into the shell. Fix with:
> ```bash
> unset GTK_PATH
> gtkwave wave.fst
> ```

### 4. NVBoard

NVBoard is a git submodule of `ysyx-workbench`, backed by
`git@github.com:daserutorre/nvboard.git`. Make sure it's cloned:

```bash
cd ~/Documents/ysyx-workbench
git submodule update --init --recursive
```

Then set `NVBOARD_HOME`:

```bash
echo 'export NVBOARD_HOME=/home/daserutorre/Documents/ysyx-workbench/nvboard' >> ~/.bashrc
source ~/.bashrc
echo $NVBOARD_HOME
```

> **New machine / new username?** `NVBOARD_HOME` and `NPC_HOME` are
> hardcoded absolute paths in `~/.bashrc`. If your username differs from
> the one baked into an old `.bashrc` (e.g. `nur` vs `daserutorre`), fix
> with:
> ```bash
> grep -n "daserutorre" ~/.bashrc      # or your old username
> sed -i 's|/home/OLD_USER|/home/NEW_USER|g' ~/.bashrc
> source ~/.bashrc
> echo $NVBOARD_HOME
> echo $NPC_HOME
> ```

## Commands to run

### Build and run the simulation

```bash
cd ~/Documents/ysyx-workbench/npc/nvboard-rtl
make sim
```

or, using the convenience script (auto-detects its own directory, so it
works from anywhere):

```bash
./run_nvboard.sh
```

This will:
1. Run the required git-tracking commit (`$(call git_commit, "sim RTL")`) —
   do not remove this line from the Makefile.
2. Compile the Verilog + C++ testbench with Verilator.
3. Launch the executable, opening an NVBoard window showing the circuit
   live (switches/LEDs you can click and observe).

### Clean build artifacts

```bash
make clean
```
or
```bash
./run_nvboard.sh clean
```

### Lint-check the RTL (static analysis)

```bash
verilator --lint-only -Wall vsrc/top.v --top-module top
```

### View waveforms

The simulation dumps `wave.fst` (FST format, ~1/50 the size of VCD).
Open it with:

```bash
gtkwave wave.fst
```