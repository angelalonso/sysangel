document.addEventListener("DOMContentLoaded", () => {
    showScreen('screen-main');
    setupKeyboardShortcuts();
    callNative("loadInitialData");
});

window.showScreen = function(screenId) {
    document.querySelectorAll('.screen').forEach(screen => {
        screen.classList.add('hidden');
    });
    const targetScreen = document.getElementById(screenId);
    if (targetScreen) {
        targetScreen.classList.remove('hidden');
    }
    if (screenId === 'screen-mix-tapes') {
        populateMixTapeSelectors();
    }
};

window.goToConfigScreen = function() {
    showScreen('screen-config');
    fetchConfig();
};

function setupKeyboardShortcuts() {
    window.addEventListener('keydown', (event) => {
        if (event.key === 'Escape') {
            const activeScreen = document.querySelector('.screen:not(.hidden)');
            if (activeScreen) {
                if (activeScreen.id === 'screen-main') {
                    showScreen('screen-confirm-exit');
                } else if (activeScreen.id === 'screen-confirm-exit') {
                    showScreen('screen-main');
                } else if (activeScreen.id === 'screen-add-mix-details') {
                    showScreen('screen-mixes');
                } else if (activeScreen.id === 'screen-add-tape-details') {
                    showScreen('screen-tapes');
                } else if (activeScreen.id === 'screen-mix-tapes') {
                    showScreen('screen-main');
                } else {
                    showScreen('screen-main');
                }
            }
        }
    });
}

function confirmExitApp() { callNative("exitApp"); }

window.appState = {
    mixes: [],
    tapes: [],
    mixTapes: []
};

window.rsyncOperations = {};

window.receiveConfig = function(data) {
    const statusCard = document.getElementById('status-card');
    const statusMsg = document.getElementById('status-message');
    const configCard = document.getElementById('config-card');
    const configContent = document.getElementById('config-content');

    if (data.exists) {
        if (statusCard) statusCard.className = "card success";
        if (statusMsg) statusMsg.innerText = "Configuration Loaded Successfully";
        if (configCard) configCard.classList.remove('hidden');
        if (configContent) configContent.innerText = data.content;
    } else {
        if (statusCard) statusCard.className = "card error";
        if (statusMsg) statusMsg.innerText = "Error: Configuration file could not be created.";
        if (configCard) configCard.classList.add('hidden');
    }
};

window.initializeData = function(configContent, dataContent, isError) {
    window.receiveConfig({ exists: !isError, content: configContent });
    
    if (isError) {
        const statusMsg = document.getElementById('status-message');
        if (statusMsg) statusMsg.innerText = "Error: Invalid data_type in cfg.yml (only 'file' is supported).";
        return;
    }

    try {
        if (dataContent) {
            const parsed = JSON.parse(dataContent);
            window.appState.mixes = parsed.mixes || [];
            window.appState.tapes = parsed.tapes || [];
            window.appState.mixTapes = parsed.mixTapes || [];
        }
    } catch (e) {
        console.error("Failed parsing initial configuration JSON file payload structure", e);
    }

    if (!window.appState.mixes) window.appState.mixes = [];
    if (!window.appState.tapes) window.appState.tapes = [];
    if (!window.appState.mixTapes) window.appState.mixTapes = [];

    renderLists();
};

function saveStateToBackend() {
    const jsonString = JSON.stringify(window.appState);
    callNative("saveData:" + jsonString);
}

