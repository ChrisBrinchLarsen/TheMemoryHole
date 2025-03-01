simpleCPU = [{p:6,q:1,k:4,a:1}, {p:7,q:1,k:4,a:2}]
function sendProgram() {
    programText = document.getElementById("programText").value;
    document.getElementById("programText").value = "";
    socket.emit("run_program", {program:programText, config:confirmArchitecture()}, runCallback)
}

function runCallback(config, load_log, exec_log) {
    INPUT_PAGE.style.display = "none";
    VISUALIZATION_PAGE.style.display = "flex";
    visualize(config, load_log, exec_log)
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