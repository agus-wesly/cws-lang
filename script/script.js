let editorElement = document.getElementById("editor");
let outputElement = document.getElementById('output');
let runButton = document.getElementById("run-btn");
let shareButton = document.getElementById("share-btn");


let __DEBUG__ = true;
let editor = ace.edit("editor");
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


runButton.addEventListener("click", () => {
    const sourceCode = editor.getValue();
    if (!sourceCode) return;

    if (outputElement) outputElement.value = ''; // clear browser cache
    Module.ccall("RUN_SOURCE", null, ['string'], [sourceCode.trim()]);
})

shareButton.addEventListener("click", () => {
    const src = editor.getValue();
    if (!src) return;

    const urlParam = new URLSearchParams({ src });
    const sharedUrl = `${window.location.toString()}?${urlParam.toString()}`;

    navigator.clipboard.writeText(sharedUrl).then(() => {
        alert("Successfully copying URL to clipboard")
    });
})


function main() {
    editor.setTheme("ace/theme/github_light_default");
    editor.session.setMode("ace/mode/swift");
    editor.getSession().setUseWorker(false);
    document.getElementById('editor').style.fontSize = '15px'; // Font size

    function getDefaultValue() {
        return new URL(window.location.toString()).searchParams.get("src") ?? `andai salam = "Hallo, Dunia !";
tampil(salam);`
    }

    const defaultValue = getDefaultValue();
    editor.setValue(defaultValue, 1);
    editor.focus();
}


main();
