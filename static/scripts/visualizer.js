let CONFIG = []
let LOAD_LOG = []
let EXEC_LOG = []
let TOTAL_STEPS = 0
let CURRENT_STEP = 0
const DELAY = 0
let BIT_LENGTHS = []

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
    INSTR_COUNTER.innerHTML = "(" + (CURRENT_STEP+1) + "/" + TOTAL_STEPS + ") "
    INSTR.innerHTML = step["title"]
    visualizeInstr(step["readers"], step["writers"])
    if (CURRENT_STEP == TOTAL_STEPS - 1) {
        EXEC_LOG = []
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