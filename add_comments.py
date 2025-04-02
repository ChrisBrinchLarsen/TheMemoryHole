import sys

def addDebugComments(program:str):
    outstr = "//PROGRAM_START\n";
    lines = program.splitlines();
    for i in range(len(lines)):
        outstr += lines[i] + f"//[[{i}]]\n";
    outstr += "//PROGRAM_END\n"
    return outstr;


filename = sys.argv[1]
pg = ""
with open(filename, "r") as file:
    pg = file.read()

with open(filename.split(".")[0] + "_commented.c", "w") as file:
    file.write(addDebugComments(pg))