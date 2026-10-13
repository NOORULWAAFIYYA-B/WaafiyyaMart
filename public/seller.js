'use strict';

const sellerUser = (() => {
    try {
        return JSON.parse(localStorage.getItem('waafiyyaUser') || 'null');
    } catch {
        return null;
    }
})();

if (!sellerUser || !sellerUser.token) {
    window.location.replace('login.html');
} else if (sellerUser.role !== 'seller') {
    window.location.replace('products.html');
}

const sellerForm = document.getElementById('addProductForm');
const editForm = document.getElementById('editProductForm');
const editCard = document.getElementById('editCard');
const productTableBody = document.getElementById('productTableBody');
const fallbackImage = '/images/product-placeholder.svg';
let products = [];
let addPreviewUrl = null;
let editPreviewUrl = null;

function showSellerMessage(element, message, type = '') {
    element.textContent = message;
    element.className = `message ${type}`.trim();
}

function authenticatedHeaders(extra = {}) {
    return {
        ...extra,
        Authorization: `Bearer ${sellerUser.token}`
    };
}

async function readResponse(response) {
    const contentType = response.headers.get('content-type') || '';
    if (contentType.includes('application/json')) {
        return response.json();
    }
    const text = await response.text();
    throw new Error(text || `Request failed (${response.status}).`);
}

function safeImageUrl(value) {
    if (typeof value !== 'string' || !value.trim()) {
        return fallbackImage;
    }
    try {
        const parsed = new URL(value, window.location.origin);
        if (parsed.protocol !== 'https:' && parsed.origin !== window.location.origin) {
            return fallbackImage;
        }
        return parsed.href;
    } catch {
        return fallbackImage;
    }
}

function setPreview(image, file, previousUrl) {
    if (previousUrl) {
        URL.revokeObjectURL(previousUrl);
    }
    if (!file) {
        image.src = fallbackImage;
        return null;
    }
    const url = URL.createObjectURL(file);
    image.src = url;
    image.onerror = () => {
        image.onerror = null;
        image.src = fallbackImage;
    };
    return url;
}

function isValidSelectedImage(file, messageElement) {
    if (!file) {
        return true;
    }
    const supportedTypes = ['image/jpeg', 'image/png', 'image/webp'];
    if (!supportedTypes.includes(file.type)) {
        showSellerMessage(messageElement, 'Choose a JPEG, PNG, or WebP image.', 'error');
        return false;
    }
    if (file.size > 5 * 1024 * 1024) {
        showSellerMessage(messageElement, 'Images must be 5 MB or smaller.', 'error');
        return false;
    }
    return true;
}

function formValues(form, prefix) {
    return {
        name: document.getElementById(`${prefix}ProductName`).value.trim(),
        description: document.getElementById(`${prefix}ProductDescription`).value.trim(),
        price: document.getElementById(`${prefix}ProductPrice`).value,
        stock: document.getElementById(`${prefix}ProductStock`).value,
        category: document.getElementById(`${prefix}ProductCategory`).value.trim()
    };
}

function appendProductFields(data, product) {
    Object.entries(product).forEach(([key, value]) => data.append(key, value));
}

function makeImage(url, alt, className) {
    const image = document.createElement('img');
    image.src = safeImageUrl(url);
    image.alt = alt;
    image.className = className;
    image.loading = 'lazy';
    image.addEventListener('error', () => {
        image.onerror = null;
        image.src = fallbackImage;
    });
    return image;
}

