let CONFIG = []
let LOAD_LOG = []
let EXEC_LOG = []
let TOTAL_STEPS = 0
let CURRENT_STEP = 0
const DELAY = 0;
let BIT_LENGTHS = []
let SPLIT_ADDRS = []
let ADDRESS_OBJECTS = []
let CACHES = []
let PROGRAM_TEXT = ""
let SRC_LINES = []
let SELECTED_LINE = undefined
let PLAYING = false
let COLORED_SET_OBJECTS = []
let COLORED_LINES = []
let REG_CHANGED = true

let READERS = []
let WRITERS = []

// The indices dictate which level of cache we're talking about
let HITS = []
let MISSES = []
let LINE_HITS = []
let LINE_MISSES = []

let CACHE_HIT_RATE = {}
let CACHE_HIT_COUNTER_OBJECTS = []
let CACHE_MISS_COUNTER_OBJECTS = []
let CACHE_PERCENT_OBJECTS = []

let step_time_sum = 0.0

let line_start = -1
let line_end = -2


function visualizeStep_playing() {
    if (PLAYING) {
        if (CURRENT_STEP == TOTAL_STEPS-1) {
            visualizeStep(EXEC_LOG[CURRENT_STEP])
            CURRENT_STEP += 1
            pause()
        } else {
            visualizeStep(EXEC_LOG[CURRENT_STEP])
            CURRENT_STEP += 1
            if (DELAY_INPUT.value == "0") {
                requestAnimationFrame(visualizeStep_playing);
            } else {
                setTimeout(() => {
                    visualizeStep_playing();
                }, DELAY_INPUT.value);
            }
        }
    }
}

function visualizeStep(step) {
    clear_sets()
    clear_lines()
    if (step["lines-changed"]) {
        clear_src_lines()
        visualize_src(step["lines"][0], step["lines"][1])
    }
    
    // Updating the summary counter of total memory accesses
    if (step["addr"].length) {
        ACCESS_COUNTER.innerHTML = 1 + Number(ACCESS_COUNTER.innerHTML)
    }
    
    
    SPLIT_ADDRS.forEach(addr => {addr.innerHTML = hex_to_string_addr(step["addr"],)})
    if (step["lines"].length > 0) {
        visualize_path(step["hits"], step["misses"], step["evict"], step["insert"], step["lines"][0], step["lines"][1], step["is_write"])
    } else {
        visualize_path(step["hits"], step["misses"], step["evict"], step["insert"], -2, -1, step["is_write"])
    }
    
    updateLineSummary(SELECTED_LINE)
    
    let hit_sum = 0
    for (let i = 0; i < CONFIG.length; i++) {
        hit_sum += HITS[i]
        ADDRESS_OBJECTS[i].innerHTML = hex_to_string_addr(step["addr"][0], BIT_LENGTHS[i].s, BIT_LENGTHS[i].b);
        CACHE_HIT_COUNTER_OBJECTS[i].innerHTML = HITS[i]
        CACHE_MISS_COUNTER_OBJECTS[i].innerHTML = MISSES[i]
        CACHE_PERCENT_OBJECTS[i].innerHTML = Math.round((HITS[i] / (MISSES[i] + HITS[i]))*100)
        
    }
    CACHE_HIT_RATE.innerHTML = Math.round((hit_sum / (MISSES[CONFIG.length-1] + hit_sum)) * 100)
    
    INSTR_COUNTER.innerHTML = "(" + (CURRENT_STEP+1) + "/" + TOTAL_STEPS + ") "
    INSTR.innerHTML = step["title"]
    visualizeInstr(step["readers"], step["writers"])
}

function visualize_path(hits, misses, evictions, inserts, lineS, lineE, is_write) {
    COLORED_LINES = []
    COLORED_SET_OBJECTS = []
    
    hits.forEach(hit => {
        HITS[hit[0]-1] += 1;
        give_line_class("hit", hit[0], hit[1], hit[2])
        if (is_write && (hit[0] == 1)) { // Hit a write access in L1
            console.log("We out here4")
            give_line_class("dirty", hit[0], hit[1], hit[2])
        }
        for (let i = lineS; i <= lineE; i++) {
            LINE_HITS[i] += 1;
        }
    })
    
    misses.forEach(miss => {
        set = CACHES[miss[0]-1].children[1].children[1+Number(miss[1])]
        set.classList.add("miss")
        COLORED_SET_OBJECTS.push(set)
        MISSES[miss[0]-1] += 1;
        for (let i = lineS; i <= lineE; i++) {
            if (miss[0] == CONFIG.length) { // Miss in last layer of cache, this is prone to breakage
                LINE_MISSES[i] += 1;
            }
        }
    })
    
    evictions.forEach(evictee => {
        give_line_class("evict", evictee[0], evictee[1], evictee[2])
            console.log("We out here3")
        give_line_class("dirty", evictee[0], evictee[1], evictee[2])
        give_line_class("valid", evictee[0], evictee[1], evictee[2])
    })
    inserts.forEach(insertee => {
        give_line_class("insert", insertee[0], insertee[1], insertee[2])
        give_line_class("valid", insertee[0], insertee[1], insertee[2])
        if (is_write && (insertee[0] == 1)) {
            console.log("We out here")
            give_line_class("dirty", insertee[0], insertee[1], insertee[2])
        } else {
            console.log("We out here2")
            remove_line_class("dirty", insertee[0], insertee[1], insertee[2])
        }
    })
}

function clear_sets() {
    COLORED_SET_OBJECTS.forEach(set => {set.classList.remove("miss")})
}


