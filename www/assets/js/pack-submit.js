import { initializeCourseSelector, courseIdToMusicId } from "./course-selector.js";

const BACKEND_URL = window.location.hostname == "localhost" ? "http://localhost:3000" : "https://backend.mkw-spc.com";

let submitForm = document.getElementById("submit-form");

let submitFormUpload = document.getElementById("submit-form-upload");
let submitFormDownload = document.getElementById("submit-form-download");

let submitFormMetadataName = document.getElementById("submit-form-metadata-name");
let submitFormMetadataDescription = document.getElementById("submit-form-metadata-description");
let submitFormMetadataAuthorNames = document.getElementById("submit-form-metadata-authorNames");

let submitFormTable = document.getElementById("submit-form-table");

let submitFormTrackUpload = document.getElementById("submit-form-track-upload");
let submitFormSearchInput = document.getElementById("submit-form-search-input");
let submitFormSearchAutocomplete = document.getElementById("submit-form-search-autocomplete");

let trackMetadataDialog = document.getElementById("track-metadata-dialog");
let trackMetadataForm = document.getElementById("track-metadata-form");
let trackMetadataQuit = document.getElementById("track-metadata-quit");
let trackMetadataName = document.getElementById("track-metadata-name");
let trackMetadataVersion = document.getElementById("track-metadata-version");
let trackMetadataCourse = document.getElementById("track-metadata-course");
let trackMetadataMusic = document.getElementById("track-metadata-music");

let loginButton = document.getElementById("login-button");
let successBox = document.getElementById("success-box");
let errorBox = document.getElementById("error-box");

function debounce(func, timeout) {
    let timer;
    return () => {
        clearTimeout(timer);
        timer = setTimeout(func, timeout);
    };
}

function showError(errorText) {
    errorBox.innerText = `Error: ${errorText}`
    successBox.hidden = true;
    errorBox.hidden = false;
}

async function fetchBackend(endpoint, options) {
    let response = await fetch(`${BACKEND_URL}/${endpoint}`, options);
    if (response.status === 401) {
        localStorage.removeItem("access-token");
        submitForm.hidden = true;
        showLoginButton();

        throw "Failure to authenticate, please log in again!"
    } else {
        return response;
    }
}

function handleLoginRedirect() {
    let urlParams = new URLSearchParams(window.location.search);
    let urlAccessToken = urlParams.get('access_token');
    if (urlAccessToken != null) {
        localStorage.setItem("access-token", urlAccessToken);
    }

    return urlAccessToken;
}

async function fetchTrack(accessToken, trackId) {
    let response = await fetchBackend(
        `track?query=${trackId}&access_token=${accessToken}`,
        {method: "GET"}
    )

    if (response.ok) {
        return await response.json();
    } else {
        throw await response.text();
    }
}

async function uploadTrack(accessToken, fileContents, name, version, course, music) {
    let formData = new FormData();
    formData.append("metadata", JSON.stringify({
        name: name,
        music: music,
        course: course,
        version: version,
        access_token: accessToken,   
    }));
    formData.append("file", fileContents);

    let response = await fetchBackend("track", {
        method: "POST",
        body: formData
    })

    if (response.ok) {
        return await response.json();
    } else {
        throw await response.text();
    }
}

function getFinalBody(accessToken) {
    let tracks = [];
    let tableRows = Array.from(submitFormTable.children);
    for (const row of tableRows.slice(1, submitForm.children.length)) {
        let id = row.children[0].hash;
        let typeSelector = row.children[2].firstElementChild;

        if (typeSelector === null) {
            tracks.push(["Race", id]);
        } else {
            tracks.push([typeSelector.value, id]);
        }
    }

    return {
        name: submitFormMetadataName.value,
        description: submitFormMetadataDescription.value,
        author_names: submitFormMetadataAuthorNames.value,

        tracks: tracks,
        access_token: accessToken,
    };
}

