let CONFIG = {}
let LOAD_LOG = []
let EXEC_LOG = []
let TOTAL_STEPS = 0
let CURRENT_STEP = 0

function visualize() {
    console.log(CURRENT_STEP)
    visualizeStep(EXEC_LOG[CURRENT_STEP])
    CURRENT_STEP += 1
    if (CURRENT_STEP == (TOTAL_STEPS - 1)) {return}
    setTimeout(() => {
        visualize();
    }, 250);
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