function renderLists() {
    const mixesList = document.getElementById('mixes-list');
    if (mixesList) {
        if (!window.appState.mixes || window.appState.mixes.length === 0) {
            mixesList.innerHTML = "<p style='color: #777;'>No mixes configured yet.</p>";
        } else {
            mixesList.innerHTML = '<ul style="list-style-type: none; padding: 0; margin: 0;">' + 
                window.appState.mixes.map((mix, index) => {
                    let mObj = typeof mix === 'string' ? { id: "legacy-mix-"+index, name: mix, paths: [mix] } : mix;
                    if (typeof mix === 'string') window.appState.mixes[index] = mObj;
                    return `
                    <li class="list-item" style="display: flex; justify-content: space-between; align-items: center; padding: 10px 5px; border-bottom: 1px solid #444;">
                        <span class="item-label" style="font-size: 1.05rem; color: #e0e0e0; font-weight: 500;">${mObj.name}</span>
                        <button class="btn-secondary btn-action" style="margin: 0; padding: 6px 16px; font-size: 0.85rem;" onclick="editMix('${mObj.id}')">Edit</button>
                    </li>`;
                }).join('') + "</ul>";
        }
    }

    const tapesList = document.getElementById('tapes-list');
    if (tapesList) {
        if (!window.appState.tapes || window.appState.tapes.length === 0) {
            tapesList.innerHTML = "<p style='color: #777;'>No tapes configured yet.</p>";
        } else {
            tapesList.innerHTML = '<ul style="list-style-type: none; padding: 0; margin: 0;">' + 
                window.appState.tapes.map((tape, index) => {
                    let tObj = typeof tape === 'string' ? { id: "legacy-tape-"+index, name: tape, path: tape } : tape;
                    if (typeof tape === 'string') window.appState.tapes[index] = tObj;
                    return `
                    <li class="list-item" style="display: flex; justify-content: space-between; align-items: center; padding: 10px 5px; border-bottom: 1px solid #444;">
                        <span class="item-label" style="font-size: 1.05rem; color: #e0e0e0; font-weight: 500;">${tObj.name}</span>
                        <button class="btn-secondary btn-action" style="margin: 0; padding: 6px 16px; font-size: 0.85rem;" onclick="editTape('${tObj.id}')">Edit</button>
                    </li>`;
                }).join('') + "</ul>";
        }
    }

    renderMixTapesList();
}

function renderMixTapesList() {
    const mixTapesList = document.getElementById('mix-tapes-list');
    if (!mixTapesList) return;
    
    if (!window.appState.mixTapes || window.appState.mixTapes.length === 0) {
        mixTapesList.innerHTML = "<li style='color: #777; padding: 10px 0;'>No mix-tapes created yet.</li>";
        return;
    }
    
    mixTapesList.innerHTML = window.appState.mixTapes.map((mt, index) => {
        const mix = window.appState.mixes.find(m => m.id === mt.mixId);
        const tape = window.appState.tapes.find(t => t.id === mt.tapeId);
        const mixName = mix ? mix.name : "(deleted mix)";
        const tapeName = tape ? tape.name : "(deleted tape)";
        const isRunning = window.rsyncOperations[mt.id] && window.rsyncOperations[mt.id].running;
        
        return `
        <li style="display: flex; justify-content: space-between; align-items: center; padding: 12px 5px; border-bottom: 1px solid #2a2a2a;">
            <div style="display: flex; align-items: center; gap: 20px; flex: 1;">
                <button class="btn-trigger-action" style="padding: 4px 12px; font-size: 0.75rem; background: ${isRunning ? '#ff9800' : '#03dac6'}; color: #000;" onclick="applyMixTape('${mt.id}')" ${isRunning ? 'disabled' : ''}>
                    ${isRunning ? '⏳ Running...' : '▶ Apply'}
                </button>
                <div>
                    <span style="font-size: 1.05rem; color: #e0e0e0; font-weight: 500;">${mt.name}</span>
                    <span style="color: #777; font-size: 0.85rem; margin-left: 15px;">
                        ${mixName} → ${tapeName}
                    </span>
                </div>
            </div>
            <div style="display: flex; gap: 10px;">
                <button class="btn-secondary btn-action" style="margin: 0; padding: 4px 12px; font-size: 0.8rem;" onclick="editMixTape('${mt.id}')">Edit</button>
                <button class="btn-secondary btn-action" style="margin: 0; padding: 4px 12px; font-size: 0.8rem; background: #cf6679; color: white; border-color: #cf6679;" onclick="deleteMixTape('${mt.id}')">Delete</button>
            </div>
        </li>
        `;
    }).join('');
}