function createTrackRow(name, isBattle, versions) {
    let tableRow = document.createElement("tr");

    let idElementContainer = document.createElement("td");
    let idElement = document.createElement("code");
    idElementContainer.appendChild(idElement);

    idElementContainer.hash = versions[0][1];
    idElement.innerText = versions[0][1].substring(0, 7);

    let nameElement = document.createElement("td");
    nameElement.innerText = name;

    let typeElementContainer = document.createElement("td");
    if (isBattle) {
        let typeElement = document.createElement("select");
        let balloonElement = document.createElement("option");
        let coinElement = document.createElement("option");
        balloonElement.text = "Balloon";
        coinElement.text = "Coin";
        typeElement.appendChild(balloonElement);
        typeElement.appendChild(coinElement);
        typeElementContainer.appendChild(typeElement);
    } else {
        typeElementContainer.innerText = "Race";
    }

    let versionSelector = document.createElement("select")
    versionSelector.addEventListener("click", (_) => {
        idElementContainer.hash = versionSelector.value
        idElement.innerText = idElementContainer.hash.substring(0, 7);
    });

    for (let [version, id] of versions) {
        let optionElement = document.createElement("option");
        optionElement.text = version;
        optionElement.value = id;

        versionSelector.appendChild(optionElement);
    }

    let versionElement = document.createElement("td");
    versionElement.appendChild(versionSelector);

    let removeElement = document.createElement("button");
    removeElement.classList.add("sp-button");
    removeElement.innerText = "X";
    removeElement.addEventListener("click", (e) => {
        e.preventDefault();
        tableRow.remove();
    })

    let removeElementContainer = document.createElement("td");
    removeElementContainer.append(removeElement);    

    tableRow.appendChild(idElementContainer);
    tableRow.appendChild(nameElement);
    tableRow.appendChild(typeElementContainer);
    tableRow.appendChild(versionElement);
    tableRow.appendChild(removeElementContainer);

    return tableRow;
}

async function onSubmit(e, accessToken) {
    e.preventDefault();

    let response;
    try {
        response = await fetchBackend(`submit`, {
            method: "POST",
            headers: {"Content-type": "application/json; charset=UTF-8"},
            body: JSON.stringify(getFinalBody(accessToken))
        });
    } catch (e) {
        showError("Network error when submitting form!");
        console.error(e);
        return;
    }

    let text;
    try {
        text = await response.text()
    } catch (e) {
        showError("Network error when submitting form!");
        console.error(e);
    }

    if (response.ok) {
        successBox.innerText = `Successfully submitted pack!\n(Pack ID: ${text})`
        successBox.hidden = false;
        errorBox.hidden = true;
    } else {
        showError(text);
    }
}

function onDownload() {
    let tempAElement = document.createElement("a");
    let data = JSON.stringify(getFinalBody(null));
    let dataURI = URL.createObjectURL(new Blob([data]));

    tempAElement.href = dataURI;
    tempAElement.download = "pack-export.json";

    tempAElement.click();
}

async function onUpload() {
    if (submitFormUpload.files.length == 0) {
        return;
    }

    let fileContents = await submitFormUpload.files[0].text();
    let file = JSON.parse(fileContents);

    try {
        submitFormMetadataName.value = file["name"];
        submitFormMetadataDescription.value = file["description"];
        submitFormMetadataAuthorNames.value = file["author_names"];
        for (let [_, track] of file["tracks"]) {
            let trackResponse = await fetchTrack(track);
            submitFormTable.appendChild(createTrackRow(
                trackResponse.name,
                trackResponse.is_battle,
                trackResponse.versions
            ));
        }
    } catch (e) {
        console.error(e);
        showError("Failed to import in-progress pack!");
    }
}

async function onSearchTyping() {
    let results;

    try {
        let resp = await fetchBackend(`search?query=${submitFormSearchInput.value}`);
        results = await resp.json();
    } catch (e) {
        showError(`Failed to search: ${e}`);
        return;
    }

    submitFormSearchAutocomplete.innerHTML = "";
    for (let track of results) {
        let listItem = document.createElement("li");
        listItem.innerText = track.name;
        listItem.isBattle = track.is_battle;
        listItem.versions = track.versions
        listItem.addEventListener("click", (_) => onAutocompleteItemClick(listItem, null));

        submitFormSearchAutocomplete.appendChild(listItem);
    }
}

