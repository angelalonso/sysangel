document.addEventListener("DOMContentLoaded", fetchConfig);

// Screen Routing Manager
window.showScreen = function(screenId) {
    document.querySelectorAll('.screen').forEach(screen => {
        screen.classList.add('hidden');
    });
    const targetScreen = document.getElementById(screenId);
    if (targetScreen) {
        targetScreen.classList.remove('hidden');
    }
};

// Pure C Callback hooks called back by C's webview_eval
window.receiveConfig = function(data) {
    const statusCard = document.getElementById('status-card');
    const statusMsg = document.getElementById('status-message');
    const configCard = document.getElementById('config-card');
    const configContent = document.getElementById('config-content');
    const createBtn = document.getElementById('btn-create');

    if (data.exists) {
        statusCard.className = "card success";
        statusMsg.innerText = "Configuration Loaded Successfully";
        configCard.classList.remove('hidden');
        configContent.innerText = data.content;
        createBtn.classList.add('hidden');
    } else {
        statusCard.className = "card error";
        statusMsg.innerText = "Warning: cfg.yml is missing!";
        configCard.classList.add('hidden');
        createBtn.classList.remove('hidden');
    }
};

window.receiveCreateStatus = function(data) {
    if (data.success) {
        fetchConfig();
    } else {
        alert("Failed to create configuration.");
    }
};

// Safe wrapper for legacy WebKit communication in Linux
function callNative(param) {
    if (window.external && window.external.invoke) {
        window.external.invoke(param);
    } else if (window.webkit && window.webkit.messageHandlers && window.webkit.messageHandlers.external) {
        window.webkit.messageHandlers.external.postMessage(param);
    } else {
        console.error("Native WebView bridge not found.");
    }
}

function fetchConfig() {
    callNative("getConfig");
}

function createConfig() {
    callNative("callNative");
}

function addMixTape() {
    callNative("addMixTape");
}
