
 // --------------------------------------------------
// API CONFIGURATION
// --------------------------------------------------

const API_BASE_URL = 'https://waafiyyamart-backend.onrender.com';

function apiUrl(path) {
    return `${API_BASE_URL}${path.startsWith('/') ? path : `/${path}`}`;
}


// --------------------------------------------------
// COMMON FUNCTIONS
// --------------------------------------------------

async function postJson(url, payload, token = null) {
    const headers = {
        'Content-Type': 'application/json'
    };

    if (token) {
        headers.Authorization = `Bearer ${token}`;
    }

    const response = await fetch(url, {
        method: 'POST',
        headers,
        body: JSON.stringify(payload)
    });

    const data = await response.json().catch(() => ({}));

    return { response, data };
}

function getStoredUser() {
    try {
        const raw = localStorage.getItem('waafiyyaUser');
        return raw ? JSON.parse(raw) : null;
    } catch (error) {
        return null;
    }
}

function setMessage(element, text, type) {
    if (!element) return;

    element.textContent = text;
    element.classList.remove('success', 'error');

    if (type) {
        element.classList.add(type);
    }
}

function redirectIfLoggedIn() {
    const user = getStoredUser();

    const path = window.location.pathname.split('/').pop();

    if (
        user &&
        (path === 'login.html' || path === 'register.html')
    ) {
        window.location.href =
            user.role === 'seller' ? 'seller.html' : 'products.html';
    }
}


// --------------------------------------------------
// AUTHENTICATION ELEMENTS
// --------------------------------------------------

const loginForm = document.getElementById('loginForm');
const loginMessage = document.getElementById('loginMessage');

const registerForm = document.getElementById('registerForm');
const registerMessage = document.getElementById('registerMessage');

if (document.body.dataset.page === 'auth') {
    redirectIfLoggedIn();
}


// --------------------------------------------------
// LOGIN
// --------------------------------------------------

if (loginForm && loginMessage) {
    loginForm.addEventListener('submit', async (event) => {
        event.preventDefault();

        const email = document.getElementById('email').value.trim();
        const password = document.getElementById('password').value;

        if (!email || !password) {
            setMessage(
                loginMessage,
                'Please enter both your email and password.',
                'error'
            );
            return;
        }

        const isEmailValid = /^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(email);

        if (!isEmailValid) {
            setMessage(loginMessage, 'Please enter a valid email address.', 'error');
            return;
        }

        try {
            const { response, data } = await postJson(
                apiUrl('/api/login'),
                { email, password }
            );

            if (!response.ok) {
                setMessage(loginMessage, data.message || 'Login failed.', 'error');
                return;
            }

            if (!data.user) {
                setMessage(loginMessage, 'Server response did not include user details.', 'error');
                return;
            }

            const user = {
                ...data.user,
                rememberMe: document.getElementById('rememberMe')?.checked || false
            };

            localStorage.setItem('waafiyyaUser', JSON.stringify(user));

            setMessage(loginMessage, data.message || 'Login successful!', 'success');
            loginForm.reset();

            setTimeout(() => {
                window.location.href =
                    user.role === 'seller' ? 'seller.html' : 'products.html';
            }, 600);

        } catch (error) {
            console.error('Login error:', error);
            setMessage(
                loginMessage,
                'Unable to reach the server. Please try again.',
                'error'
            );
        }
    });
}


// --------------------------------------------------
// REGISTER
// --------------------------------------------------

if (registerForm && registerMessage) {
    registerForm.addEventListener('submit', async (event) => {
        event.preventDefault();

        const fullName = document.getElementById('fullName').value.trim();
        const email = document.getElementById('email').value.trim();
        const password = document.getElementById('password').value;

        if (!fullName || !email || !password) {
            setMessage(registerMessage, 'Please fill in all fields.', 'error');
            return;
        }

        const isEmailValid = /^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(email);

        if (!isEmailValid) {
            setMessage(registerMessage, 'Please enter a valid email address.', 'error');
            return;
        }

        try {
            const { response, data } = await postJson(
                apiUrl('/api/register'),
                {
                    full_name: fullName,
                    email,
                    password
                }
            );

            if (!response.ok) {
                setMessage(
                    registerMessage,
                    data.message || 'Registration failed.',
                    'error'
                );
                return;
            }

            setMessage(
                registerMessage,
                data.message || 'Registration successful.',
                'success'
            );

            registerForm.reset();

            setTimeout(() => {
                window.location.href = 'login.html';
            }, 800);

        } catch (error) {
            console.error('Registration error:', error);
            setMessage(
                registerMessage,
                'Unable to reach the server. Please try again.',
                'error'
            );
        }
    });
}


// --------------------------------------------------
// SHOP / PRODUCTS / CART
// --------------------------------------------------

