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
    print("what")
    args = ""
    id = uuid.uuid4()
    architecture_file_name = f"./tmp/architecture_{id}"
    program_file_name = f"./tmp/program_{id}"
    architecture_file_name = "./engine/testing/Architectures/SimpleCPU.md"

    # with open("config", "w") as file:
    #     file.write(f"{config["addr_len"]}\n")
    #     file.write(f"{config["word_size"]}\n")
    #     file.write(f"{config["words_per_block"]}\n")
    #     cache_levels: int = int(config["cache_levels"])
    #     file.write(f"{cache_levels}\n")
    #     for i in range(cache_levels):
    #         file.write(f"{config["caches"][i]["name"]}\n")
    #         file.write(f"{config["caches"][i]["size"]}\n")
    #         file.write(f"{config["caches"][i]["associativity"]}\n")

    # with open(architecture_file_name, "w") as file:
    #     file.write(f"32\n")
    #     file.write(f"1\n")
    #     file.write(f"8\n")
    #     cache_levels: int = 2
    #     file.write(f"{cache_levels}\n")
    #     file.write(f"L1\n")
    #     file.write(f"128\n")
    #     file.write(f"2\n")
    #     file.write(f"L2\n")
    #     file.write(f"256\n")
    #     file.write(f"4\n")

    with open(f"{program_file_name}.c", 'w') as file: file.write(program)

    # Compile from C -> RISC-V
    os.system(f"./riscv/bin/riscv32-unknown-elf-gcc -march=rv32im -mabi=ilp32 -fno-tree-loop-distribute-patterns -mno-relax -O1 {program_file_name}.c lib.c -static -nostartfiles -nostdlib -o {program_file_name}.riscv")
    # Compile from RISC-V -> dis
    os.system(f"./riscv/bin/riscv32-unknown-elf-objdump -s -w {program_file_name}.riscv > {program_file_name}.dis")
    os.system(f"./riscv/bin/riscv32-unknown-elf-objdump -S {program_file_name}.riscv >> {program_file_name}.dis")

    result = subprocess.run(["./engine/sim", architecture_file_name, f"{program_file_name}.dis", "--", f"{args}"], capture_output=True, text=True)
    print(result.stdout)
    os.system(f"rm -f accesses cache_log {program_file_name}.riscv {program_file_name}.dis {program_file_name}.c")



### MAIN
if __name__ == "__main__":
    socketio.run(app, debug=True)