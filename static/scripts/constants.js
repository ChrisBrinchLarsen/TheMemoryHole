const INPUT_PAGE = document.getElementsByClassName("full-container")[0]
const VISUALIZATION_PAGE = document.getElementsByClassName("full-container")[1]

const ARCHITECTURE = document.getElementById("architecture")
const ADD_CACHE = document.getElementById("add-cache")

const R = document.querySelectorAll(".register");

const INSTR = document.getElementById("instr");
const INSTR_COUNTER = document.getElementById("instr_counter")

const CODE_VIEWER = document.getElementById("code-viewer");
const ACCESS_COUNTER = document.getElementById("memory-accesses")
const SUMMARY = document.getElementById("running-summary")
const LINE_SUMMARY = document.getElementById("line-summary")
const SUMMARY_LINE_NR = document.getElementById("summary-line-nr")
const SUMMARY_LINE_HITS = document.getElementById("summary-line-hits")
const SUMMARY_LINE_MISSES = document.getElementById("summary-line-misses")
const SUMMARY_LINE_MISS_RATE = document.getElementById("summary-line-miss-rate")

const DELAY_INPUT = document.getElementById("delay-input")
const NEXT_BUTTON = document.getElementById("next-button")
const PLAY_BUTTON = document.getElementById("play-button")
const PAUSE_BUTTON = document.getElementById("pause-button")

const STDOUT = document.getElementById("stdout")