if (document.body.dataset.page === 'shop') {
    const productGrid = document.getElementById('productGrid');
    const cartItemsList = document.getElementById('cartItems');
    const cartCount = document.getElementById('cartCount');
    const cartTotal = document.getElementById('cartTotal');
    const orderMessage = document.getElementById('orderMessage');
    const userLabel = document.getElementById('userLabel');
    const logoutBtn = document.getElementById('logoutBtn');

    const user = getStoredUser();

    if (!user) {
        window.location.href = 'login.html';
    } else {
        if (userLabel) {
            userLabel.textContent = user.full_name || user.email || 'User';
        }

        if (logoutBtn) {
            logoutBtn.addEventListener('click', async () => {
                try {
                    if (user.token) {
                        await fetch(apiUrl('/api/logout'), {
                            method: 'POST',
                            headers: {
                                Authorization: `Bearer ${user.token}`
                            }
                        });
                    }
                } catch (error) {
                    console.error('Could not revoke user session:', error);
                } finally {
                    localStorage.removeItem('waafiyyaUser');
                    window.location.replace('login.html');
                }
            });
        }
    }


    // --------------------------------------------------
    // CART STORAGE
    // --------------------------------------------------

    let cart = [];

    try {
        cart = JSON.parse(localStorage.getItem('waafiyyaCart') || '[]');

        if (!Array.isArray(cart)) {
            cart = [];
        }
    } catch (error) {
        cart = [];
    }

    function saveCart() {
        localStorage.setItem('waafiyyaCart', JSON.stringify(cart));
    }

    function getCartTotal() {
        return cart.reduce(
            (sum, item) => sum + (Number(item.price) * Number(item.quantity)),
            0
        );
    }


    // --------------------------------------------------
    // RENDER CART
    // --------------------------------------------------

    function renderCart() {
        if (!cartItemsList) return;

        cartItemsList.replaceChildren();

        if (!cart.length) {
            const emptyItem = document.createElement('li');
            emptyItem.className = 'empty-state';
            emptyItem.textContent = 'Your cart is empty.';
            cartItemsList.appendChild(emptyItem);

            if (cartTotal) cartTotal.textContent = '$0.00';
            if (cartCount) cartCount.textContent = '0';
            return;
        }

        cart.forEach((item) => {
            const li = document.createElement('li');
            li.className = 'cart-item';

            const details = document.createElement('div');
            const name = document.createElement('strong');
            name.textContent = item.name;

            const quantity = document.createElement('span');
            quantity.textContent = `Qty: ${item.quantity}`;

            details.append(name, quantity);

            const actions = document.createElement('div');

            const price = document.createElement('strong');
            price.textContent =
                `$${(Number(item.price) * Number(item.quantity)).toFixed(2)}`;

            const removeButton = document.createElement('button');
            removeButton.className = 'remove-btn';
            removeButton.dataset.id = String(item.id);
            removeButton.type = 'button';
            removeButton.textContent = 'Remove';

            actions.append(price, removeButton);
            li.append(details, actions);
            cartItemsList.appendChild(li);
        });

        if (cartCount) {
            cartCount.textContent = String(
                cart.reduce((sum, item) => sum + Number(item.quantity), 0)
            );
        }

        if (cartTotal) {
            cartTotal.textContent = `$${getCartTotal().toFixed(2)}`;
        }
    }


    // --------------------------------------------------
    // PRODUCT IMAGES
    // --------------------------------------------------

    function getProductFallbackImage(product) {
        const category = String(product.category || '').trim().toLowerCase();

        const fallbackByCategory = {
            clothing: '/images/tshirt.svg',
            footwear: '/images/sneakers.svg',
            bags: '/images/backpack.svg',
            accessories: '/images/bottle.svg'
        };

        return fallbackByCategory[category] || '/images/product-placeholder.svg';
    }

    function getProductImage(product) {
        const fallback = getProductFallbackImage(product);

        if (
            typeof product.image_url !== 'string' ||
            !product.image_url.trim()
        ) {
            return fallback;
        }

        try {
            const imageUrl = new URL(product.image_url, window.location.origin);

            if (
                imageUrl.protocol !== 'https:' &&
                imageUrl.origin !== window.location.origin
            ) {
                return fallback;
            }

            return imageUrl.href;
        } catch (error) {
            return fallback;
        }
    }


    // --------------------------------------------------
    // PRODUCT SEARCH / FILTER
    // --------------------------------------------------

    let availableProducts = [];

    const productSearch = document.getElementById('productSearch');
    const categoryFilter = document.getElementById('categoryFilter');

    function renderProducts() {
        if (!productGrid) return;

        const query = (productSearch?.value || '').trim().toLowerCase();
        const category = categoryFilter?.value || '';

        const filteredProducts = availableProducts.filter((product) => {
            const searchableText =
                `${product.name || ''} ${product.description || ''}`.toLowerCase();

            return (
                (!query || searchableText.includes(query)) &&
                (!category || product.category === category)
            );
        });

        productGrid.replaceChildren();

        if (!filteredProducts.length) {
            const empty = document.createElement('p');
            empty.className = 'empty-state';
            empty.textContent = availableProducts.length
                ? 'No products match your filters.'
                : 'No products are available yet.';

            productGrid.appendChild(empty);
            return;
        }

        filteredProducts.forEach((product) => {
            const card = document.createElement('article');
            card.className = 'product-card';

            const image = document.createElement('img');
            image.src = getProductImage(product);
            image.alt = product.name || 'Product';
            image.loading = 'lazy';

            image.addEventListener('error', () => {
                image.onerror = null;
                image.src = getProductFallbackImage(product);
            });

            const info = document.createElement('div');
            info.className = 'product-info';

            const tag = document.createElement('span');
            tag.className = 'product-tag';
            tag.textContent = product.category || 'General';

            const name = document.createElement('h3');
            name.textContent = product.name || '';

            const description = document.createElement('p');
            description.textContent = product.description || '';

            const footer = document.createElement('div');
            footer.className = 'product-footer';

            const price = document.createElement('strong');
            price.textContent = `$${Number(product.price).toFixed(2)}`;

            const addButton = document.createElement('button');
            addButton.className = 'add-btn';
            addButton.dataset.id = String(product.id);
            addButton.dataset.name = product.name || '';
            addButton.dataset.price = String(product.price);
            addButton.type = 'button';
            addButton.textContent = 'Add to cart';

            footer.append(price, addButton);
            info.append(tag, name, description, footer);
            card.append(image, info);
            productGrid.appendChild(card);
        });
    }

    productSearch?.addEventListener('input', renderProducts);
    categoryFilter?.addEventListener('change', renderProducts);


    // --------------------------------------------------
    // LOAD PRODUCTS FROM BACKEND
    // --------------------------------------------------

    async function loadProducts() {
        try {
            const response = await fetch(apiUrl('/api/products'));

            const data = await response.json();

            if (!response.ok || !Array.isArray(data)) {
                throw new Error('Unable to load products');
            }

            availableProducts = data;

            const categories = [
                ...new Set(data.map((product) => product.category).filter(Boolean))
            ].sort((left, right) => left.localeCompare(right));

            if (categoryFilter) {
                const selected = categoryFilter.value;

                categoryFilter.replaceChildren(
                    new Option('All categories', ''),
                    ...categories.map((name) => new Option(name, name))
                );

                categoryFilter.value = categories.includes(selected) ? selected : '';
            }

            renderProducts();

        } catch (error) {
            console.error('Product loading error:', error);

            if (productGrid) {
                productGrid.replaceChildren();

                const message = document.createElement('p');
                message.className = 'empty-state';
                message.textContent = 'Could not load products right now.';

                productGrid.appendChild(message);
            }
        }
    }


    // --------------------------------------------------
    // ADD / REMOVE CART ITEMS
    // --------------------------------------------------

    document.addEventListener('click', (event) => {
        const target = event.target;

        if (!(target instanceof Element)) return;

        const addBtn = target.closest('.add-btn');

        if (addBtn) {
            const product = {
                id: Number(addBtn.dataset.id),
                name: addBtn.dataset.name,
                price: Number(addBtn.dataset.price),
                quantity: 1
            };

            const existing = cart.find((item) => item.id === product.id);

            if (existing) {
                existing.quantity += 1;
            } else {
                cart.push(product);
            }

            saveCart();
            renderCart();
        }

        const removeBtn = target.closest('.remove-btn');

        if (removeBtn) {
            const id = Number(removeBtn.dataset.id);

            cart = cart.filter((item) => item.id !== id);

            saveCart();
            renderCart();
        }
    });


    // --------------------------------------------------
    // PLACE ORDER
    // --------------------------------------------------

    const placeOrderBtn = document.getElementById('placeOrderBtn');

    if (placeOrderBtn) {
        placeOrderBtn.addEventListener('click', async () => {
            if (!cart.length) {
                setMessage(orderMessage, 'Your cart is empty.', 'error');
                return;
            }

            if (!user?.id) {
                setMessage(orderMessage, 'Please log in again before placing an order.', 'error');
                return;
            }

            try {
                const { response, data } = await postJson(
                    apiUrl('/api/orders'),
                    {
                        user_id: user.id,
                        items: cart.map((item) => ({
                            product_id: item.id,
                            quantity: item.quantity
                        }))
                    },
                    user.token || null
                );

                if (!response.ok) {
                    setMessage(
                        orderMessage,
                        data.message || 'Order could not be placed.',
                        'error'
                    );
                    return;
                }

                cart = [];
                saveCart();
                renderCart();

                const total = Number(data.order?.total_amount || 0).toFixed(2);

                setMessage(
                    orderMessage,
                    `${data.message || 'Order placed successfully.'} Total: $${total}`,
                    'success'
                );

            } catch (error) {
                console.error('Order error:', error);

                setMessage(
                    orderMessage,
                    'Unable to place order right now.',
                    'error'
                );
            }
        });
    }


    // --------------------------------------------------
    // INITIAL PAGE LOAD
    // --------------------------------------------------

    renderCart();
    loadProducts();
}
