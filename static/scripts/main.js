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
        let line = document.createElement("div");
        line.setAttribute("data-nr", i+1);
        line.onclick = function () {
            SELECTED_LINE = i
            SRC_LINES.forEach(innerLine => {innerLine.classList.remove("selected")})
            line.classList.add("selected")
            SUMMARY.style.display = "none"
            LINE_SUMMARY.style.display = "flex"
            SUMMARY_LINE_NR.innerHTML = i+1
            updateLineSummary(i)
        }
        line.classList.add("src-line");
        line.innerHTML = src_lines[i];
        if (line.innerHTML == "") {line.innerHTML = " "}
        CODE_VIEWER.appendChild(line);
        SRC_LINES.push(line);
        LINE_HITS.push(0)
        LINE_MISSES.push(0)
    }

    ADDRESS_OBJECTS = document.querySelectorAll(".split_addr");

    for (let i = 0; i < CONFIG.length; i++) {
        cache_stats = document.createElement("div");
        cache_stats.innerHTML = `
            <div>L${i+1}: <span class="cache-miss-percent"></span>% (<span class="cache-hits"></span>H/<span class="cache-misses"></span>M)</div>
        `
        SUMMARY.appendChild(cache_stats)
    }

    combined_missrate_object = document.createElement("div");
    combined_missrate_object.innerHTML = `Cache Miss-rate: <span id="cache-miss-rate"></span>%`
    SUMMARY.appendChild(combined_missrate_object);

    CACHE_MISS_RATE = document.getElementById("cache-miss-rate")
    CACHE_HIT_COUNTER_OBJECTS = SUMMARY.querySelectorAll(".cache-hits")
    CACHE_MISS_COUNTER_OBJECTS = SUMMARY.querySelectorAll(".cache-misses")
    CACHE_PERCENT_OBJECTS = SUMMARY.querySelectorAll(".cache-miss-percent")

    INPUT_PAGE.style.display = "none";
    VISUALIZATION_PAGE.style.display = "flex";
    LOAD_LOG = load_log
    EXEC_LOG = exec_log
    TOTAL_STEPS = load_log.length + exec_log.length
    INSTR_COUNTER.innerHTML = "(0/" + TOTAL_STEPS + ") "
}

function confirmArchitecture() {
    cacheList = ARCHITECTURE.querySelectorAll(".cache-container");
    N_CACHE_LAYERS = cacheList.length
    
    cacheList.forEach(cache => {
        // Setting these counters up for later
        HITS.push(0);
        MISSES.push(0);

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
            <button onclick="removeCache(this)" class="x-button">X</button>
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

function preset_architecture(caches) {
    ARCHITECTURE.innerHTML = ""
    caches.forEach(cache => {
        container = document.createElement("div");
        container.classList.add("cache-container");
        container.innerHTML = `
        <div class="cache-header">
            <h2 class="cache-title">L1</h2>
            <button onclick="removeCache(this)" class="x-button">X</button>
        </div>
        <div class="cache-setting">
            Cache size: 2^<input class="num-box" type="number" value="${cache[0]}"> * <input class="num-box" type="number" value="${cache[1]}"> bytes
        </div>
        <div class="cache-setting">
            Block size: 2^<input class="num-box" type="number" value="${cache[2]}"> bytes
        </div>
        <div class="cache-setting">
            Associativity: <input class="num-box" type="number" value="${cache[3]}">-way
        </div>
        `
        ARCHITECTURE.appendChild(container)
    })
    ARCHITECTURE.appendChild(ADD_CACHE)
    renameCaches()
}

function preset_program(path) {
    fetch(path)
        .then(response => response.text())
        .then(data => {document.getElementById("programText").value = data})
        .catch(error => console.error("Error fetching file:", error));
}