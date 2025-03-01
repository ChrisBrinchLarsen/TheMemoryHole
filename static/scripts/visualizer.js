function visualize(config, load_log, exec_log) {
    visualizeStep(exec_log[0])
    for (let i = 1; i < exec_log.length; i += 1) {
        setTimeout(() => {
            visualizeStep(exec_log[i]);
        }, 250);
    }

    visualizeStep(exec_log[1])
}

function visualizeStep(step) {
    console.log(step)
    document.getElementById("instr").innerHTML = step["title"]
    visualizeInstr(step["readers"], step["writers"])
}

function visualizeInstr(readers, writers) {
    clearRegisters();
    readers.forEach(reader => {R[reader].classList.toggle("read-from"); reg_changed = true});
    writers.forEach(writer => {R[writer].classList.toggle("written-to"); reg_changed = true});
}

reg_changed = true
function clearRegisters() {
    if (!reg_changed) {return}
    R.forEach(reg => {
        reg.classList.remove("read-from");
        reg.classList.remove("written-to");
    });
    reg_changed = false
}