function give_line_class(className, cacheN, setN, lineN) {
    let line = CACHES[cacheN-1].children[1].children[1+Number(setN)].children[lineN]
    line.classList.add(className);
    COLORED_LINES.push(line)
}

function remove_line_class(className, cacheN, setN, lineN) {
    let line = CACHES[cacheN-1].children[1].children[1+Number(setN)].children[lineN]
    line.classList.remove(className);
}

function clear_lines() {
    COLORED_LINES.forEach(line => {
        line.classList.remove("hit")
        line.classList.remove("evict")
        line.classList.remove("insert")
    })
}


function clear_src_lines() {
    for (let i = line_start; i <= line_end; i++) {
        SRC_LINES[i].classList.remove("active")
    }
}

function visualize_src(start, end) {
    line_start = start;
    line_end = end;
    for (let i = start; i <= end; i++) {
        SRC_LINES[i].classList.add("active");
    }
}

function create_caches() {
    document.getElementById("caches").innerHTML = "";
    for (let i = 0; i < CONFIG.length; i++) {
        cache_div = document.createElement("div");
        cache_div.classList.add("cache")
        title = document.createElement("h1");
        title.innerHTML = "L" + (i+1);
        cache_info_div = document.createElement("div");
        cache_info_div.classList.add("cache-info");
        cache_div.appendChild(title);
        cache_div.appendChild(cache_info_div);
        addr = document.createElement("div")
        SPLIT_ADDRS.push(addr)
        addr.classList.add("split_addr")
        addr.innerHTML = hex_to_string_addr(0x0, BIT_LENGTHS[i].s, BIT_LENGTHS[i].b);
        cache_info_div.appendChild(addr);
        for (let k = 0; k < Math.pow(2, BIT_LENGTHS[i].s); k++) {
            set = document.createElement("div")
            set.classList.add("set")
            for (let j = 0; j < CONFIG[i].a; j++) {
                line = document.createElement("div");
                line.classList.add("line");
                set.appendChild(line)
            }
            cache_info_div.appendChild(set);
        }
        document.getElementById("caches").appendChild(cache_div)
        CACHES.push(cache_div)
    }
}

function hex_to_string_addr(addr, set_len, offset_len) {
    let i = 0
    let tag = "";
    let set = "";
    let offset = "";

    for (i; i < offset_len; i++) {
        offset = ((addr >> i) & 1) + offset
    }

    for (i; i < (offset_len + set_len); i++) {
        set = ((addr >> i) & 1) + set
    }
    for (i; i < 32; i++) {
        tag = ((addr >> i) & 1) + tag
    }
    return `<span style="color: red;">${tag}</span>|<span style="color: green;">${set}</span>|<span style="color: blue;">${offset}</span>`
}

function visualizeInstr(readers, writers) {
    clearRegisters();
    READERS = []
    WRITERS = []
    readers.forEach(reader => {
        READERS.push(R[reader])
        R[reader].classList.add("read-from"); 
        REG_CHANGED = true
    });
    writers.forEach(writer => {
        WRITERS.push(R[writer])
        R[writer].classList.add("written-to");
        REG_CHANGED = true
    });
}

function clearRegisters() {
    if (!REG_CHANGED) {return}
    READERS.forEach(reg => {
        reg.classList.remove("read-from");
        reg.classList.remove("written-to");
    });
    WRITERS.forEach(reg => {
        reg.classList.remove("read-from");
        reg.classList.remove("written-to");
    })
    REG_CHANGED = false
}

function returnToSummary() {
    SELECTED_LINE = undefined
    LINE_SUMMARY.style.display = "none"
    SUMMARY.style.display = "flex"
    SRC_LINES.forEach(line => {line.classList.remove("selected")})
}

function updateLineSummary(line_nr) {
    if (line_nr == undefined) {return}
    SUMMARY_LINE_HITS.innerHTML = LINE_HITS[SELECTED_LINE]
    SUMMARY_LINE_MISSES.innerHTML = LINE_MISSES[SELECTED_LINE]
    SUMMARY_LINE_HIT_RATE.innerHTML = Math.round((LINE_HITS[SELECTED_LINE] / (LINE_MISSES[SELECTED_LINE] + LINE_HITS[SELECTED_LINE]))*100)
}

function play() {
    if (PLAYING) {
        alert("You're already playing.")
        return
    } 
    if (CURRENT_STEP == TOTAL_STEPS) {
        alert("Already at end of execution")
        return
    }
    PLAYING = true
    PLAY_BUTTON.style.display = "none"
    PAUSE_BUTTON.style.display = "block"
    setTimeout(() => {
        visualizeStep_playing();
    }, DELAY_INPUT.value);
}

function pause() {
    PLAYING = false
    PAUSE_BUTTON.style.display = "none"
    PLAY_BUTTON.style.display = "block"
}

function next() {
    if (CURRENT_STEP == TOTAL_STEPS) {
        alert("Already at end of execution")
        return
    }
    if (PLAYING) {
        alert("Must be paused in order to perform step-through")
        return
    }
    visualizeStep(EXEC_LOG[CURRENT_STEP])
    CURRENT_STEP += 1
}

function end() {
    if (CURRENT_STEP == TOTAL_STEPS) {
        alert("Already at end of execution")
        return
    }
    if (CURRENT_STEP == TOTAL_STEPS-1) {
        visualizeStep(EXEC_LOG[CURRENT_STEP])
        CURRENT_STEP += 1
        if (PLAYING) {pause()}
    } else {
        visualizeStep(EXEC_LOG[CURRENT_STEP])
        CURRENT_STEP += 1
        end()
    }
}