function renderProducts() {
    productTableBody.replaceChildren();
    if (!products.length) {
        const row = document.createElement('tr');
        const cell = document.createElement('td');
        cell.colSpan = 6;
        cell.textContent = 'You have not added any products yet.';
        row.appendChild(cell);
        productTableBody.appendChild(row);
        return;
    }

    products.forEach((product) => {
        const row = document.createElement('tr');
        const imageCell = document.createElement('td');
        imageCell.appendChild(makeImage(product.image_url, product.name, 'product-thumbnail'));
        row.appendChild(imageCell);
        [product.name, `₹${Number(product.price).toFixed(2)}`, product.category, String(product.stock)]
            .forEach((value) => {
                const cell = document.createElement('td');
                cell.textContent = value;
                row.appendChild(cell);
            });
        const actions = document.createElement('td');
        actions.className = 'actions';
        const editButton = document.createElement('button');
        editButton.type = 'button';
        editButton.className = 'edit-btn';
        editButton.dataset.action = 'edit';
        editButton.dataset.id = String(product.id);
        editButton.textContent = 'Edit';
        const deleteButton = document.createElement('button');
        deleteButton.type = 'button';
        deleteButton.className = 'delete-btn';
        deleteButton.dataset.action = 'delete';
        deleteButton.dataset.id = String(product.id);
        deleteButton.textContent = 'Delete';
        actions.append(editButton, deleteButton);
        row.appendChild(actions);
        productTableBody.appendChild(row);
    });
}

async function loadProducts() {
    const message = document.getElementById('tableMessage');
    showSellerMessage(message, 'Loading your products...');
    try {
        const response = await fetch('/api/admin/products', {
            headers: authenticatedHeaders()
        });
        const data = await readResponse(response);
        if (!response.ok) {
            throw new Error(data.message || 'Unable to load your products.');
        }
        products = Array.isArray(data.products) ? data.products : [];
        renderProducts();
        showSellerMessage(message, '');
    } catch (error) {
        console.error('Loading seller products failed:', error);
        showSellerMessage(message, error.message || 'Unable to load your products.', 'error');
    }
}

function validateFileInput(input, message) {
    const file = input.files[0];
    if (!isValidSelectedImage(file, message)) {
        input.value = '';
        return null;
    }
    return file;
}

document.getElementById('productImage').addEventListener('change', (event) => {
    const file = validateFileInput(event.currentTarget, document.getElementById('addMessage'));
    addPreviewUrl = setPreview(document.getElementById('productImagePreview'), file, addPreviewUrl);
});

document.getElementById('editProductImage').addEventListener('change', (event) => {
    const file = validateFileInput(event.currentTarget, document.getElementById('editMessage'));
    if (file) {
        editPreviewUrl = setPreview(document.getElementById('editProductImagePreview'), file, editPreviewUrl);
    }
});

sellerForm.addEventListener('submit', async (event) => {
    event.preventDefault();
    const message = document.getElementById('addMessage');
    const image = document.getElementById('productImage').files[0];
    if (!isValidSelectedImage(image, message)) {
        return;
    }

    const button = document.getElementById('addProductButton');
    button.disabled = true;
    button.textContent = 'Saving...';
    showSellerMessage(message, 'Saving product...');
    const data = new FormData();
    appendProductFields(data, formValues(sellerForm, ''));
    data.append('image', image);

    try {
        const response = await fetch('/api/admin/products', {
            method: 'POST',
            headers: authenticatedHeaders(),
            body: data
        });
        const result = await readResponse(response);
        if (!response.ok) {
            throw new Error(result.message || 'Unable to add product.');
        }
        sellerForm.reset();
        addPreviewUrl = setPreview(document.getElementById('productImagePreview'), null, addPreviewUrl);
        showSellerMessage(message, result.message || 'Product added successfully.', 'success');
        await loadProducts();
    } catch (error) {
        console.error('Adding seller product failed:', error);
        showSellerMessage(message, error.message || 'Unable to add product.', 'error');
    } finally {
        button.disabled = false;
        button.textContent = 'Add Product';
    }
});

