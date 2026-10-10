// --------------------------------------------------
// COMMON FUNCTIONS
// --------------------------------------------------

async function postJson(url, payload) {

    const response = await fetch(url, {

        method: 'POST',

        headers: {
            'Content-Type': 'application/json'
        },

        body: JSON.stringify(payload)

    });

    const data = await response.json();

    return {
        response,
        data
    };
}


function getStoredUser() {

    try {

        const raw =
            localStorage.getItem('waafiyyaUser');

        return raw ? JSON.parse(raw) : null;

    } catch (error) {

        return null;
    }
}


function setMessage(element, text, type) {

    if (!element) return;

    element.textContent = text;

    element.classList.remove(
        'success',
        'error'
    );

    if (type) {
        element.classList.add(type);
    }
}


function redirectIfLoggedIn() {

    const user = getStoredUser();

    const path =
        window.location.pathname
            .split('/')
            .pop();

    if (
        user &&
        (
            path === 'login.html' ||
            path === 'register.html'
        )
    ) {

        if (user.role === 'seller') {

            window.location.href =
                'seller.html';

        } else {

            window.location.href =
                'products.html';
        }
    }
}


// --------------------------------------------------
// AUTHENTICATION
// --------------------------------------------------

const loginForm =
    document.getElementById('loginForm');

const loginMessage =
    document.getElementById('loginMessage');

const registerForm =
    document.getElementById('registerForm');

const registerMessage =
    document.getElementById('registerMessage');


if (document.body.dataset.page === 'auth') {

    redirectIfLoggedIn();
}


// --------------------------------------------------
// LOGIN
// --------------------------------------------------

if (loginForm && loginMessage) {

    loginForm.addEventListener(
        'submit',
        async (event) => {

            event.preventDefault();


            const email =
                document.getElementById(
                    'email'
                ).value.trim();


            const password =
                document.getElementById(
                    'password'
                ).value.trim();


            if (!email || !password) {

                setMessage(
                    loginMessage,
                    'Please enter both your email and password.',
                    'error'
                );

                return;
            }


            const isEmailValid =
                /^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(
                    email
                );


            if (!isEmailValid) {

                setMessage(
                    loginMessage,
                    'Please enter a valid email address.',
                    'error'
                );

                return;
            }


            try {

                const { response, data } =
                    await postJson(
                        '/api/login',
                        {
                            email: email,
                            password: password
                        }
                    );


                if (!response.ok) {

                    setMessage(
                        loginMessage,
                        data.message ||
                            'Login failed.',
                        'error'
                    );

                    return;
                }


                const user = {

                    ...data.user,

                    rememberMe:
                        document.getElementById(
                            'rememberMe'
                        )?.checked || false

                };


                localStorage.setItem(
                    'waafiyyaUser',
                    JSON.stringify(user)
                );


                setMessage(
                    loginMessage,
                    data.message ||
                        'Login successful!',
                    'success'
                );


                loginForm.reset();


                // ------------------------------------------
                // ROLE-BASED REDIRECT
                // ------------------------------------------

                setTimeout(() => {

                    if (user.role === 'seller') {

                        window.location.href =
                            'seller.html';

                    } else {

                        window.location.href =
                            'products.html';
                    }

                }, 600);

            }
            catch (error) {

                console.error(
                    'Login error:',
                    error
                );

                setMessage(
                    loginMessage,
                    'Unable to reach the server. Please try again.',
                    'error'
                );
            }
        }
    );
}


// --------------------------------------------------
// REGISTER
// --------------------------------------------------

if (registerForm && registerMessage) {

    registerForm.addEventListener(
        'submit',
        async (event) => {

            event.preventDefault();


            const fullName =
                document.getElementById(
                    'fullName'
                ).value.trim();


            const email =
                document.getElementById(
                    'email'
                ).value.trim();


            const password =
                document.getElementById(
                    'password'
                ).value.trim();


            if (
                !fullName ||
                !email ||
                !password
            ) {

                setMessage(
                    registerMessage,
                    'Please fill in all fields.',
                    'error'
                );

                return;
            }


            const isEmailValid =
                /^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(
                    email
                );


            if (!isEmailValid) {

                setMessage(
                    registerMessage,
                    'Please enter a valid email address.',
                    'error'
                );

                return;
            }


            try {

                const { response, data } =
                    await postJson(
                        '/api/register',
                        {
                            full_name: fullName,
                            email: email,
                            password: password
                        }
                    );


                if (!response.ok) {

                    setMessage(
                        registerMessage,
                        data.message ||
                            'Registration failed.',
                        'error'
                    );

                    return;
                }


                setMessage(
                    registerMessage,
                    data.message ||
                        'Registration successful.',
                    'success'
                );


                registerForm.reset();


                setTimeout(() => {

                    window.location.href =
                        'login.html';

                }, 800);

            }
            catch (error) {

                console.error(
                    'Registration error:',
                    error
                );

                setMessage(
                    registerMessage,
                    'Unable to reach the server. Please try again.',
                    'error'
                );
            }
        }
    );
}


