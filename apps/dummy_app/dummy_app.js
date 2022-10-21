"use strict";
var __awaiter = (this && this.__awaiter) || function (thisArg, _arguments, P, generator) {
    function adopt(value) { return value instanceof P ? value : new P(function (resolve) { resolve(value); }); }
    return new (P || (P = Promise))(function (resolve, reject) {
        function fulfilled(value) { try { step(generator.next(value)); } catch (e) { reject(e); } }
        function rejected(value) { try { step(generator["throw"](value)); } catch (e) { reject(e); } }
        function step(result) { result.done ? resolve(result.value) : adopt(result.value).then(fulfilled, rejected); }
        step((generator = generator.apply(thisArg, _arguments || [])).next());
    });
};
// remove node and file args
const args = process.argv.slice(2);
var inspect = false; // inspect
var job = {}; // the job
var workflow = ""; // the workflow
var name_param = ""; // param name
var delay_param = 1; // param delay
var worker = ""; // worker (dumm_app_worker_1 or dumm_app_worker_2)
for (var i = 0; i < args.length; i++) {
    if (args[i] == "-i")
        inspect = true;
    else if (args[i] == "-w")
        workflow = args[++i];
    else if (args[i] == "-j")
        job = JSON.parse(args[++i]);
    else if (args[i] == "--name")
        name_param = args[++i];
    else if (args[i] == "--delay")
        delay_param = parseInt(args[++i]);
    else if (args[i] == "--worker")
        worker = args[++i];
}
// notify a progress
function notifyProgress(value) {
    const message = { notify: "progress", value: value };
    console.log(JSON.stringify(message));
}
// notify completition
function notifyCompletion(success, error = "", condition = "") {
    const message = { notify: "completion", success: success, error: error, condition: condition };
    console.log(JSON.stringify(message));
}
// notify work
function notifyWork(work) {
    const message = { notify: "work", work: work };
    console.log(JSON.stringify(message));
}
// log message
function log_message(severity, log) {
    const message = { notify: "log", severity: severity, message: log };
    console.log(JSON.stringify(message));
}
function sleep(ms) {
    return new Promise((resolve) => {
        setTimeout(resolve, ms);
    });
}
// work1
function doWork1() {
    return __awaiter(this, void 0, void 0, function* () {
        log_message("info", "work started");
        for (var i = 0; i < delay_param; i++) {
            yield sleep(1000);
            notifyProgress((100 / delay_param) * (i + 1));
            notifyWork({ sleep: i });
        }
        log_message("info", "work ended");
        notifyCompletion(true);
    });
}
// work2
function doWork2() {
    doWork2();
}
// inspect
if (inspect) {
    const workers = [
        { name: "dumm_app_worker_1", params: ["name", "delay"] },
        { name: "dumm_app_worker_2", params: ["name", "delay"] },
    ];
    // parse command args
    console.log(JSON.stringify(workers));
}
// process
else {
    if (worker == "dumm_app_worker_1")
        doWork1();
    else if (worker == "dumm_app_worker_2")
        doWork2();
}
//# sourceMappingURL=dummy_app.js.map