function populateMixTapeSelectors() {
    const mixSelect = document.getElementById('mix-tape-mix-select');
    if (mixSelect) {
        const currentValue = mixSelect.value;
        mixSelect.innerHTML = '<option value="">-- Select a Mix --</option>';
        window.appState.mixes.forEach(mix => {
            const option = document.createElement('option');
            option.value = mix.id;
            option.textContent = mix.name;
            mixSelect.appendChild(option);
        });
        if (currentValue) mixSelect.value = currentValue;
    }
    
    const tapeSelect = document.getElementById('mix-tape-tape-select');
    if (tapeSelect) {
        const currentValue = tapeSelect.value;
        tapeSelect.innerHTML = '<option value="">-- Select a Tape --</option>';
        window.appState.tapes.forEach(tape => {
            const option = document.createElement('option');
            option.value = tape.id;
            option.textContent = tape.name;
            tapeSelect.appendChild(option);
        });
        if (currentValue) tapeSelect.value = currentValue;
    }
}

window.startNewMixTape = function() {
    document.getElementById('current-mix-tape-id').value = "";
    document.getElementById('new-mix-tape-name').value = "";
    populateMixTapeSelectors();
    document.getElementById('mix-tape-mix-select').value = "";
    document.getElementById('mix-tape-tape-select').value = "";
    showScreen('screen-mix-tapes');
};

window.editMixTape = function(mixTapeId) {
    const mixTape = window.appState.mixTapes.find(mt => mt.id === mixTapeId);
    if (mixTape) {
        document.getElementById('current-mix-tape-id').value = mixTape.id;
        document.getElementById('new-mix-tape-name').value = mixTape.name;
        populateMixTapeSelectors();
        document.getElementById('mix-tape-mix-select').value = mixTape.mixId || "";
        document.getElementById('mix-tape-tape-select').value = mixTape.tapeId || "";
        showScreen('screen-mix-tapes');
    }
};

window.saveMixTape = function() {
    const name = document.getElementById('new-mix-tape-name').value.trim();
    const mixId = document.getElementById('mix-tape-mix-select').value;
    const tapeId = document.getElementById('mix-tape-tape-select').value;
    const rawId = document.getElementById('current-mix-tape-id').value;
    
    if (!name) {
        alert("Please enter a name for the Mix-Tape.");
        return;
    }
    
    if (!mixId) {
        alert("Please select a Mix.");
        return;
    }
    
    if (!tapeId) {
        alert("Please select a Tape.");
        return;
    }
    
    const mixTapeId = rawId ? rawId : "mixtape-" + Date.now();
    const payload = { id: mixTapeId, name: name, mixId: mixId, tapeId: tapeId };
    
    const existingIndex = window.appState.mixTapes.findIndex(mt => mt.id === mixTapeId);
    if (existingIndex >= 0) {
        window.appState.mixTapes[existingIndex] = payload;
    } else {
        window.appState.mixTapes.push(payload);
    }
    
    saveStateToBackend();
    renderLists();
    showScreen('screen-main');
};

window.deleteMixTape = function(mixTapeId) {
    if (confirm("Are you sure you want to delete this Mix-Tape?")) {
        window.appState.mixTapes = window.appState.mixTapes.filter(mt => mt.id !== mixTapeId);
        saveStateToBackend();
        renderLists();
    }
};

