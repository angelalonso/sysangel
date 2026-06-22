document.addEventListener("DOMContentLoaded", () => {
    // Application initially starts directly at the 'main' viewport
    showScreen('screen-main');
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
};

window.goToConfigScreen = function() {
    showScreen('screen-config');
    fetchConfig();
};

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

function addMixTape() {
    callNative("addMixTape");
}
