let EXEC_LOG = []
let TOTAL_STEPS = 0
let CURRENT_STEP = 0
const DELAY = 0;
let BIT_LENGTHS = []
let SPLIT_ADDRS = []
let INSTR_SPLIT_ADDRS = null
let ADDRESS_OBJECTS = []
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
let INSTR_HITS = 0
let INSTR_MISSES = 0
let MISSES = []
let LINE_HITS = []
let LINE_MISSES = []

let INSTR_CACHE_LATENCY = 3
let LATENCY_AT_LEVEL = [1, 3, 9, 27, 81, 243, 729, 2187, 6561, 19683, 59049, 177147]
let DRAM_LATENCY = 195
let ESTIMATED_CYCLES = 0
let CYCLE_COUNTER = {}

let CACHE_MISS_RATE = {}
let CACHE_HIT_COUNTER_OBJECTS = []
let CACHE_MISS_COUNTER_OBJECTS = []
let CACHE_PERCENT_OBJECTS = []

let line_start = -1
let line_end = -2

let render_time = 0
let frame_start = 0
let step_time = 0
let steps_per_frame = 0
let SECOND = 1000
let maximum_frames_per_second = 0

let CACHE_OBJECTS = [] // The first N_CACHE_LAYERS are the data caches, and then potnetially N+1 is the instr cache
let INSTR_CACHE_OBJECT = null

let CHECKSUM = 0

const CHUNK_SIZE = 500

async function increment_step() {
    CURRENT_STEP++
    if (CURRENT_STEP % CHUNK_SIZE == 0) {await get_more_steps()}
}

async function visualizeStep_playing(n_steps) {
    if (CURRENT_STEP == TOTAL_STEPS) {
        pause()
        return
    }

    if (PLAYING) {
        if (CURRENT_STEP == TOTAL_STEPS-1) {
            visualizeStep(EXEC_LOG[CURRENT_STEP % CHUNK_SIZE])
            await increment_step()
            pause()
        } else {
            if (n_steps < 1) {n_steps++}
            for (let i = 0; i < n_steps && CURRENT_STEP < TOTAL_STEPS; i++) {
                visualizeStep(EXEC_LOG[CURRENT_STEP % CHUNK_SIZE]);
                await increment_step()
                
            }
            prep_next_step_dynamically()
        }
    }
}

function visualizeStep(step) {
    clear_sets_of_misses()
    clear_lines()
    if (step["lines-changed"]) {
        clear_src_lines()
        visualize_src(step["lines"][0], step["lines"][1])
    }

    if (step["stdout"] != 0) {
        STDOUT.innerHTML += String.fromCharCode(step["stdout"])
    }
    
    // Updating the summary counter of total memory accesses
    if (step["addr"].length) {
        ACCESS_COUNTER.innerHTML = 1 + Number(ACCESS_COUNTER.innerHTML)
    }
    
    if (step["lines"].length > 0) {
        visualize_path(step["type"], step["hits"], step["misses"], step["evict"], step["insert"], step["validity"], step["dirtiness"], step["lines"][0], step["lines"][1], step["is_write"])
    } else {
        visualize_path(step["type"], step["hits"], step["misses"], step["evict"], step["insert"], step["validity"], step["dirtiness"], -2, -1, step["is_write"])
    }

    
    updateLineSummary(SELECTED_LINE)
    
    let hit_sum = 0
    for (let i = 0; i < N_CACHE_LAYERS; i++) {
        hit_sum += HITS[i]
        ADDRESS_OBJECTS[i].innerHTML = hex_to_string_addr(step["addr"][0], BIT_LENGTHS[i].s, BIT_LENGTHS[i].b);
        CACHE_HIT_COUNTER_OBJECTS[i].innerHTML = HITS[i]
        CACHE_MISS_COUNTER_OBJECTS[i].innerHTML = MISSES[i]
        CACHE_PERCENT_OBJECTS[i].innerHTML = Math.round((MISSES[i] / (MISSES[i] + HITS[i]))*100)
        
    }
    if (HAS_INSTRUCTION_CACHE) {
        INSTR_SPLIT_ADDRS.innerHTML = hex_to_string_addr(step["addr"][0], INSTR_BIT_LENGTHS.s, INSTR_BIT_LENGTHS.b)
        hit_sum += INSTR_HITS
        INSTR_HIT_COUNTER_OBJECT.innerHTML = INSTR_HITS
        INSTR_MISS_COUNTER_OBJECT.innerHTML = INSTR_MISSES
        INSTR_PERCENT_OBJECT.innerHTML = Math.round((INSTR_MISSES / (INSTR_MISSES + INSTR_HITS))*100)
    }
    CACHE_MISS_RATE.innerHTML = Math.round((MISSES[N_CACHE_LAYERS-1] / (MISSES[N_CACHE_LAYERS-1] + hit_sum)) * 100)
    CYCLE_COUNTER.innerHTML = ESTIMATED_CYCLES
    
    INSTR_COUNTER.innerHTML = "(" + (CURRENT_STEP+1) + "/" + TOTAL_STEPS + ") "
    INSTR.innerHTML = step["title"]
    visualizeInstrRegs(step["readers"], step["writers"])
    
    if (step["type"] == "instr") {
        if (Number(step["CS"]) != CHECKSUM) {
            console.log(step["CS"] + " B != F " + CHECKSUM)
        }
    }
}

