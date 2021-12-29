// const root = "/";
const root = "/GP-Tool/";

const config = {
    title: "GP-Tool: An user friendly graphical interface to apply GP-FBM (page under construction)",
    intro: { header: "Welcome", address: "index.html" },
    started: { header: "Getting started", address: "started.html" },
    plugins: [
        { header: "Alignment", address: "alignment.html" },
        { header: "Denoise", address: "denoise.html" },
        { header: "Gaussian Process", address: "gprocess.html" },
        { header: "Movie", address: "movie.html" },
        { header: "Trajectory", address: "trajectory.html" }
    ],
    batch: { header: "Batching", address: "batch.html" },
    save: { header: "Saving", address: "save.html" },
    test: { header: "Library tests", address: "tests.html" },
};


// This main function runs automatically
(function main() {

    // Setup header here
    let header = document.querySelector("header");
    let txt = document.createElement('h1');
    txt.innerText = config.title;
    header.append(txt)

    // Setup explorer tab
    let explorer = document.getElementsByClassName("explorer")[0]

    txt = document.createElement("a");
    txt.innerHTML = config.intro.header;
    txt.href = root + config.intro.address;
    explorer.append(txt)

    txt = document.createElement("a");
    txt.innerHTML = config.started.header;
    txt.href = root + config.started.address;
    explorer.append(txt)

    // Creating table with all the plugins
    txt = document.createElement("h2")
    txt.innerText = "Plugins"
    txt.style = "color: white; padding-top: 1rem;"
    explorer.append(txt);

    let loc = document.createElement("div");
    loc.style = "margin-left: 1rem;"

    config.plugins.forEach(element => {
        let txt = document.createElement("a");
        txt.innerText = element.header;
        txt.href = root + element.address;
        loc.append(txt)
    });

    explorer.append(loc);

    //  Batching content
    txt = document.createElement('a');
    txt.innerText = config.batch.header;
    txt.href = root + config.batch.address;
    txt.style = "padding-top: 1rem;"
    explorer.append(txt)

    // Saving content
    txt = document.createElement("a")
    txt.innerText = config.save.header;
    txt.href = root + config.save.address;
    explorer.append(txt)

    // Test content
    txt = document.createElement("a")
    txt.innerText = config.test.header;
    txt.href = root + config.test.address;
    explorer.append(txt)

})()