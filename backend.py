from flask import Flask, render_template, request, session
from flask_socketio import SocketIO, emit
import os
import logging
import uuid
import subprocess

app = Flask(__name__)
app.config['SECRET_KEY'] = uuid.uuid4().hex
log = logging.getLogger('werkzeug')
log.disabled = True
socketio = SocketIO(app)

CHUNK_SIZE = 15000

policy_dict = {
    "LRU" : 0,
    "RR"  : 1,
    "LIFO": 2,
    "FIFO": 3,
    "MRU" : 4,
}

@app.route('/')
def index():
    return render_template('index.html')

@app.route("/get_logs")
def get_logs():
    return [[], session["loading_prog"], session["executing_prog"]]


def write_cache_to_file(file, name, config):
    file.write(f"{name}\n")
    file.write(f"{config['p']}\n")
    file.write(f"{config['q']}\n")
    file.write(f"{config['k']}\n")
    file.write(f"{config['a']}\n")

@socketio.on("more_steps")
def request_more_steps(cookie):
    exec_log, cookie = read_n_log_steps(CHUNK_SIZE, cookie)
    return exec_log, cookie

# TODO: Very important that we make the config and program filenames be unique
# and random, as well as have them cleaned ud (deleted) after use. This is due
# to multithreading if multiple people are requesting at the same time we're
# currently vulnurable to a race condition.
# NOTE: This function expects a dictionary with specific entries present as the input
@socketio.on("run_program")
def handle_run_program(data):
    print("Got message from server")
    data_caches = data["data_caches"]
    instr_cache = data["instr_cache"]
    r_policy = policy_dict[data["r_policy"]]
    program = data["program"]
    N_CACHE_LEVELS = len(data_caches)
    args = data["args"].split(" ")
    id = uuid.uuid4().hex
    architecture_file_name = f"./tmp/architecture_{id}"
    program_file_path = f"./tmp/program_{id}"

    with open(architecture_file_name, "w") as file:
        file.write(f"{N_CACHE_LEVELS}\n")
        file.write(f"{r_policy}\n")
        if instr_cache:
            print("Instruction cache was detected")
            write_cache_to_file(file, "i", instr_cache)
        for i in range(N_CACHE_LEVELS):
            write_cache_to_file(file, f"L{i+1}", data_caches[i])

    program = addDebugComments(program)

    with open(f"{program_file_path}.c", 'w') as file: file.write(program)

    C_to_dis(program_file_path)
    print("Starting program simulation...")
    os.system(f"rm -f accesses loggers cache_log step_count")
    print("policy", r_policy)
    result = subprocess.run(["./engine/sim", architecture_file_name, f"{program_file_path}.dis", "-l", "loggers", "--", *args], capture_output=True, text=True)
    print("...Finished program simulation")
    print("Stdout:")
    print(result.stdout)
    print("Stderr:")
    print(result.stderr)
    print("Retval:")
    print(result.returncode)

    n_steps = 0
    with open("step_count", "r") as f:
        n_steps = int(f.readline())
        print("Total steps:", n_steps)

    print("Parsing cache_log starting...")

    cookie = 0
    with open("cache_log", "r") as log:
        log.seek(0, os.SEEK_SET)
        cookie = log.tell()

    executing_prog, cookie = read_n_log_steps(CHUNK_SIZE, cookie)
   
    os.system(f"rm -f {program_file_path}.riscv {program_file_path}.dis {program_file_path}.c {architecture_file_name} tmp/program_* tmp/architecture_*")
    return executing_prog, n_steps, cookie


def C_to_dis(program_file_path):
     # Compile from C -> RISC-V
    os.system(f"./riscv/bin/riscv32-unknown-elf-gcc -march=rv32im -mabi=ilp32 -fno-tree-loop-distribute-patterns -mno-relax -Og {program_file_path}.c lib.c -static -nostartfiles -nostdlib -o {program_file_path}.riscv -g")
    # Compile from RISC-V -> dis
    os.system(f"./riscv/bin/riscv32-unknown-elf-objdump -s -w {program_file_path}.riscv > {program_file_path}.dis")
    os.system(f"./riscv/bin/riscv32-unknown-elf-objdump -S {program_file_path}.riscv >> {program_file_path}.dis")

