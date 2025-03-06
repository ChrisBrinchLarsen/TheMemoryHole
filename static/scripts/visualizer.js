let CONFIG = []
let LOAD_LOG = []
let EXEC_LOG = []
let TOTAL_STEPS = 0
let CURRENT_STEP = 0
const DELAY = 500
let BIT_LENGTHS = []
let CACHES = []

function visualize() {
    visualizeStep(EXEC_LOG[CURRENT_STEP])
    CURRENT_STEP += 1
    if (CURRENT_STEP == (TOTAL_STEPS - 1)) {
        setTimeout(() => {
            visualizeStep(EXEC_LOG[CURRENT_STEP]);
        }, DELAY);
        return
    }
    setTimeout(() => {
        visualize();
    }, DELAY);
}

function visualizeStep(step) {
    clear_sets()
    clear_lines()
    visualize_path(step["hits"], step["misses"], step["evict"], step["insert"])
    INSTR_COUNTER.innerHTML = "(" + (CURRENT_STEP+1) + "/" + TOTAL_STEPS + ") "
    INSTR.innerHTML = step["title"]
    visualizeInstr(step["readers"], step["writers"])
    if (CURRENT_STEP == TOTAL_STEPS - 1) {
        EXEC_LOG = []
    }
}

function visualize_path(hits, misses, evictions, inserts) {
    hits.forEach(hit => {give_line_class("hit", hit[0], hit[1], hit[2])})
    evictions.forEach(evictee => {give_line_class("evict", evictee[0], evictee[1], evictee[2])})
    inserts.forEach(insertee => {give_line_class("insert", insertee[0], insertee[1], insertee[2])})
    misses.forEach(miss => {CACHES[miss[0]-1].children[1].children[1+Number(miss[1])].classList.add("miss")})
}

function give_line_class(className, cacheN, setN, lineN) {
    CACHES[cacheN-1].children[1].children[1+Number(setN)].children[lineN].classList.add(className);
}

function clear_lines() {
    document.querySelectorAll(".line").forEach(line => {
        line.classList.remove("hit")
        line.classList.remove("evict")
        line.classList.remove("insert")
    })
}

function clear_sets() {
    document.querySelectorAll(".set").forEach(set => {
        set.classList.remove("miss")
    })
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