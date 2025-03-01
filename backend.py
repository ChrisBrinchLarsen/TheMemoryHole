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


@app.route('/')
def index():
    return render_template('index.html')

@app.route('/visualizer')
def visualizer():
    return render_template('visualizer.html')

@app.route("/get_logs")
def get_logs():
    return [[], session["loading_prog"], session["executing_prog"]]

# TODO: Very important that we make the config and program filenames be unique
# and random, as well as have them cleaned ud (deleted) after use. This is due
# to multithreading if multiple people are requesting at the same time we're
# currently vulnurable to a race condition.
@socketio.on("run_program")
def handle_run_program(data):
    config = data["config"]
    program = data["program"]
    N_CACHE_LEVELS = len(config)
    args = ""
    id = uuid.uuid4().hex
    architecture_file_name = f"./tmp/architecture_{id}"
    program_file_path = f"./tmp/program_{id}"

    with open(architecture_file_name, "w") as file:
        file.write(f"{N_CACHE_LEVELS}\n")
        for i in range(N_CACHE_LEVELS):
            file.write(f"L{i+1}\n")
            file.write(f"{config[i]["p"]}\n")
            file.write(f"{config[i]["q"]}\n")
            file.write(f"{config[i]["k"]}\n")
            file.write(f"{config[i]["a"]}\n")

    with open(f"{program_file_path}.c", 'w') as file: file.write(program)

    C_to_dis(program_file_path)

    result = subprocess.run(["./engine/sim", architecture_file_name, f"{program_file_path}.dis", "--", f"{args}"], capture_output=True, text=True)
    print(result.stdout)

    # TODO: Add parsing and sending back each iteration from cache_log to frontend
    loading_instr = []
    executing_prog = []
    with open("cache_log", "r") as log:
        line = log.readline()
        while (line != "---- PROGRAM START ----\n"): # Writing program to memory
            line = log.readline()
        while (True): # Executing program
            step = {"type":"", "title":"", "ram":False, "hits":[], "misses":[], "readers":[], "writers":[]}
            line = log.readline()
            if (not line): break
            tokens = line.split()
            match tokens[0]:
                case "fetch:":
                    step["type"] = "fetch"
                    step["title"] = f"Fetching instruction {tokens[2]}"
                    line = log.readline()
                    tokens = line.split()
                    while (line != "endfetch\n"):
                        if line == "RAM\n":
                            step["ram"] = True
                            line = log.readline()
                            tokens = line.split()
                            continue
                        match tokens[1]:
                            case "H":
                                step["hits"].append((tokens[0], tokens[2]))
                            case "M":
                                step["misses"].append((tokens[0], tokens[2]))
                            case "E":
                                pass
                        line = log.readline()
                        tokens = line.split()
                case "instr:":
                    step["type"] = "instr"
                    step["title"] = log.readline()[:-1] # This might have an \n at the end that we would want to trim off
                    if (step["title"] == "endinstr"): continue
                    line =  log.readline()
                    tokens = line.split()
                    while (line != "endinstr\n"):
                        if line == "RAM\n":
                            step["ram"] = True
                            line = log.readline()
                            tokens = line.split()
                            continue
                        match tokens[1]:
                            case "H":
                                step["hits"].append((tokens[0], tokens[2]))
                            case "M":
                                step["misses"].append((tokens[0], tokens[2]))
                            case "E":
                                pass
                            case _:
                                match tokens[0]:
                                    case "r":
                                        step["readers"].append(tokens[1])
                                    case "w":
                                        step["writers"].append(tokens[1])
                        # Since there's no default case, stuff like 'ww 0xffa630 1' is actually ignored since the results of that operation are implicitly known by the cache misses and hits
                        line = log.readline()
                        tokens = line.split()
            executing_prog.append(step)

    os.system(f"rm -f accesses {program_file_path}.riscv {program_file_path}.dis {program_file_path}.c {architecture_file_name}")

    return [], loading_instr, executing_prog


def C_to_dis(program_file_path):
     # Compile from C -> RISC-V
    os.system(f"./riscv/bin/riscv32-unknown-elf-gcc -march=rv32im -mabi=ilp32 -fno-tree-loop-distribute-patterns -mno-relax -O1 {program_file_path}.c lib.c -static -nostartfiles -nostdlib -o {program_file_path}.riscv")
    # Compile from RISC-V -> dis
    os.system(f"./riscv/bin/riscv32-unknown-elf-objdump -s -w {program_file_path}.riscv > {program_file_path}.dis")
    os.system(f"./riscv/bin/riscv32-unknown-elf-objdump -S {program_file_path}.riscv >> {program_file_path}.dis")

### MAIN
if __name__ == "__main__":
    socketio.run(app, debug=True)