window.applyMixTape = function(mixTapeId) {
    if (window.rsyncOperations[mixTapeId] && window.rsyncOperations[mixTapeId].running) {
        alert("Rsync is already running for this Mix-Tape. Please wait.");
        return;
    }
    
    const mixTape = window.appState.mixTapes.find(mt => mt.id === mixTapeId);
    if (!mixTape) {
        alert("Mix-Tape not found.");
        return;
    }
    
    const mix = window.appState.mixes.find(m => m.id === mixTape.mixId);
    const tape = window.appState.tapes.find(t => t.id === mixTape.tapeId);
    
    if (!mix) {
        alert("Associated Mix not found. It may have been deleted.");
        return;
    }
    
    if (!tape) {
        alert("Associated Tape not found. It may have been deleted.");
        return;
    }
    
    if (!mix.paths || mix.paths.length === 0) {
        alert("The selected Mix has no paths to copy.");
        return;
    }
    
    window.rsyncOperations[mixTapeId] = {
        running: true,
        started: Date.now()
    };
    renderMixTapesList();
    
    const sourcePaths = mix.paths;
    const destPath = tape.path;
    const MAX_PATHS_PER_BATCH = 20;
    
    // Group paths into batches of MAX_PATHS_PER_BATCH
    const batches = [];
    for (let i = 0; i < sourcePaths.length; i += MAX_PATHS_PER_BATCH) {
        batches.push(sourcePaths.slice(i, i + MAX_PATHS_PER_BATCH));
    }
    
    const totalBatches = batches.length;
    let completedBatches = 0;
    let failedBatches = 0;
    let errorMessages = [];
    
    console.log(`[Mix-Tape: ${mixTape.name}] Starting rsync of ${sourcePaths.length} path(s) in ${totalBatches} batch(es) to ${destPath}`);
    
    batches.forEach((batch, batchIndex) => {
        // Build a single rsync command with all paths in this batch
        let cmd = "rsync -av";
        batch.forEach(path => {
            cmd += ` '${path}'`;
        });
        cmd += ` '${destPath}/'`;
        
        const cmdId = `${mixTapeId}-batch-${batchIndex}`;
        
        window.rsyncCallbacks = window.rsyncCallbacks || {};
        window.rsyncCallbacks[cmdId] = function(result, output) {
            completedBatches++;
            if (result !== "success") {
                failedBatches++;
                errorMessages.push(`Batch ${batchIndex+1}: ${result}`);
            }
            
            console.log(`[Mix-Tape: ${mixTape.name}] Batch ${batchIndex+1}/${totalBatches}: ${result}`);
            if (output) {
                console.log(`Output: ${output}`);
            }
            
            if (completedBatches === totalBatches) {
                window.rsyncOperations[mixTapeId].running = false;
                window.rsyncOperations[mixTapeId].completed = Date.now();
                renderMixTapesList();
                
                const successCount = totalBatches - failedBatches;
                let summary = `Rsync completed for "${mixTape.name}":\n`;
                summary += `${successCount} batch(es) succeeded, ${failedBatches} failed.\n`;
                if (failedBatches > 0) {
                    summary += `\nErrors:\n${errorMessages.join('\n')}`;
                }
                console.log(`[Mix-Tape: ${mixTape.name}] ${summary}`);
                alert(summary);
            }
        };
        
        callNative(`rsync:${cmd}|${cmdId}`);
    });
};

window.tempMixPaths = [];

window.startNewMix = function() {
    document.getElementById('current-mix-id').value = "";
    document.getElementById('new-mix-name').value = "";
    window.tempMixPaths = [];
    renderMixPathsInEdit();
    showScreen('screen-add-mix-details');
    callNative("selectMixPaths"); 
};

window.addMixPaths = function() {
    callNative("selectMixPaths");
};

window.addMixFolders = function() {
    callNative("selectMixFolders");
};

window.receiveMixFolder = function(folderPath) {
    if (!folderPath) return;

    window.tempMixPaths = window.tempMixPaths.concat([folderPath]);

    const mixId = document.getElementById('current-mix-id').value;
    const nameInput = document.getElementById('new-mix-name');
    if (!mixId && nameInput.value === "") {
        nameInput.value = folderPath.split(/[/\\]/).pop() + " Mix";
    }

    renderMixPathsInEdit();
};

window.receiveMixFolders = function(foldersArray) {
    if (!foldersArray || foldersArray.length === 0) return;
    
    window.tempMixPaths = window.tempMixPaths.concat(foldersArray);
    
    const mixId = document.getElementById('current-mix-id').value;
    const nameInput = document.getElementById('new-mix-name');
    if (!mixId && nameInput.value === "") {
        nameInput.value = foldersArray[0].split(/[/\\]/).pop() + " Mix";
    }
    
    renderMixPathsInEdit();
};

window.receiveMixPaths = function(pathsArray) {
    if (!pathsArray || pathsArray.length === 0) return;
    
    window.tempMixPaths = window.tempMixPaths.concat(pathsArray);
    
    const mixId = document.getElementById('current-mix-id').value;
    const nameInput = document.getElementById('new-mix-name');
    if (!mixId && nameInput.value === "") {
        nameInput.value = pathsArray[0].split(/[/\\]/).pop() + " Mix";
    }
    
    renderMixPathsInEdit();
};