def addDebugComments(program:str):
    outstr = "//PROGRAM_START\n";
    lines = program.splitlines();
    for i in range(len(lines)):
        outstr += lines[i] + f"//[[{i}]]\n";
    outstr += "//PROGRAM_END\n"
    return outstr;

def read_n_log_steps(n: int, cookie):
    ret_array = []
    active_lines = []
    
    with open("cache_log", "r") as log:
        log.seek(cookie, os.SEEK_SET)
        i = 0
        while (i < n):
            step = {"type":"", "title":"", "CS":0, "ram":False, "hits":[], "misses":[], "readers":[], "writers":[], "addr":[], "evict":[], "insert":[], "validity":[], "dirtiness":[], "lines":active_lines, "lines-changed":False, "is_write":False, "stdout":0}
            line = log.readline()
            if (not line): break
            tokens = line.split()
            match tokens[0]:
                case "fetch:":
                    step["type"] = "fetch"
                    step["title"] = f"Fetching instruction {tokens[2]}"
                    step["addr"].append(int(tokens[2], 16))
                    line = log.readline()
                    tokens = line.split()
                    while (line != "endfetch\n"):
                        if line == "RAM\n":
                            step["ram"] = True
                            line = log.readline()
                            tokens = line.split()
                            continue
                        match tokens[0]:
                            case "H":
                                step["hits"].append((tokens[1], tokens[2], tokens[3]))
                            case "M":
                                step["misses"].append((tokens[1], tokens[2]))
                            case "E":
                                step["evict"].append((tokens[1], tokens[2], tokens[3]))
                            case "F": # Yes I know F (fetch into cache) being insert is weird asf
                                step["insert"].append((tokens[1], tokens[2], tokens[3]))
                            case "V" | "IV":
                                step["validity"].append((tokens[1], tokens[2], tokens[3], tokens[0] == "V", tokens[4]))
                            case "D" | "C":
                                step["dirtiness"].append((tokens[1], tokens[2], tokens[3], tokens[0] == "D", tokens[4]))
                        line = log.readline()
                        tokens = line.split()
                case "instr:":
                    step["type"] = "instr"
                    step["title"] = log.readline()[:-1]
                    if (step["title"] == "endinstr"): continue
                    line =  log.readline()
                    tokens = line.split()
                    while (line != "endinstr\n"):
                        if line == "RAM\n":
                            step["ram"] = True
                            line = log.readline()
                            tokens = line.split()
                            continue
                        match tokens[0]:
                            case "CS": step["CS"] = tokens[1]
                            case "stdout": step["stdout"] = tokens[1]
                            case "H": step["hits"].append((tokens[1], tokens[2], tokens[3]))
                            case "M": step["misses"].append((tokens[1], tokens[2]))
                            case "E": step["evict"].append((tokens[1], tokens[2], tokens[3]))
                            case "F": step["insert"].append((tokens[1], tokens[2], tokens[3]))
                            case "V" | "IV": step["validity"].append((tokens[1], tokens[2], tokens[3], tokens[0] == "V", tokens[4]))
                            case "D" | "C": step["dirtiness"].append((tokens[1], tokens[2], tokens[3], tokens[0] == "D", tokens[4]))
                            case _:
                                match tokens[0]:
                                    case access if access in ["wb", "wh", "ww", "rb", "rh", "rw"]:
                                        step["addr"].append(int(tokens[1], 16))
                                        step["is_write"] = access in ["wb", "wh", "ww"]
                                    case "r": step["readers"].append(tokens[1])
                                    case "w": step["writers"].append(tokens[1])
                                    case "pc":
                                        active_lines = (tokens[2], tokens[3])
                                        step["lines-changed"] = True
                        # Since there's no default case, stuff like 'ww 0xffa630 1' is actually ignored since the results of that operation are implicitly known by the cache misses and hits
                        line = log.readline()
                        tokens = line.split()
            step["lines"] = active_lines
            ret_array.append(step)
            i += 1
        cookie = log.tell()
    return ret_array, cookie



### MAIN
if __name__ == "__main__":
    socketio.run(app, debug=True)