// --------------------------------------------------
// SHOP / PRODUCTS / CART
// --------------------------------------------------

if (document.body.dataset.page === 'shop') {

    const productGrid =
        document.getElementById(
            'productGrid'
        );


    const cartItemsList =
        document.getElementById(
            'cartItems'
        );


    const cartCount =
        document.getElementById(
            'cartCount'
        );


    const cartTotal =
        document.getElementById(
            'cartTotal'
        );


    const orderMessage =
        document.getElementById(
            'orderMessage'
        );


    const userLabel =
        document.getElementById(
            'userLabel'
        );


    const user =
        getStoredUser();


    // --------------------------------------------------
    // CHECK LOGIN
    // --------------------------------------------------

    if (!user) {

        window.location.href =
            'login.html';

    } else {

        if (userLabel) {

            userLabel.textContent =
                user.full_name ||
                user.email;
        }
    }


    // --------------------------------------------------
    // LOAD CART FROM BROWSER STORAGE
    // --------------------------------------------------

    let cart =
        JSON.parse(
            localStorage.getItem(
                'waafiyyaCart'
            ) || '[]'
        );


    function saveCart() {

        localStorage.setItem(
            'waafiyyaCart',
            JSON.stringify(cart)
        );
    }


    function getCartTotal() {

        return cart.reduce(

            (sum, item) =>
                sum +
                (
                    item.price *
                    item.quantity
                ),

            0
        );
    }


    // --------------------------------------------------
    // RENDER CART
    // --------------------------------------------------

    function renderCart() {

        if (!cartItemsList) return;


        cartItemsList.innerHTML = '';


        if (!cart.length) {

            cartItemsList.innerHTML =
                '<li class="empty-state">Your cart is empty.</li>';


            if (cartTotal) {

                cartTotal.textContent =
                    '$0.00';
            }


            if (cartCount) {

                cartCount.textContent =
                    '0';
            }

            return;
        }


        cart.forEach((item) => {

            const li =
                document.createElement(
                    'li'
                );


            li.className =
                'cart-item';


            li.innerHTML = `

                <div>

                    <strong>
                        ${item.name}
                    </strong>

                    <span>
                        Qty: ${item.quantity}
                    </span>

                </div>


                <div>

                    <strong>
                        $${(
                            item.price *
                            item.quantity
                        ).toFixed(2)}
                    </strong>


                    <button
                        class="remove-btn"
                        data-id="${item.id}">
                        Remove
                    </button>

                </div>

            `;


            cartItemsList.appendChild(li);

        });


        if (cartCount) {

            cartCount.textContent =
                String(

                    cart.reduce(
                        (sum, item) =>
                            sum +
                            item.quantity,
                        0
                    )

                );
        }


        if (cartTotal) {

            cartTotal.textContent =
                `$${getCartTotal().toFixed(2)}`;
        }
    }


    // --------------------------------------------------
    // PRODUCT IMAGE FUNCTION
    // --------------------------------------------------

    function getProductImage(product) {

        if (
            product.image_url &&
            product.image_url.trim() !== ''
        ) {

            return product.image_url;
        }


        const productImages = {

            'Classic T-Shirt':
                '/images/tshirt.svg',

            'Casual Sneakers':
                '/images/sneakers.svg',

            'College Backpack':
                '/images/backpack.svg',

            'Stainless Steel Bottle':
                '/images/bottle.svg'

        };


        return (
            productImages[product.name] ||
            '/images/tshirt.svg'
        );
    }


    // --------------------------------------------------
    // LOAD PRODUCTS
    // --------------------------------------------------

    async function loadProducts() {

        try {

            const response =
                await fetch(
                    '/api/products'
                );


            const data =
                await response.json();


            if (
                !response.ok ||
                !Array.isArray(data)
            ) {

                throw new Error(
                    'Unable to load products'
                );
            }


            if (!productGrid) return;


            productGrid.innerHTML =

                data.map((product) => {

                    const image =
                        getProductImage(
                            product
                        );


                    return `

                        <article
                            class="product-card">

                            <img
                                src="${image}"
                                alt="${product.name}"
                                loading="lazy"
                            />


                            <div
                                class="product-info">

                                <span
                                    class="product-tag">

                                    ${product.category}

                                </span>


                                <h3>
                                    ${product.name}
                                </h3>


                                <p>
                                    ${product.description}
                                </p>


                                <div
                                    class="product-footer">

                                    <strong>

                                        $${Number(
                                            product.price
                                        ).toFixed(2)}

                                    </strong>


                                    <button
                                        class="add-btn"
                                        data-id="${product.id}"
                                        data-name="${product.name}"
                                        data-price="${product.price}">

                                        Add to cart

                                    </button>

                                </div>

                            </div>

                        </article>

                    `;

                }).join('');

        }
        catch (error) {

            console.error(
                'Product loading error:',
                error
            );


            if (productGrid) {

                productGrid.innerHTML =
                    '<p class="empty-state">Could not load products right now.</p>';
            }
        }
    }


    // --------------------------------------------------
    // ADD / REMOVE CART ITEMS
    // --------------------------------------------------

    document.addEventListener(
        'click',
        (event) => {


            // ------------------------------------------
            // ADD TO CART
            // ------------------------------------------

            const addBtn =
                event.target.closest(
                    '.add-btn'
                );


            if (addBtn) {

                const product = {

                    id:
                        Number(
                            addBtn.dataset.id
                        ),

                    name:
                        addBtn.dataset.name,

                    price:
                        Number(
                            addBtn.dataset.price
                        ),

                    quantity: 1
                };


                const existing =
                    cart.find(
                        (item) =>
                            item.id ===
                            product.id
                    );


                if (existing) {

                    existing.quantity += 1;

                } else {

                    cart.push(product);
                }


                saveCart();

                renderCart();
            }


            // ------------------------------------------
            // REMOVE FROM CART
            // ------------------------------------------

            const removeBtn =
                event.target.closest(
                    '.remove-btn'
                );


            if (removeBtn) {

                const id =
                    Number(
                        removeBtn.dataset.id
                    );


                cart =
                    cart.filter(
                        (item) =>
                            item.id !== id
                    );


                saveCart();

                renderCart();
            }
        }
    );


    // --------------------------------------------------
    // PLACE ORDER
    // --------------------------------------------------

    const placeOrderBtn =
        document.getElementById(
            'placeOrderBtn'
        );


    if (placeOrderBtn) {

        placeOrderBtn.addEventListener(
            'click',
            async () => {


                if (!cart.length) {

                    setMessage(
                        orderMessage,
                        'Your cart is empty.',
                        'error'
                    );

                    return;
                }


                try {

                    const {
                        response,
                        data
                    } = await postJson(

                        '/api/orders',

                        {

                            user_id:
                                user.id,

                            items:
                                cart.map(
                                    (item) => ({

                                        product_id:
                                            item.id,

                                        quantity:
                                            item.quantity

                                    })
                                )

                        }
                    );


                    if (!response.ok) {

                        setMessage(
                            orderMessage,
                            data.message ||
                                'Order could not be placed.',
                            'error'
                        );

                        return;
                    }


                    // Clear cart
                    // after successful order

                    cart = [];

                    saveCart();

                    renderCart();


                    const total =
                        Number(
                            data.order
                                ?.total_amount ||
                            0
                        ).toFixed(2);


                    setMessage(
                        orderMessage,

                        `${data.message} Total: $${total}`,

                        'success'
                    );

                }
                catch (error) {

                    console.error(
                        'Order error:',
                        error
                    );


                    setMessage(
                        orderMessage,
                        'Unable to place order right now.',
                        'error'
                    );
                }
            }
        );
    }


    // --------------------------------------------------
    // INITIAL PAGE LOAD
    // --------------------------------------------------

    renderCart();

    loadProducts();

}