function beginEdit(productId) {
    const product = products.find((item) => item.id === productId);
    if (!product) {
        showSellerMessage(document.getElementById('tableMessage'), 'Product not found.', 'error');
        return;
    }
    document.getElementById('editProductId').value = String(product.id);
    document.getElementById('editProductName').value = product.name;
    document.getElementById('editProductDescription').value = product.description;
    document.getElementById('editProductPrice').value = product.price;
    document.getElementById('editProductStock').value = product.stock;
    document.getElementById('editProductCategory').value = product.category;
    document.getElementById('editProductImage').value = '';
    const preview = document.getElementById('editProductImagePreview');
    preview.src = safeImageUrl(product.image_url);
    preview.onerror = () => {
        preview.onerror = null;
        preview.src = fallbackImage;
    };
    if (editPreviewUrl) {
        URL.revokeObjectURL(editPreviewUrl);
        editPreviewUrl = null;
    }
    editCard.classList.remove('hidden');
    editCard.scrollIntoView({ behavior: 'smooth' });
}

editForm.addEventListener('submit', async (event) => {
    event.preventDefault();
    const message = document.getElementById('editMessage');
    const imageInput = document.getElementById('editProductImage');
    const image = imageInput.files[0];
    if (!isValidSelectedImage(image, message)) {
        return;
    }

    const button = document.getElementById('updateProductButton');
    button.disabled = true;
    button.textContent = 'Saving...';
    showSellerMessage(message, 'Saving changes...');
    const data = new FormData();
    appendProductFields(data, formValues(editForm, 'edit'));
    if (image) {
        data.append('image', image);
    }

    try {
        const id = document.getElementById('editProductId').value;
        const response = await fetch(`/api/admin/products/${encodeURIComponent(id)}`, {
            method: 'PUT',
            headers: authenticatedHeaders(),
            body: data
        });
        const result = await readResponse(response);
        if (!response.ok) {
            throw new Error(result.message || 'Unable to update product.');
        }
        showSellerMessage(message, result.message || 'Product updated successfully.', 'success');
        await loadProducts();
        editCard.classList.add('hidden');
        editForm.reset();
        editPreviewUrl = setPreview(document.getElementById('editProductImagePreview'), null, editPreviewUrl);
    } catch (error) {
        console.error('Updating seller product failed:', error);
        showSellerMessage(message, error.message || 'Unable to update product.', 'error');
    } finally {
        button.disabled = false;
        button.textContent = 'Update Product';
    }
});

document.getElementById('cancelEditBtn').addEventListener('click', () => {
    editCard.classList.add('hidden');
    editForm.reset();
    editPreviewUrl = setPreview(document.getElementById('editProductImagePreview'), null, editPreviewUrl);
    showSellerMessage(document.getElementById('editMessage'), '');
});

productTableBody.addEventListener('click', async (event) => {
    const button = event.target.closest('button[data-action]');
    if (!button) {
        return;
    }
    const productId = Number(button.dataset.id);
    if (button.dataset.action === 'edit') {
        beginEdit(productId);
        return;
    }
    if (button.dataset.action !== 'delete' ||
        !window.confirm('Are you sure you want to delete this product?')) {
        return;
    }

    button.disabled = true;
    try {
        const response = await fetch(`/api/admin/products/${encodeURIComponent(productId)}`, {
            method: 'DELETE',
            headers: authenticatedHeaders()
        });
        const result = await readResponse(response);
        if (!response.ok) {
            throw new Error(result.message || 'Unable to delete product.');
        }
        showSellerMessage(document.getElementById('tableMessage'), result.message || 'Product deleted.', 'success');
        await loadProducts();
    } catch (error) {
        console.error('Deleting seller product failed:', error);
        showSellerMessage(document.getElementById('tableMessage'), error.message || 'Unable to delete product.', 'error');
    }
});

document.getElementById('logoutBtn').addEventListener('click', async () => {
    try {
        await fetch('/api/logout', {
            method: 'POST',
            headers: authenticatedHeaders()
        });
    } catch (error) {
        console.error('Could not revoke seller session:', error);
    } finally {
        localStorage.removeItem('waafiyyaUser');
        window.location.replace('login.html');
    }
});

if (sellerUser && sellerUser.role === 'seller' && sellerUser.token) {
    loadProducts();
}