function visualize_path(access_type, hits, misses, evictions, inserts, validities, dirties, lineS, lineE, is_write) {
    COLORED_LINES = []
    COLORED_SET_OBJECTS = []
    
    hits.forEach(hit => {
        if (HAS_INSTRUCTION_CACHE && Number(hit[0]) == 1 && access_type == "fetch") {
            INSTR_HITS += 1
            ESTIMATED_CYCLES += INSTR_CACHE_LATENCY
            give_line_class("hit", INSTR_CACHE_OBJECT, hit[1], hit[2])
        } else {
            HITS[hit[0]-1] += 1;
            ESTIMATED_CYCLES += LATENCY_AT_LEVEL[hit[0]-1]
            give_line_class("hit", CACHE_OBJECTS[hit[0]-1], hit[1], hit[2])
        }
        for (let i = lineS; i <= lineE; i++) {
            LINE_HITS[i] += 1;
        }
    })
    
    misses.forEach(miss => {
        if (HAS_INSTRUCTION_CACHE && Number(miss[0]) == 1 && access_type == "fetch") {
            give_set_class("miss", INSTR_CACHE_OBJECT, miss[1])
            INSTR_MISSES += 1
            ESTIMATED_CYCLES += INSTR_CACHE_LATENCY
        } else {
            give_set_class("miss", CACHE_OBJECTS[miss[0]-1], miss[1])
            MISSES[miss[0]-1] += 1;
            ESTIMATED_CYCLES += LATENCY_AT_LEVEL[miss[0]-1]
        }

        if (miss[0] == N_CACHE_LAYERS) {
            ESTIMATED_CYCLES += DRAM_LATENCY
        }

        for (let i = lineS; i <= lineE; i++) {
            if (miss[0] == N_CACHE_LAYERS) { // Miss in last layer of cache, this is prone to breakage
                LINE_MISSES[i] += 1;
            }
        }
    })
    
    evictions.forEach(evictee => {
        let cache = CACHE_OBJECTS[evictee[0]-1]
        let set = evictee[1]
        let line = evictee[2]
        give_line_class("evict", cache, set, line)
    })
    inserts.forEach(insertee => {
        if (HAS_INSTRUCTION_CACHE && Number(insertee[0]) == 1 && access_type == "fetch") {
            give_line_class("insert", INSTR_CACHE_OBJECT, insertee[1], insertee[2])
        } else {
            let cache = CACHE_OBJECTS[insertee[0]-1]
            let set = insertee[1]
            let line = insertee[2]
            give_line_class("insert", cache, set, line)
        }
    })

    validities.forEach(validity_change => {
        let cache = CACHE_OBJECTS[validity_change[0]-1]
        let set = validity_change[1]
        let line = validity_change[2]
        let became_valid = validity_change[3]

        if (validity_change[4] == 'i') {
            if (became_valid) {
                give_line_class("valid", INSTR_CACHE_OBJECT, set, line)
                update_checksum(1, 0, set, line)
            } else {
                remove_class_from_line("valid", INSTR_CACHE_OBJECT, set, line)
                update_checksum(2, 0, set, line)
            }
        } else if (became_valid) {
            give_line_class("valid", cache, set, line)
            update_checksum(1, validity_change[0], set, line)
        } else {
            remove_class_from_line("valid", cache, set, line)
            update_checksum(2, validity_change[0], set, line)
        }
    })

    dirties.forEach(dirtiness_change => {
        let cache = CACHE_OBJECTS[dirtiness_change[0]-1]
        let set = dirtiness_change[1]
        let line = dirtiness_change[2]



        if (dirtiness_change[4] == 'i') {
            if (dirtiness_change[3]) {
                give_line_class("dirty", INSTR_CACHE_OBJECT, set, line)
                update_checksum(3, 0, set, line)
            } else {
                remove_class_from_line("dirty", INSTR_CACHE_OBJECT, set, line)
                update_checksum(4, 0, set, line)
            }
        } else if (dirtiness_change[3]) {
            give_line_class("dirty", cache, set, line)
            update_checksum(3, dirtiness_change[0], set, line)
        } else {
            remove_class_from_line("dirty", cache, set, line)
            update_checksum(4, dirtiness_change[0], set, line)
        }
    })
}

