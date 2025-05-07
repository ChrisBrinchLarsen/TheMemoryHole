let data_caches = []
let instr_cache = {}
let N_CACHE_LAYERS = 0
let INSTR_BIT_LENGTHS = {}
let HAS_INSTRUCTION_CACHE = false
let INSTR_HIT_COUNTER_OBJECT = null
let INSTR_MISS_COUNTER_OBJECT = null
let INSTR_PERCENT_OBJECT = null

function sendProgram() {
    confirmArchitecture()
    PROGRAM_TEXT = document.getElementById("programText").value;
    document.getElementById("programText").value = "";

    let policy = document.getElementById("policy-dropdown").value
    socket.emit("run_program", {program:PROGRAM_TEXT, data_caches:data_caches, instr_cache:instr_cache, r_policy:policy, args:document.getElementById("args").value}, runCallback)
}

// This function needs a rewrite
function runCallback(exec_log) {
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

    ADDRESS_OBJECTS = Array.from(document.querySelectorAll(".split_addr")).filter((addr) => addr.previousElementSibling.innerHTML != "L1i");

    if (HAS_INSTRUCTION_CACHE) {
        l1d_stats = document.createElement("div")
        li1_stats = document.createElement("div")
        l1d_stats.innerHTML = `
            <div>L1d: <span class="cache-miss-percent"></span>% (<span class="cache-hits"></span>H/<span class="cache-misses"></span>M)</div>
        `
        li1_stats.innerHTML = `
            <div>L1i: <span class="instr-cache-miss-percent"></span>% (<span class="instr-cache-hits"></span>H/<span class="instr-cache-misses"></span>M)</div>
        `
        SUMMARY.appendChild(l1d_stats)
        SUMMARY.appendChild(li1_stats)
        for (let i = 1; i < N_CACHE_LAYERS; i++) {
            cache_stats = document.createElement("div");
            cache_stats.innerHTML = `
                <div>L${i+1}: <span class="cache-miss-percent"></span>% (<span class="cache-hits"></span>H/<span class="cache-misses"></span>M)</div>
            `
            SUMMARY.appendChild(cache_stats)
        }
    } else {
        for (let i = 0; i < N_CACHE_LAYERS; i++) {
            cache_stats = document.createElement("div");
            cache_stats.innerHTML = `
                <div>L${i+1}: <span class="cache-miss-percent"></span>% (<span class="cache-hits"></span>H/<span class="cache-misses"></span>M)</div>
            `
            SUMMARY.appendChild(cache_stats)
        }
    }

    combined_missrate_object = document.createElement("div");
    combined_missrate_object.innerHTML = `Cache Miss-rate: <span id="cache-miss-rate"></span>%`
    SUMMARY.appendChild(combined_missrate_object);

    clock_cycles_counter_object = document.createElement("div");
    clock_cycles_counter_object.innerHTML = `Lookup Clock Cycles: <span id="cycle_counter"></span>`
    SUMMARY.appendChild(clock_cycles_counter_object)

    CYCLE_COUNTER = document.getElementById("cycle_counter")
    CACHE_MISS_RATE = document.getElementById("cache-miss-rate")
    CACHE_HIT_COUNTER_OBJECTS = SUMMARY.querySelectorAll(".cache-hits")
    CACHE_MISS_COUNTER_OBJECTS = SUMMARY.querySelectorAll(".cache-misses")
    CACHE_PERCENT_OBJECTS = SUMMARY.querySelectorAll(".cache-miss-percent")
    if (HAS_INSTRUCTION_CACHE) {
        INSTR_HIT_COUNTER_OBJECT = SUMMARY.querySelector(".instr-cache-hits")
        INSTR_MISS_COUNTER_OBJECT = SUMMARY.querySelector(".instr-cache-misses")
        INSTR_PERCENT_OBJECT = SUMMARY.querySelector(".instr-cache-miss-percent")
    }

    INPUT_PAGE.style.display = "none";
    VISUALIZATION_PAGE.style.display = "flex";
    EXEC_LOG = exec_log
    TOTAL_STEPS = exec_log.length
    INSTR_COUNTER.innerHTML = "(0/" + TOTAL_STEPS + ") "
}

function confirmArchitecture() {
    cacheList = Array.from(ARCHITECTURE.querySelectorAll(".cache-container"));
    cacheList.forEach(cache => {
        if (cache.querySelector(".cache-title").innerHTML == "L1 Instruction") {
            confirm_instr_cache(cache)
        } else {
            confirm_data_cache(cache)
            N_CACHE_LAYERS++ // Data caches determine depth
        }
    });
}

function confirm_data_cache(cache) {
    let input_boxes = cache.querySelectorAll(".num-box")
    HITS.push(0)
    MISSES.push(0)
    data_caches.push({p:input_boxes[0].value,
                      q:input_boxes[1].value,
                      k:input_boxes[2].value,
                      a:input_boxes[3].value})
    block_offset_len = Math.log2(Math.pow(2, input_boxes[2].value))
    set_len = Math.log2((Math.pow(2,input_boxes[0].value) * input_boxes[1].value) / (Math.pow(2, input_boxes[2].value) * input_boxes[3].value))
    BIT_LENGTHS.push({s:set_len, b:block_offset_len})
}

