// remove node and file args
const args = process.argv.slice(2);

var inspect: boolean = false;   // inspect
var job: any = {};              // the job
var workflow: string = "";      // the workflow
var name_param: string = "";    // param name
var delay_param: number = 1;    // param delay
var worker: string = "";        // worker (dumm_app_worker_1 or dumm_app_worker_2)

for(var i=0; i < args.length; i++) {
    if(args[i] == "-i")
        inspect = true;
    else if(args[i] == "-w")
        workflow = args[++i];
    else if(args[i] == "-j")
        job = JSON.parse(args[++i]);
    else if(args[i] == "--name")
        name_param = args[++i];
    else if(args[i] == "--delay")
        delay_param = parseInt(args[++i]);
    else if(args[i] == "--worker")
        worker = args[++i];
}

// notify a progress
function notifyProgress(value: number) {
    const message = {notify: "progress", value: value};
    console.log(JSON.stringify(message));
}

// notify completition
function notifyCompletion(success: boolean, error: string="", condition: string="") {
    const message = {notify: "completion", success: success, error: error, condition: condition};
    console.log(JSON.stringify(message));
}

// notify work
function notifyWork(work: any) {
    const message = {notify: "work", work: work};
    console.log(JSON.stringify(message));
}

// log message
function log_message(severity: string, log: string) {
    const message = {notify: "log", severity: severity, message: log};
    console.log(JSON.stringify(message));
}

function sleep(ms: number) {
    return new Promise((resolve) => {
        setTimeout(resolve, ms);
    });
}

// work1
async function doWork1() {
    log_message("info", "work started");

    for(var i = 0; i < delay_param; i++) {
        await sleep(1000);
        notifyProgress((100/delay_param) * (i+1));
        notifyWork({sleep: i});
    }

    log_message("info", "work ended");

    notifyCompletion(true);
}

// work2
function doWork2() {
    doWork2();    
}

// inspect
if(inspect) {
    const workers: any = [ 
        { name : "dumm_app_worker_1", params : ["name", "delay"]},
        { name : "dumm_app_worker_2", params : ["name", "delay"]},
    ];

    // parse command args
    console.log(JSON.stringify(workers));
}
// process
else {
    if(worker == "dumm_app_worker_1")
        doWork1();
    else if(worker == "dumm_app_worker_2")
        doWork2();
}