function give_set_class(class_name, cache, set_n) {
    let set = cache.children[1].children[Number(set_n)]
    set.classList.add(class_name)
    COLORED_SET_OBJECTS.push(set)
}

function clear_sets_of_misses() {
    COLORED_SET_OBJECTS.forEach(set => {set.classList.remove("miss")})
}

function give_line_class(class_name, cache, set_n, line_n) {
    let line = cache.children[1].children[Number(set_n)].children[line_n]
    line.classList.add(class_name);
    COLORED_LINES.push(line)
}

function remove_class_from_line(class_name, cache, set_n, line_n) {
    cache.children[1].children[Number(set_n)].children[line_n].classList.remove(class_name)
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
        if (ACTIVE_BREAKPOINTS.has(1+Number(i))) {pause()}
        SRC_LINES[i].classList.add("active");
    }
}

function create_caches() {
    let cache_container = document.getElementById("cache-container")
    cache_container.innerHTML = ""

    for (let i = 0; i < N_CACHE_LAYERS; i++) {
        let cache_layer = document.createElement("div")
        cache_layer.classList.add("cache-layer")
        
        let cache = document.createElement("div")
        cache.classList.add("cache")
        
        cache.innerHTML = `
        <div class="cache-info">
            <h1 class="cache-name">L${i+1}</h1>
            <div class="split_addr">
            ${hex_to_string_addr(0x0, BIT_LENGTHS[i].s, BIT_LENGTHS[i].b)}
            </div>
        </div>
        <div class="set-holder"></div>
        `
        let set_holder = cache.querySelector(".set-holder")
        for (let k = 0; k < Math.pow(2, BIT_LENGTHS[i].s); k++) {
            set = document.createElement("div")
            set.classList.add("set")
            for (let j = 0; j < data_caches[i].a; j++) {
                line = document.createElement("div");
                line.classList.add("line");
                set.appendChild(line)
            }
            set_holder.appendChild(set);
        }

        CACHE_OBJECTS.push(cache)
        
        cache_layer.appendChild(cache)
        cache_container.appendChild(cache_layer)
    }

    if (HAS_INSTRUCTION_CACHE) {
        INSTR_CACHE_OBJECT = document.createElement("div")
        INSTR_CACHE_OBJECT.classList.add("cache")
        INSTR_CACHE_OBJECT.innerHTML = `
        <div class="cache-info">
            <h1 class="cache-name">L1i</h1>
            <div class="split_addr">
            ${hex_to_string_addr(0x0, INSTR_BIT_LENGTHS.s, INSTR_BIT_LENGTHS.b)}
            </div>
        </div>
        <div class="set-holder"></div>
        `

        INSTR_SPLIT_ADDRS = INSTR_CACHE_OBJECT.querySelector(".split_addr")
        let set_holder = INSTR_CACHE_OBJECT.querySelector(".set-holder")
        for (let k = 0; k < Math.pow(2, INSTR_BIT_LENGTHS.s); k++) {
            set = document.createElement("div")
            set.classList.add("set")
            for (let j = 0; j < instr_cache.a; j++) {
                line = document.createElement("div");
                line.classList.add("line");
                set.appendChild(line)
            }
            set_holder.appendChild(set);
        }
        CACHE_OBJECTS[0].parentElement.appendChild(INSTR_CACHE_OBJECT)
        CACHE_OBJECTS[0].querySelector(".cache-name").innerHTML = "L1d"
    }
}

