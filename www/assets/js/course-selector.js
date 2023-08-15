const RAW_HTML = `
<optgroup label="Mushroom Cup">
    <option value="0x8">Luigi Circuit (1.1)</option>
    <option value="0x1">Moo Moo Meadows (1.2)</option>
    <option value="0x2">Mushroom Gorge (1.3)</option>
    <option value="0x4">Toad's Factory (1.4)</option>
</optgroup>
<optgroup label="Flower Cup">
    <option value="0x0">Mario Circuit (2.1)</option>
    <option value="0x5">Coconut Mall (2.2)</option>
    <option value="0x6">DK Summit (2.3)</option>
    <option value="0x7">Wario's Gold Mine (2.4)</option>
</optgroup>
<optgroup label="Star Cup">
    <option value="0x9">Daisy Circuit (3.1)</option>
    <option value="0xF">Koopa Cape (3.2)</option>
    <option value="0xB">Maple Treeway (3.3)</option>
    <option value="0x3">Grumble Volcano (3.4)</option>
<optgroup label="Special Cup">
    <option value="0xE">Dry Dry Ruins (4.1)</option>
    <option value="0xA">Moonview Highway (4.2)</option>
    <option value="0xC">Bowser's Castle (4.3)</option>
    <option value="0xD">Rainbow Road (4.4)</option>
</optgroup>
<optgroup label="Shell Cup">
    <option value="0x10">GCN Peach Beach (5.1)</option>
    <option value="0x14">DS Yoshi Falls (5.2)</option>
    <option value="0x19">SNES Ghost Valley 2 (5.3)</option>
    <option value="0x1A">N64 Mario Raceway (5.4)</option>
</optgroup>
<optgroup label="Banana Cup">
    <option value="0x1B">N64 Sherbet Land (6.1)</option>
    <option value="0x1F">GBA Shy Guy Beach (6.2)</option>
    <option value="0x17">DS Delfino Square (6.3)</option>
    <option value="0x12">GCN Waluigi Stadium (6.4)</option>
</optgroup>
<optgroup label="Leaf Cup">
    <option value="0x15">DS Desert Hills (7.1)</option>
    <option value="0x1E">GBA Bowser Castle 3 (7.2)</option>
    <option value="0x1D">N64 DK's Jungle Parkway (7.3)</option>
    <option value="0x11">GCN Mario Circuit (7.4)</option>
</optgroup>
<optgroup label="Lightning Cup">
    <option value="0x18">SNES Mario Circuit 3 (8.1)</option>
    <option value="0x16">DS Peach Gardens (8.2)</option>
    <option value="0x13">GCN DK Mountain (8.3)</option>
    <option value="0x1C">N64 Bowser's Castle (8.4)</option>
</optgroup>`;

// selector: An existing <select> element to be filled
export function initializeCourseSelector(selector) {
    const parser = new DOMParser();
    const options = parser.parseFromString(RAW_HTML, "text/html");
    for (const child of options.body.children) {
        selector.appendChild(child);
    }
}

export function courseIdToMusicId(courseId) {
    switch (courseId) {
        case 0x8: 
            return 0x75;
        case 0x1:
            return 0x77;
        case 0x2:
            return 0x79;
        case 0x4:
            return 0x7B;
        case 0x0:
            return 0x7D;
        case 0x5:
            return 0x7F;
        case 0x6:
            return 0x81;
        case 0x7:
            return 0x83;
        case 0x9:
            return 0x87;
        case 0xF:
            return 0x85;
        case 0xB:
            return 0x8F;
        case 0x3:
            return 0x8B;
        case 0xE:
            return 0x89;
        case 0xA:
            return 0x8D;
        case 0xC:
            return 0x91;
        case 0xD:
            return 0x93;
        case 0x10:
            return 0xA5;
        case 0x14:
            return 0xAD;
        case 0x19:
            return 0x97;
        case 0x1A:
            return 0x9F;
        case 0x1B:
            return 0x9D;
        case 0x1F:
            return 0x95;
        case 0x17:
            return 0xAF;
        case 0x12:
            return 0xA9;
        case 0x15:
            return 0xB1;
        case 0x1E:
            return 0x9B;
        case 0x1D:
            return 0xA1;
        case 0x11:
            return 0xA7;
        case 0x18:
            return 0x99;
        case 0x16:
            return 0xB3;
        case 0x13:
            return 0xAB;
        case 0x1C:
            return 0xA3;
        default:
            console.error(`Invalid course ID: ${courseId}`);
    }
}
