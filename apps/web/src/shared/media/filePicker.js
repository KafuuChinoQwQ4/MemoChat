/** Open a file picker and resolve with selected files */
export function pickFiles(accept, multiple = false) {
    return new Promise((resolve) => {
        const input = document.createElement("input");
        input.type = "file";
        input.accept = accept;
        input.multiple = multiple;
        input.onchange = () => resolve(Array.from(input.files ?? []));
        input.click();
    });
}
