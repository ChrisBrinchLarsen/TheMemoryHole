# Running the Simulator
1. `$ git clone https://github.com/ChrisBrinchLarsen/TheMemoryHole.git`
2. You will need the rars cross compiler installed and placed into a `riscv` directory at the root of the repository. Download release `rv32i-131023`from the [RARS repository](https://github.com/stnolting/riscv-gcc-prebuilt?tab=readme-ov-file)
3. Extract the tar file and rename the directory to `riscv`
4. Move the entire directory to the root of the repository, such that the path to is is `TheMemoryHole/riscv/`. For clarity, the extracted directory you renamed `riscv` should contain the subdirectories: `bin/`, `include/`, `lib/`, `libexec/`, `riscv32-unknown-elf/` and `share/`.
5. `$ cd TheMemoryHole/engine`
6. `$ make sim`
7. `cd ..`
8. Set up a python virtual environment
9. Run `$ pip install -r requirements.txt` while in the root directory of the repository, and while being inside of your virtual environment. This will set up the required packages for our flask-socketIO communication with the frontend.
10. With the virtual environment active, run `$python backend.py` in the root directory of the repository.
11. The frontend is now up and running on `http://localhost:5000/`

# Running Unit Tests
Running the unit tests boils down to the following steps:
1. `$ git clone https://github.com/ChrisBrinchLarsen/TheMemoryHole.git`
2. `$ cd TheMemoryHole/engine/`
3. `$ make unit`
4. `$ ./unit`