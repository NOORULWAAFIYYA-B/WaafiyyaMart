SELECT
    o.id AS order_id,
    u.full_name,
    o.total_amount,
    o.status
FROM orders o
JOIN users u ON u.id = o.user_id
ORDER BY o.id DESC
LIMIT 5;