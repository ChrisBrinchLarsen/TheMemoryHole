simpleCPU = [{p:6,q:1,k:4,a:1}, {p:7,q:1,k:4,a:2}]
function sendProgram() {
    PROGRAM_TEXT = document.getElementById("programText").value;
    document.getElementById("programText").value = "";
    socket.emit("run_program", {program:PROGRAM_TEXT, config:confirmArchitecture(), args:document.getElementById("args").value}, runCallback)
}

function runCallback(load_log, exec_log) {
    create_caches()
    src_lines = PROGRAM_TEXT.replaceAll("<", "&lt")
                            .replaceAll(">", "&gt")
                            .split("\n");
    CODE_VIEWER.innerHTML = "";
    for (let i = 0; i < src_lines.length; i++) {
        line = document.createElement("div");
        line.classList.add("src-line");
        line.innerHTML = src_lines[i];
        if (line.innerHTML == "") {line.innerHTML = " "}
        CODE_VIEWER.appendChild(line);
        SRC_LINES.push(line);
    }
    ADDRESS_OBJECTS = document.querySelectorAll(".split_addr");
    INPUT_PAGE.style.display = "none";
    VISUALIZATION_PAGE.style.display = "flex";
    LOAD_LOG = load_log
    EXEC_LOG = exec_log
    TOTAL_STEPS = load_log.length + exec_log.length
    visualize()
}

function confirmArchitecture() {
    cacheList = ARCHITECTURE.querySelectorAll(".cache-container");
    N_CACHE_LAYERS = cacheList.length
    cacheList.forEach(cache => {
        settings = cache.querySelectorAll(".num-box")
        CONFIG.push({p:settings[0].value
                    ,q:settings[1].value
                    ,k:settings[2].value
                    ,a:settings[3].value})
        block_offset_len = Math.log2(Math.pow(2, settings[2].value))
        set_len = Math.log2((Math.pow(2,settings[0].value) * settings[1].value) / (Math.pow(2, settings[2].value) * settings[3].value))
        BIT_LENGTHS.push({s:set_len
                         ,b:block_offset_len})
    })

    return CONFIG
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

{/* <select name="cars" id="cars">
    <option value="1">1-way</option>
    <option value="2">2-way</option>
    <option value="4">4-way</option>
    <option value="8">8-way</option>
    <option value="16">16-way</option>
    <option value="32">32-way</option>
    <option value="64">64-way</option>
    <option value="128">128-way</option>
    <option value="256">256-way</option>
    <option value="512">512-way</option>
  </select> */}