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
 * print("Hello, World!");

// Variable
let x = 69;
x=x+1;
print(x);

// Object
let obj = {"foo": "bar"};
print(obj);

// Array
let arr = [1,false,nil,obj,"Wesly"];
for(let i=0; i<len(arr); i=i+1) {
    print(arr[i]);
}
 *
 *
 * */