function confirm_instr_cache(cache) {
    HAS_INSTRUCTION_CACHE = true
    let input_boxes = cache.querySelectorAll(".num-box")
    instr_cache = {p:input_boxes[0].value,
                   q:input_boxes[1].value,
                   k:input_boxes[2].value,
                   a:input_boxes[3].value}
    block_offset_len = Math.log2(Math.pow(2, input_boxes[2].value))
    set_len = Math.log2((Math.pow(2,input_boxes[0].value) * input_boxes[1].value) / (Math.pow(2, input_boxes[2].value) * input_boxes[3].value))
    INSTR_BIT_LENGTHS = {s:set_len, b:block_offset_len}
}

// TODO: Everything below this point is giga scuffed right now
function renameCaches() {
    titles = ARCHITECTURE.querySelectorAll(".cache-title")
    
    if (HAS_INSTRUCTION_CACHE) {
        for (let i = 2; i <= titles.length-1; i++) {
            titles[i].innerHTML = "L" + i
        }
    } else {
        for (let i = 1; i <= titles.length; i++) {
            titles[i-1].innerHTML = "L" + i
        }
    }
    if (!HAS_INSTRUCTION_CACHE && ARCHITECTURE.children.length != 1) {
        ARCHITECTURE.children[0].querySelector(".add-instr-cache-button").style.display = "flex"
    }
}

function removeCache(button) {
    if (button.previousElementSibling.innerHTML == "L1 Data") {
        button.parentElement.parentElement.parentElement.remove()
        HAS_INSTRUCTION_CACHE = false
        renameCaches()
    } else if (button.previousElementSibling.innerHTML == "L1 Instruction"){
        data_cache = button.parentElement.parentElement.previousElementSibling
        data_cache.style.width ="auto"
        wrapper = data_cache.parentElement
        wrapper.replaceWith(data_cache)
        HAS_INSTRUCTION_CACHE = false
    }
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
        <div class="spread-horizontal">
            <span>
                Associativity: <input class="num-box" type="number">-way
            </span>
            <button style="display: none;" class="add-instr-cache-button" onclick="add_instruction_cache(this)">Add instruction cache</button>
        </div>
    </div>
    `
    ARCHITECTURE.insertBefore(container, ADD_CACHE)
    renameCaches()
}

function add_instruction_cache() {
    HAS_INSTRUCTION_CACHE = true
    l1 = ARCHITECTURE.children[0]
    l1.querySelector(".add-instr-cache-button").style.display = "none"
    settings = l1.querySelectorAll(".num-box")
    data_container = document.createElement("div")
    

    data_container.style.width = "50%"
    data_container.classList.add("cache-container")
    instr_container = document.createElement("div")
    instr_container.style.width = "50%"
    instr_container.classList.add("cache-container")

    data_container.innerHTML = l1.innerHTML
    data_container.querySelector(".cache-title").innerHTML = "L1 Data"
    data_container.querySelector(".add-instr-cache-button").style.display = "none"
    instr_container.innerHTML = `
        <div class="cache-header">
            <h2 class="cache-title">L1 Instruction</h2>
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
    new_settingsd = data_container.querySelectorAll(".num-box")
    new_settingsi = instr_container.querySelectorAll(".num-box")
    for (let i = 0; i < 4; i++) {
        new_settingsd[i].value = settings[i].value
        new_settingsi[i].value = settings[i].value
    }

    wrapper = document.createElement("div")
    wrapper.appendChild(data_container)
    wrapper.appendChild(instr_container)
    wrapper.classList.add("flex-horizontal")
    ARCHITECTURE.replaceChild(wrapper, l1)
}


function preset_program(path) {
    fetch(path)
        .then(response => response.text())
        .then(data => {document.getElementById("programText").value = data})
        .catch(error => console.error("Error fetching file:", error));
}

// This doesn't work currently
function preset_architecture(caches) {
    HAS_INSTRUCTION_CACHE = !caches["instr"] == null
    ARCHITECTURE.innerHTML = ""
    caches["data"].forEach(cache => {
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
                <div class="spread-horizontal">
                    <span>
                        Associativity: <input class="num-box" type="number" value="${cache[3]}">-way
                    </span>
                    <button style="display: none;" class="add-instr-cache-button" onclick="add_instruction_cache(this)">Add instruction cache</button>
                </div>
            </div>
        `
        ARCHITECTURE.appendChild(container)
    })

    if (caches["instr"]) {
        add_instruction_cache()
        let instr_setup = caches["instr"]
        let instr_boxes = ARCHITECTURE.children[0].children[1].querySelectorAll(".num-box")
        for (let i = 0; i < instr_setup.length; i++) {
            instr_boxes[i].value = instr_setup[i]
        }
    }

    ARCHITECTURE.appendChild(ADD_CACHE)
    renameCaches()
}
