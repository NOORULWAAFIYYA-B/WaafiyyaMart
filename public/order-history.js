// --------------------------------------------------
// WAafiyyaMart - ORDER HISTORY
// --------------------------------------------------


function getStoredUser() {

    try {

        const raw =
            localStorage.getItem('waafiyyaUser');

        return raw
            ? JSON.parse(raw)
            : null;

    } catch (error) {

        return null;
    }
}


// --------------------------------------------------
// ELEMENTS
// --------------------------------------------------

const ordersContainer =
    document.getElementById('ordersContainer');

const refreshBtn =
    document.getElementById('refreshBtn');


// --------------------------------------------------
// FORMAT MONEY
// --------------------------------------------------

function formatMoney(value) {

    return `$${Number(value || 0).toFixed(2)}`;
}


// --------------------------------------------------
// FORMAT DATE
// --------------------------------------------------

function formatDate(value) {

    if (!value) {
        return 'Date unavailable';
    }


    const date =
        new Date(value);


    if (Number.isNaN(date.getTime())) {
        return value;
    }


    return date.toLocaleString();
}


// --------------------------------------------------
// LOAD ORDERS
// --------------------------------------------------

async function loadOrders() {

    const user =
        getStoredUser();


    // --------------------------------------------------
    // CHECK LOGIN
    // --------------------------------------------------

    if (!user || !user.id) {

        window.location.href =
            'login.html';

        return;
    }


    // --------------------------------------------------
    // LOADING MESSAGE
    // --------------------------------------------------

    ordersContainer.innerHTML = `
        <div class="loading">
            Loading your orders...
        </div>
    `;


    try {

        const response =
            await fetch(
                `/api/orders?user_id=${encodeURIComponent(user.id)}`
            );


        const data =
            await response.json();


        if (!response.ok) {

            throw new Error(
                data.message ||
                'Unable to load orders.'
            );
        }


        const orders =
            Array.isArray(data.orders)
                ? data.orders
                : [];


        // --------------------------------------------------
        // NO ORDERS
        // --------------------------------------------------

        if (!orders.length) {

            ordersContainer.innerHTML = `
                <div class="empty-state">

                    <h2>
                        No orders yet
                    </h2>

                    <p>
                        Your completed orders will appear here.
                    </p>

                    <a
                        class="button primary"
                        href="products.html"
                    >
                        Start Shopping
                    </a>

                </div>
            `;

            return;
        }


        // --------------------------------------------------
        // DISPLAY ORDERS
        // --------------------------------------------------

        ordersContainer.innerHTML =
            orders.map((order) => {

                const items =
                    Array.isArray(order.items)
                        ? order.items
                        : [];


                const itemsHtml =
                    items.map((item) => {

                        const itemTotal =
                            Number(item.price_at_order || 0) *
                            Number(item.quantity || 0);


                        return `
                            <div class="order-item">

                                <div>

                                    <div class="item-name">
                                        ${item.name}
                                    </div>

                                    <div class="item-details">
                                        Qty:
                                        ${item.quantity}
                                        ×
                                        ${formatMoney(item.price_at_order)}
                                    </div>

                                </div>


                                <div class="item-price">
                                    ${formatMoney(itemTotal)}
                                </div>

                            </div>
                        `;

                    }).join('');


                return `
                    <article class="order-card">

                        <div class="order-header">

                            <div>

                                <h2 class="order-number">
                                    Order #${order.id}
                                </h2>

                                <div class="order-date">
                                    ${formatDate(order.created_at)}
                                </div>

                            </div>


                            <span class="status">
                                ${order.status || 'pending'}
                            </span>

                        </div>


                        <div class="order-items">

                            ${itemsHtml}

                        </div>


                        <div class="order-footer">

                            <div class="order-total">
                                Total:
                                ${formatMoney(order.total_amount)}
                            </div>

                        </div>

                    </article>
                `;

            }).join('');


    } catch (error) {

        console.error(
            'Order history error:',
            error
        );


        ordersContainer.innerHTML = `
            <div class="error">
                ${error.message || 'Unable to load order history.'}
            </div>
        `;
    }
}


// --------------------------------------------------
// REFRESH BUTTON
// --------------------------------------------------

if (refreshBtn) {

    refreshBtn.addEventListener(
        'click',
        loadOrders
    );
}


// --------------------------------------------------
// INITIAL LOAD
// --------------------------------------------------

loadOrders();