function onClick(event) {
    // Ignore other click events if the dialog is open.
    if (trackMetadataDialog.open) {
        return;
    }

    // If the click is on the input box, show it.
    if (submitFormSearchInput.contains(event.target)) {
        submitFormSearchAutocomplete.hidden = false;
        return;
    }

    // If the click is outside the autocomplete box, hide it.
    if (!submitFormSearchAutocomplete.contains(event.target)) {
        submitFormSearchAutocomplete.hidden = true;
        return;
    }
}

function onAutocompleteItemClick(item) {
    submitFormSearchInput.value = "";
    submitFormSearchAutocomplete.innerHTML = "";
    submitFormTable.appendChild(createTrackRow(
        item.innerText,
        item.isBattle,
        item.versions
    ));
}

function onTrackUpload(_) {
    if (submitFormTrackUpload.files.length == 0) {
        return;
    }

    trackMetadataForm.file = submitFormTrackUpload.files[0];
    trackMetadataDialog.showModal();
}

function onDialogClick(event) {
    // If the X svg element is clicked, or the click is outside the dialog.
    if (
        trackMetadataQuit.contains(event.target) || 
        event.currentTarget === event.target
    ) {
        // Close the dialog properly.
        trackMetadataDialog.close();
    }    
}

async function onDialogSubmit(event, accessToken) {
    event.preventDefault();
    trackMetadataDialog.close();
    
    let response;
    let fileContents = await trackMetadataForm.file;
    try {
        response = await uploadTrack(
            accessToken,
            fileContents,
            trackMetadataName.value,
            trackMetadataVersion.value,
            parseInt(trackMetadataCourse.value),
            courseIdToMusicId(parseInt(trackMetadataMusic.value)),
        )
    } catch (e) { 
        return showError(e);
    }

    let name, isBattle, versions;
    if (response.is_new) {
        versions = [[trackMetadataVersion.value, response.id]];
        name = trackMetadataName.value;
        isBattle = false;
    } else {
        try {
            let fetchResponse = await fetchTrack(accessToken, response.id);
            isBattle = fetchResponse.is_battle;
            versions = fetchResponse.versions;
            name = fetchResponse.name;
        } catch (e) {
            return showError(e);
        }
    }

    let tableRow = createTrackRow(name, isBattle, versions);
    submitFormTable.appendChild(tableRow);
}

function showLoginButton() {
    let clientId = "1139943703931277362";
    let redirectUri = encodeURIComponent(`${BACKEND_URL}/oauth`);
    let oauth2Url = `https://discord.com/api/oauth2/authorize?client_id=${clientId}&redirect_uri=${redirectUri}&response_type=code&scope=identify`;

    loginButton.href = oauth2Url;
    loginButton.hidden = false;
}

function showSubmitForm(accessToken) {
    document.addEventListener("click", onClick);
    submitForm.addEventListener("submit", (e) => onSubmit(e, accessToken));
    submitFormDownload.addEventListener("click", onDownload);
    submitFormUpload.addEventListener("input", onUpload);
    submitFormSearchInput.addEventListener("keyup", debounce(onSearchTyping, 500));
    submitFormTrackUpload.addEventListener("input", onTrackUpload);
    trackMetadataDialog.addEventListener("click", onDialogClick);
    trackMetadataForm.addEventListener("submit", (e) => onDialogSubmit(e, accessToken));

    initializeCourseSelector(trackMetadataCourse);
    initializeCourseSelector(trackMetadataMusic);

    submitForm.hidden = false;
}

let accessToken = handleLoginRedirect() ?? localStorage.getItem("access-token");
if (accessToken === null) {
    showLoginButton();
} else {
    showSubmitForm(accessToken);
}
