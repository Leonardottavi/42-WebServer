// Console logging for debugging
console.log('✅ JavaScript loaded successfully');

// Test JS functionality
function testJS() {
    const resultElement = document.getElementById('js-result');
    if (resultElement) {
        resultElement.innerHTML = '✅ JavaScript is working perfectly!';
        resultElement.style.color = '#56ab2f';
        resultElement.style.fontWeight = 'bold';
    }
    console.log('✅ JS Test Passed');
}

// Initialization on page load
window.addEventListener('DOMContentLoaded', function() {
    console.log('✅ Page loaded, all static files served correctly');
});

// Generic logger
function log(message) {
    console.log(`[WebServer] ${message}`);
}

log('JavaScript module initialized');
