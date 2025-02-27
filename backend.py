from flask import Flask, render_template
from flask_socketio import SocketIO
import os
import logging
import uuid
import subprocess

app = Flask(__name__)
app.config['SECRET_KEY'] = uuid.uuid4().hex
log = logging.getLogger('werkzeug')
log.disabled = False
socketio = SocketIO(app)


@app.route('/')
def index():
    return render_template('index.html')

# TODO: Very important that we make the config and program filenames be unique
# and random, as well as have them cleaned ud (deleted) after use. This is due
# to multithreading if multiple people are requesting at the same time we're
# currently vulnurable to a race condition.
@socketio.on("run_program")
def handle_run_program(data):
    config = data["config"]
    program = data["program"]
    N_CACHE_LEVELS = len(config)
    print(N_CACHE_LEVELS)
    args = ""
    id = uuid.uuid4()
    architecture_file_name = f"./tmp/architecture_{id}"
    program_file_name = f"./tmp/program_{id}"

    with open(architecture_file_name, "w") as file:
        file.write(f"{N_CACHE_LEVELS}\n")
        for i in range(N_CACHE_LEVELS):
            file.write(f"L{i+1}\n")
            file.write(f"{config[i]["p"]}\n")
            file.write(f"{config[i]["q"]}\n")
            file.write(f"{config[i]["k"]}\n")
            file.write(f"{config[i]["a"]}\n")

    with open(f"{program_file_name}.c", 'w') as file: file.write(program)

    # Compile from C -> RISC-V
    os.system(f"./riscv/bin/riscv32-unknown-elf-gcc -march=rv32im -mabi=ilp32 -fno-tree-loop-distribute-patterns -mno-relax -O1 {program_file_name}.c lib.c -static -nostartfiles -nostdlib -o {program_file_name}.riscv")
    # Compile from RISC-V -> dis
    os.system(f"./riscv/bin/riscv32-unknown-elf-objdump -s -w {program_file_name}.riscv > {program_file_name}.dis")
    os.system(f"./riscv/bin/riscv32-unknown-elf-objdump -S {program_file_name}.riscv >> {program_file_name}.dis")

    result = subprocess.run(["./engine/sim", architecture_file_name, f"{program_file_name}.dis", "--", f"{args}"], capture_output=True, text=True)
    print(result.stdout)

    # TODO: Add parsing and sending back each iteration from cache_log to frontend

    os.system(f"rm -f accesses cache_log {program_file_name}.riscv {program_file_name}.dis {program_file_name}.c {architecture_file_name}")



### MAIN
if __name__ == "__main__":
    socketio.run(app, debug=True)