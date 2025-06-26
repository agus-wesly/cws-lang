let editorElement = document.getElementById("editor");
let outputElement = document.getElementById('output');
let button = document.getElementById("run-btn");

let editor = ace.edit("editor");
editor.setTheme("ace/theme/github_light_default");
editor.session.setMode("ace/mode/swift");
editor.getSession().setUseWorker(false);
// Font size
document.getElementById('editor').style.fontSize = '15px';

let __DEBUG__ = true;

var Module = {
    print(...args) {
        if (__DEBUG__) {
            console.log(...args);
        }

        if (outputElement) {
            var text = args.join(' ');
            outputElement.value += text + "\n";
            outputElement.scrollTop = outputElement.scrollHeight; // focus on bottom
        }
    },
    printErr(...args) {
        console.log(...args);
        if (outputElement) {
            var text = args.join(' ');
            outputElement.value += text + "\n";
            outputElement.scrollTop = outputElement.scrollHeight; // focus on bottom
        }
    },
};


button.addEventListener("click", () => {
    const sourceCode = editor.getValue();
    if (!sourceCode) return;

    if (outputElement) outputElement.value = ''; // clear browser cache
    Module.ccall("RUN_SOURCE", null, ['string'], [sourceCode]);


})


/*
 * 
tampil("Halo, Dunia!");

// Variable
andai x = 69;
x=x+1;
tampil(x);

// Boolean
andai true = sah;
andai false = sesat;

jika(true == sah) tampil("True adalah sah");
jika(false == sesat) tampil("False adalah sesat");

// Object
andai obj = {"foo": "bar"};
tampil(obj);

// Array
andai arr = [1,sesat,nihil,obj,"Wesly"];

ulang(andai i=0; i<jmlh(arr); i=i+1) {
    tampil(arr[i]);
}
 *
 *
 * */