window.removeMixPath = function(index) {
    window.tempMixPaths.splice(index, 1);
    renderMixPathsInEdit();
};

window.renderMixPathsInEdit = function() {
    const list = document.getElementById('new-mix-paths-list');
    if (window.tempMixPaths.length === 0) {
        list.innerHTML = "<p style='color:#777; font-size: 0.9rem;'>No paths added yet.</p>";
        return;
    }
    list.innerHTML = window.tempMixPaths.map((p, i) => `
        <div style="display: flex; justify-content: space-between; align-items: center; background: #2d2d2d; padding: 8px 12px; margin-bottom: 8px; border-radius: 4px; border: 1px solid #444;">
            <span style="font-family: monospace; color: #a5d6ff; word-break: break-all; margin-right: 15px; font-size: 0.85rem;">${p}</span>
            <button class="btn-secondary" style="padding: 4px 10px; font-size: 0.8rem; background: #cf6679; color: white; border-color: #cf6679;" onclick="removeMixPath(${i})">Remove</button>
        </div>
    `).join('');
};

window.editMix = function(mixId) {
    const mix = window.appState.mixes.find(m => m.id === mixId);
    if (mix) {
        document.getElementById('current-mix-id').value = mix.id;
        document.getElementById('new-mix-name').value = mix.name;
        window.tempMixPaths = [...(mix.paths || [])];
        renderMixPathsInEdit();
        showScreen('screen-add-mix-details');
    }
};

window.saveNewMix = function() {
    const name = document.getElementById('new-mix-name').value;
    const rawId = document.getElementById('current-mix-id').value;
    const mixId = rawId ? rawId : "mix-" + Date.now();
    
    const payload = { id: mixId, name: name, paths: [...window.tempMixPaths] };
    
    const existingIndex = window.appState.mixes.findIndex(m => m.id === mixId);
    if (existingIndex >= 0) {
        window.appState.mixes[existingIndex] = payload;
    } else {
        window.appState.mixes.push(payload);
    }
    
    saveStateToBackend();
    renderLists();
    showScreen('screen-mixes');
};

window.startNewTape = function() {
    document.getElementById('current-tape-id').value = ""; 
    callNative("selectTapeFolder");
};

window.addTape = function() {
    callNative("selectTapeFolder");
};

window.receiveSelectedFolder = function(folderPath) {
    if (!folderPath) return;
    document.getElementById('new-tape-path').innerText = folderPath;
    
    const tapeId = document.getElementById('current-tape-id').value;
    if (!tapeId) {
        document.getElementById('new-tape-name').value = folderPath.split(/[/\\]/).pop() || folderPath;
    }
    showScreen('screen-add-tape-details');
};

window.editTape = function(tapeId) {
    const tape = window.appState.tapes.find(t => t.id === tapeId);
    if (tape) {
        document.getElementById('current-tape-id').value = tape.id;
        document.getElementById('new-tape-name').value = tape.name || tape.path;
        document.getElementById('new-tape-path').innerText = tape.path;
        showScreen('screen-add-tape-details');
    }
};

window.saveNewTape = function() {
    const path = document.getElementById('new-tape-path').innerText;
    const name = document.getElementById('new-tape-name').value;
    const rawId = document.getElementById('current-tape-id').value;
    const tapeId = rawId ? rawId : "tape-" + Date.now();
    
    const payload = { id: tapeId, name: name, path: path };
    
    const existingIndex = window.appState.tapes.findIndex(t => t.id === tapeId);
    if (existingIndex >= 0) {
        window.appState.tapes[existingIndex] = payload;
    } else {
        window.appState.tapes.push(payload);
    }
    
    saveStateToBackend();
    renderLists();
    showScreen('screen-tapes');
};

function callNative(param) {
    if (window.external && window.external.invoke) {
        window.external.invoke(param);
    } else if (window.webkit && window.webkit.messageHandlers && window.webkit.messageHandlers.external) {
        window.webkit.messageHandlers.external.postMessage(param);
    }
}
function fetchConfig() { callNative("getConfig"); }
