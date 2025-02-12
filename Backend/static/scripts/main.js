const programField = document.getElementById("programText");

function sendProgram() {
    programText = programField.value;
    console.log(programText);
    programField.value = "";
    socket.emit("run_program", {program:programText, config:"Hey"})
}
