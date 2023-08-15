---
layout: default
title: Track Pack Upload
permalink: /pack-upload
---

<script type="module" src="/assets/js/pack-submit.js"></script>

## Track Pack Uploader
This tool allows you to submit Track Packs for download in game.

All packs go through a manual review process to prevent spam and unwanted content, and
your discord account will be banned from submitting packs if found to be abusing this tool.

Tracks uploaded must use the `wbz` or `wu8` formats, to avoid transmitting copyrighted
Nintendo assets over the internet. For information on how to convert to these formats,
see the [wiki page](https://wiki.tockdom.com/wiki/WBZ_(File_Format)#Conversion_in_Windows).

<form id="submit-form" autocomplete="off" hidden=true>
<fieldset>
    <div class="submit-form-metadata">
        <label for="submit-form-metadata-name">Pack Name</label>
        <input required id="submit-form-metadata-name" maxLength=64 type="text">
        <label for="submit-form-metadata-description">Pack Description</label>
        <textarea required id="submit-form-metadata-description" maxLength=128 type="text"></textarea>
        <label for="submit-form-metadata-authorNames">Pack Authors</label>
        <input required id="submit-form-metadata-authorNames" maxLength=64 type="text">
    </div>
    <div class="submit-form-tracks">
        <div class="submit-form-table-container">
        <table id="submit-form-table">
            <tr>
                <th scope="col">ID</th>
                <th scope="col">Name</th>
                <th scope="col">Type</th>
                <th scope="col">Version</th>
                <th scope="col"></th>
            </tr>
        </table>
        </div>
        <div class="submit-form-tracks-submit">
            <label class="sp-button" for="submit-form-track-upload">Upload Track</label>
            <input id="submit-form-track-upload" type="file" accept=".wbz,.wu8" hidden>
            <div class="submit-form-tracks-search">
                <input id="submit-form-search-input" type="text" placeholder="Search for a Track...">
                <ul id="submit-form-search-autocomplete"></ul>
            </div>
        </div>
    </div>
</fieldset>
<button type="submit" class="sp-button">Submit for review</button>
<button type="button" class="sp-button" id="submit-form-download">Download in-progress pack</button>
<label class="sp-button" for="submit-form-upload">Upload in-progress pack</label>
<input id="submit-form-upload" type="file" accept=".json" hidden>
</form>

<a id="login-button" hidden=true>Login To Discord!</a>
<p id="success-box" hidden=true></p>
<p id="error-box" hidden=true></p>

<dialog id="track-metadata-dialog">
<div>
<div class="track-metadata-header">
<h2>Submit a Track</h2>
<svg id="track-metadata-quit" height=24px width=24px viewBox="5 5 14 14" fill="#bbbbbb">
    <path fill-rule="evenodd" clip-rule="evenodd" d="M5.29289 5.29289C5.68342 4.90237 6.31658 4.90237 6.70711 5.29289L12 10.5858L17.2929 5.29289C17.6834 4.90237 18.3166 4.90237 18.7071 5.29289C19.0976 5.68342 19.0976 6.31658 18.7071 6.70711L13.4142 12L18.7071 17.2929C19.0976 17.6834 19.0976 18.3166 18.7071 18.7071C18.3166 19.0976 17.6834 19.0976 17.2929 18.7071L12 13.4142L6.70711 18.7071C6.31658 19.0976 5.68342 19.0976 5.29289 18.7071C4.90237 18.3166 4.90237 17.6834 5.29289 17.2929L10.5858 12L5.29289 6.70711C4.90237 6.31658 4.90237 5.68342 5.29289 5.29289Z"/>
</svg>
</div>
<p>This dialog allows you to submit important metadata for uploaded tracks.</p>
<form id = "track-metadata-form">
    <label for="track-metadata-name">Track Name</label>
    <input required id="track-metadata-name" type="text">
    <label for="track-metadata-version">Version Number</label>
    <input required id="track-metadata-version" type="text" pattern="v?\d\.?\d">
    <label for="track-metadata-course">Select slot ID to load to</label>
    <select id="track-metadata-course"></select>
    <label for="track-metadata-music">Select slot ID for music</label>
    <select id="track-metadata-music"></select>
    <button id="track-metadata-submit" type="submit">Submit</button>
</form>
</div>
</dialog>
