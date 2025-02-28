simpleCPU = [{p:6,q:1,k:4,a:1}, {p:7,q:1,k:4,a:2}]
function sendProgram() {
    programText = document.getElementById("programText").value;
    document.getElementById("programText").value = "";
    socket.emit("run_program", {program:programText, config:confirmArchitecture()})
}

function confirmArchitecture() {
    cacheList = ARCHITECTURE.querySelectorAll(".cache-container");
    N_CACHE_LAYERS = cacheList.length
    config = []
    cacheList.forEach(cache => {
        settings = cache.querySelectorAll(".num-box")
        config.push({p:settings[0].value
                    ,q:settings[1].value
                    ,k:settings[2].value
                    ,a:settings[3].value})
    })
    return config
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

function renameCaches() {
    titles = ARCHITECTURE.querySelectorAll(".cache-title")
    for (let i = 1; i <= titles.length; i++) {
        titles[i-1].innerHTML = "L" + i
    }
}

function removeCache(button) {
    button.parentElement.parentElement.remove()
    renameCaches()
}

function addCache() {
    container = document.createElement("div")
    container.classList.add("cache-container")

    container.innerHTML = `
        <div class="cache-header">
            <h2 class="cache-title">L1</h2>
            <button onclick="removeCache(this)">X</button>
        </div>
        <div class="cache-setting">
            Cache size: 2^<input class="num-box" type="number"> * <input class="num-box" type="number"> bytes
        </div>
        <div class="cache-setting">
            Block size: 2^<input class="num-box" type="number"> bytes
        </div>
        <div class="cache-setting">
            Associativity: <input class="num-box" type="number">-way
        </div>
    `
    ARCHITECTURE.insertBefore(container, ADD_CACHE)
    renameCaches()
}