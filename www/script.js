// Load Files Gallery - VERSIONE DINAMICA
async function loadFilesGallery() {
    const gallery = document.getElementById('files-gallery');
    const loader = document.getElementById('gallery-loader');

    gallery.innerHTML = '';
    loader.style.display = 'block';

    try {
        // Richiedi dinamicamente la lista dei file dalla directory /files
        const response = await fetch('/api/list-files/files');

        if (!response.ok) {
            throw new Error('Failed to fetch file list');
        }

        const data = await response.json();
        const files = data.files || [];

        if (files.length === 0) {
            loader.innerHTML = '<p style="color: #718096; font-size: 0.95rem;">📁 No files yet<br><br>💡 Upload files to see them here!</p>';
            return;
        }

        loader.style.display = 'none';

        // Crea una galleria per ogni file trovato
        files.forEach((filename) => {
            const filepath = '/files/' + filename;
            const isImage = /\.(jpg|jpeg|png|gif|webp)$/i.test(filename);
            const fileItem = document.createElement('div');
            fileItem.className = 'gallery-item';

            const checkbox = document.createElement('input');
            checkbox.type = 'checkbox';
            checkbox.value = filepath;
            checkbox.onclick = (e) => e.stopPropagation();
            fileItem.appendChild(checkbox);

            if (isImage) {
                const img = document.createElement('img');
                img.src = filepath;
                img.onerror = () => {
                    img.style.display = 'none';
                    const icon = document.createElement('div');
                    icon.textContent = '🖼️';
                    icon.style.fontSize = '40px';
                    fileItem.insertBefore(icon, fileItem.firstChild);
                };
                fileItem.appendChild(img);
            } else {
                const icon = document.createElement('div');
                icon.textContent = '📄';
                icon.style.fontSize = '40px';
                icon.style.marginBottom = '8px';
                fileItem.appendChild(icon);
            }

            const name = document.createElement('p');
            name.textContent = filename;
            fileItem.appendChild(name);

            fileItem.onclick = () => { checkbox.checked = !checkbox.checked; };

            gallery.appendChild(fileItem);
        });
    } catch (error) {
        console.error('Error loading files:', error);
        loader.innerHTML = '<p style="color: #718096; font-size: 0.95rem;">❌ Error loading files<br><br>' + error.message + '</p>';
    }
}

// Upload Files
async function uploadFiles() {
    const fileInput = document.getElementById('fileInput');
    const resultDiv = document.getElementById('upload-result');

    if (fileInput.files.length === 0) {
        resultDiv.className = 'result error';
        resultDiv.textContent = '❌ Please select a file';
        resultDiv.style.display = 'block';
        return;
    }

    const formData = new FormData();
    for (let i = 0; i < fileInput.files.length; i++) {
        formData.append('file', fileInput.files[i]);
    }

    try {
        const response = await fetch('/upload', { method: 'POST', body: formData });
        const text = await response.text();

        if (response.ok) {
            resultDiv.className = 'result success';
            resultDiv.innerHTML = '✅ Upload successful!';
        } else {
            resultDiv.className = 'result error';
            resultDiv.innerHTML = '❌ Upload failed!';
        }
        resultDiv.style.display = 'block';
        fileInput.value = '';

        setTimeout(() => { loadFilesGallery(); }, 500);
    } catch (error) {
        resultDiv.className = 'result error';
        resultDiv.textContent = '❌ Error: ' + error.message;
        resultDiv.style.display = 'block';
    }
}

// Delete Selected Files
async function deleteSelectedFiles() {
    const checkboxes = document.querySelectorAll('#files-gallery input[type="checkbox"]:checked');

    if (checkboxes.length === 0) {
        alert('❌ Please select files to delete');
        return;
    }

    if (!confirm(`Delete ${checkboxes.length} file(s)?`)) {
        return;
    }

    let successCount = 0;
    let failCount = 0;

    for (const checkbox of checkboxes) {
        try {
            const filepath = checkbox.value;
            console.log(`Deleting: ${filepath}`);

            const response = await fetch(filepath, { method: 'DELETE' });
            console.log(`Response status: ${response.status} for ${filepath}`);

            if (response.status === 204) {
                successCount++;
                console.log(`✅ Deleted: ${filepath}`);
            } else {
                failCount++;
                console.log(`❌ Failed to delete: ${filepath} (Status: ${response.status})`);
            }
        } catch (error) {
            failCount++;
            console.error(`Error deleting file:`, error);
        }
    }

    await new Promise(resolve => setTimeout(resolve, 300));
    loadFilesGallery();

    if (failCount === 0) {
        alert(`✅ Deleted ${successCount} file(s)`);
    } else {
        alert(`⚠️ Deleted ${successCount} file(s), Failed: ${failCount}`);
    }
}

// Test HTTP Methods
async function testMethod(method) {
    const resultDiv = document.getElementById('method-result');

    // Use appropriate endpoints for each method
    let endpoint = '/';
    let options = { method: method };

    if (method === 'POST') {
        endpoint = '/upload';
        // Send empty FormData for POST test
        options.body = new FormData();
    } else if (method === 'DELETE') {
        // Try to delete a non-existent file (should return 404, not 403)
        endpoint = '/files/test-method-check.txt';
    }

    try {
        const response = await fetch(endpoint, options);

        // Determine if the response is successful based on the method
        let isSuccess = false;
        let statusMessage = '';

        if (method === 'GET') {
            isSuccess = response.status === 200;
            statusMessage = `Status: ${response.status} ${response.statusText}`;
        } else if (method === 'POST') {
            // POST might return 200, 201, or even 400 depending on implementation
            isSuccess = response.status >= 200 && response.status < 500;
            statusMessage = `Status: ${response.status} ${response.statusText}`;
        } else if (method === 'DELETE') {
            // DELETE returns 204 (No Content) on success, 404 if file doesn't exist
            isSuccess = response.status === 204 || response.status === 404;
            statusMessage = response.status === 204
                ? 'Status: 204 No Content (File deleted)'
                : 'Status: 404 Not Found (File doesn\'t exist - normal for test)';
        }

        resultDiv.className = isSuccess ? 'result success' : 'result error';
        resultDiv.innerHTML = `${isSuccess ? '✅' : '⚠️'} ${method} request completed<br>${statusMessage}`;
        resultDiv.style.display = 'block';
    } catch (error) {
        resultDiv.className = 'result error';
        resultDiv.textContent = '❌ Error: ' + error.message;
        resultDiv.style.display = 'block';
    }
}

// Trigger Error
function triggerError(errorCode) {
    const paths = {
        404: '/404.html',
        403: '/403.html',
        405: '/405.html',
        500: '/500.html'
    };
    window.location.href = paths[errorCode];
}

// Load gallery on page load
document.addEventListener('DOMContentLoaded', loadFilesGallery);

console.log('✅ JavaScript loaded successfully from external file');
