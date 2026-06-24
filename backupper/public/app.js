document.addEventListener("DOMContentLoaded", () => {
    // Application initially starts directly at the 'main' viewport
    showScreen('screen-main');
    setupKeyboardShortcuts();
});

// Single Window View/Route Controller
window.showScreen = function(screenId) {
    document.querySelectorAll('.screen').forEach(screen => {
        screen.classList.add('hidden');
    });
    const targetScreen = document.getElementById(screenId);
    if (targetScreen) {
        targetScreen.classList.remove('hidden');
    }

    // Automatically load or update the list when visiting the dashboard screen
    if (screenId === 'screen-main') {
        loadMixTapesList();
    }
};

window.goToConfigScreen = function() {
    showScreen('screen-config');
    fetchConfig();
};

// Global Keyboard Shortcut Listener
function setupKeyboardShortcuts() {
    window.addEventListener('keydown', (event) => {
        if (event.key === 'Escape') {
            // Find the currently active screen
            const activeScreen = document.querySelector('.screen:not(.hidden)');
            
            if (activeScreen) {
                if (activeScreen.id === 'screen-main') {
                    // Esc on main screen routes to the exit confirmation view
                    console.log("Esc detected on main screen. Opening confirmation screen.");
                    showScreen('screen-confirm-exit');
                } else if (activeScreen.id === 'screen-confirm-exit') {
                    // Esc on the confirmation screen itself dismisses it and goes back to main
                    showScreen('screen-main');
                } else {
                    // Esc on any other sub-screen goes back to main dashboard
                    showScreen('screen-main');
                }
            }
        }
    });
}

function confirmExitApp() {
    callNative("exitApp");
}

// Pure C Callback hook invoked by backend's webview_eval
window.receiveConfig = function(data) {
    const statusCard = document.getElementById('status-card');
    const statusMsg = document.getElementById('status-message');
    const configCard = document.getElementById('config-card');
    const configContent = document.getElementById('config-content');

    if (data.exists) {
        statusCard.className = "card success";
        statusMsg.innerText = "Configuration Loaded Successfully";
        configCard.classList.remove('hidden');
        configContent.innerText = data.content;
    } else {
        statusCard.className = "card error";
        statusMsg.innerText = "Error: Configuration file could not be created.";
        configCard.classList.add('hidden');
    }
};

// Callback to populate existing mix tapes list dynamically
window.renderMixTapes = function(mixTapesArray) {
    const listContainer = document.getElementById('mix-tapes-list');
    if (!listContainer) return;

    listContainer.innerHTML = '';

    if (!mixTapesArray || mixTapesArray.length === 0) {
        listContainer.innerHTML = '<li style="color: #777; padding: 10px 0;">No existing Mix-Tapes found.</li>';
        return;
    }

    mixTapesArray.forEach(tape => {
        const item = document.createElement('li');
        item.className = "mixtape-item";
        
        const label = document.createElement('span');
        label.innerText = tape.name || "Unnamed Mix-Tape";
        label.className = "mixtape-label";

        const actionBtn = document.createElement('button');
        actionBtn.innerText = "Trigger";
        actionBtn.className = "btn-trigger-action";
        actionBtn.onclick = () => {
            triggerMixTape(tape.id);
        };

        item.appendChild(label);
        item.appendChild(actionBtn);
        listContainer.appendChild(item);
    });
};

// Safe bridge platform handler for Linux WebKit environments
function callNative(param) {
    if (window.external && window.external.invoke) {
        window.external.invoke(param);
    } else if (window.webkit && window.webkit.messageHandlers && window.webkit.messageHandlers.external) {
        window.webkit.messageHandlers.external.postMessage(param);
    } else {
        console.error("Native WebView bridge interface not discovered.");
    }
}

function fetchConfig() {
    callNative("getConfig");
}

function loadMixTapesList() {
    callNative("getMixTapesList");
}

function triggerMixTape(tapeId) {
    callNative("triggerMixTape:" + tapeId);
}

fn_addMixTape = function() {
    callNative("addMixTape");
}
