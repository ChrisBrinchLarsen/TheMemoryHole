# Running the Simulator
1. `$ git clone https://github.com/ChrisBrinchLarsen/TheMemoryHole.git`
2. You will need the rars cross compiler installed and placed into a `riscv` directory at the root of the repository. Download release `rv32i-131023`from the [RARS repository](https://github.com/stnolting/riscv-gcc-prebuilt?tab=readme-ov-file)
3. Extract the tar file and rename the directory to `riscv`
4. Move the entire directory to the root of the repository, such that the path to it is `TheMemoryHole/riscv/`. For clarity, the extracted directory you renamed `riscv` should contain the subdirectories: `bin/`, `include/`, `lib/`, `libexec/`, `riscv32-unknown-elf/` and `share/`.
5. `$ cd TheMemoryHole/engine`
6. `$ make sim`
7. `cd ..`
8. Set up a python virtual environment
9. Run `$ pip install -r requirements.txt` while in the root directory of the repository, and while being inside of your virtual environment. This will set up the required packages for our flask-socketIO communication with the frontend.
10. With the virtual environment active, run `$ python backend.py` in the root directory of the repository.
11. The frontend is now up and running on `http://localhost:5000/`

# Frontend Legend
As the program is running, the cache lines light change colors. Here's what each of them mean:

| Color    | Description |
| - | - |
| White fill | Invalid line |
| Gray fill | Valid line |
| Dashed | Dirty line |
| Green fill | Line hit |
| Red outline | Miss in set |
| Blue fill | Insertion victim |
| Orange fill | Eviction to line |

# Lib.h
The simulator only supports one header file which is the included lib.h. Below are all its functions:
| Returns | Function | Description |  
| - | - | - |
| int | read_int_buffer(int file, int* buffer, int max_size); | reads from file |
| int | write_int_buffer(int file, int* buffer, int size); | writes to file |
| void | outp(char c); | prints char to stdout |
| void | print_string(const char* p); | prints string to stdout |
| uint | str_to_uns(const char* str); | string to uint |
| int | uns_to_str(char* buffer, unsigned int val); | uint to string |
| void* | allocate(int size); | malloc() equivalent |
| void | release(void* mem); | free() equivalent |
| uint | rand_uint(); | random uint in [0, 65536]  |
| int | open_file(char* path, char* flags); | opens a file |
| int | close_file(int file); | closes a file |
        


# Running Unit Tests
Running the unit tests boils down to the following steps:
1. `$ git clone https://github.com/ChrisBrinchLarsen/TheMemoryHole.git`
2. `$ cd TheMemoryHole/engine/`
3. `$ make unit`
4. `$ ./unit`