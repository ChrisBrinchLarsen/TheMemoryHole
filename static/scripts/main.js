function sendProgram() {
    programText = document.getElementById("programText").value;
    console.log(programText);
    document.getElementById("programText").value = "";
    socket.emit("run_program", {program:programText, config:"Hey"})
}

INSTR.innerHTML = "add 3 13 24"
add = {readers:[1, 2, 3], writers:[7, 8, 9]}

visualizeInstr(add);


function visualizeInstr(instruction) {
    clearRegisters();
    instruction.readers.forEach(reader => {R[reader].classList.toggle("read-from");});
    instruction.writers.forEach(writer => {R[writer].classList.toggle("written-to");});
}

function clearRegisters() {
    R.forEach(reg => {
        reg.classList.remove("read-from");
        reg.classList.remove("written-to");
    });
}