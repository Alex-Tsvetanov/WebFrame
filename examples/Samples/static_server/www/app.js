// Coroute v2 - Example JavaScript

document.addEventListener('DOMContentLoaded', function() {
    const statusDiv = document.getElementById('status');
    
    // Fetch API status
    fetch('/api/status')
        .then(response => response.json())
        .then(data => {
            statusDiv.className = 'success';
            statusDiv.innerHTML = `
                <strong>API Status:</strong><br>
                <pre>${JSON.stringify(data, null, 2)}</pre>
            `;
        })
        .catch(error => {
            statusDiv.className = 'error';
            statusDiv.textContent = 'Failed to fetch API status: ' + error.message;
        });
});

console.log('Coroute v2 static file server is working!');