// Returns a span containing the address split up in tag, set, and offset bits
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
    element = `<span style="color: red;">${tag}</span>|<span style="color: green;">${set}</span>|<span style="color: blue;">${offset}</span>`
    SPLIT_ADDRS.push(element)
    return element
}

// Colors in the corresponding registers that were read from and written to
function visualizeInstrRegs(readers, writers) {
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

// Removes color from all colored registers
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
    SUMMARY_LINE_MISS_RATE.innerHTML = Math.round((LINE_MISSES[SELECTED_LINE] / (LINE_MISSES[SELECTED_LINE] + LINE_HITS[SELECTED_LINE]))*100)
}

async function play() {
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

    render_time = 1000/60 // Guessing 60fps before further estimations

    // Testing rendering time
    requestAnimationFrame(time_dummy1)
    const step_start = performance.now()
    visualizeStep(EXEC_LOG[CURRENT_STEP % CHUNK_SIZE])
    await increment_step()
    step_time = performance.now() - step_start
    console.log("Render time: " + render_time)
    console.log("Step time: " + step_time)
    steps_per_frame = render_time / step_time
    console.log("We can render " + steps_per_frame + " steps per frame")
    maximum_frames_per_second = SECOND / render_time
    console.log("We can render " + maximum_frames_per_second + " frames per second")


    //TEMP
    desired_steps_per_second = DELAY_INPUT.value
    console.log("We want " + desired_steps_per_second + " steps per second")
    requires_one_step_per = SECOND / desired_steps_per_second
    console.log("So we need one step per " + requires_one_step_per + "ms")

    if (requires_one_step_per >= render_time + step_time) {
        console.log("That's longer than it takes us to render a single step, which is " + (render_time + step_time))
        console.log("So we simply wait " + requires_one_step_per + " before executing next step") 
    } else {
        console.log("That's less time than it takes to render a single frame, so we need more steps in each frame")
        console.log("In fact we need to render " + Math.round(desired_steps_per_second/maximum_frames_per_second) + " steps every " + render_time + "ms")
    }

    prep_next_step_dynamically()
}

function pause() {
    PLAYING = false
    PAUSE_BUTTON.style.display = "none"
    PLAY_BUTTON.style.display = "block"
}

async function next() {
    if (CURRENT_STEP == TOTAL_STEPS) {
        alert("Already at end of execution")
        return
    }
    if (PLAYING) {
        alert("Must be paused in order to perform step-through")
        return
    }
    visualizeStep(EXEC_LOG[CURRENT_STEP % CHUNK_SIZE])
    await increment_step()
}

async function end() {
    if (CURRENT_STEP == TOTAL_STEPS) {
        alert("Already at end of execution")
        return
    }

    while (CURRENT_STEP != TOTAL_STEPS) {
        visualizeStep(EXEC_LOG[CURRENT_STEP % CHUNK_SIZE])
        await increment_step()
    }
    if (PLAYING) {pause()}
}

function prep_next_step_dynamically() {
    desired_steps_per_second = DELAY_INPUT.value
    
    requires_one_step_per = SECOND / desired_steps_per_second
    if (requires_one_step_per >= render_time + step_time) {
        setTimeout(() => {
           visualizeStep_playing(1) 
        }, requires_one_step_per);
    } else { // We need to fit multiple steps per render
        setTimeout(() => {
            visualizeStep_playing(Math.round(desired_steps_per_second/maximum_frames_per_second))
        }, 0)

    }
}

function time_dummy1() {
    frame_start = performance.now()
    requestAnimationFrame(time_dummy2)
}

function time_dummy2() {
    render_time = performance.now() - frame_start
}

function update_checksum(type, layer, setidx, lineidx) {
    CHECKSUM = (CHECKSUM + type * 10 + layer * 100 + setidx * 1000 + lineidx * 10000) % 0xFFFFFFFF
}

const ACTIVE_BREAKPOINTS = new Set()

function pressed_breakpoint(number) {
    if (ACTIVE_BREAKPOINTS.has(number)) {
        ACTIVE_BREAKPOINTS.delete(number)
    } else {
        ACTIVE_BREAKPOINTS.add(number)
    }
}