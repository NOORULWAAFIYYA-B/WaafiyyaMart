import hashlib
import json
import sqlite3
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import urlparse

BASE_DIR = Path(__file__).resolve().parent
PUBLIC_DIR = BASE_DIR / 'public'
DB_DIR = BASE_DIR / 'db'
DB_PATH = DB_DIR / 'waafiyyamart.db'
SCHEMA_PATH = DB_DIR / 'schema.sql'


def hash_password(password: str) -> str:
    return hashlib.sha256(password.encode('utf-8')).hexdigest()


def open_db() -> sqlite3.Connection:
    DB_DIR.mkdir(exist_ok=True)
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    conn.execute('PRAGMA foreign_keys = ON')
    return conn


def seed_products(conn):
    count = conn.execute('SELECT COUNT(*) FROM products').fetchone()[0]
    if count > 0:
        return

    default_products = [
        ('Classic T-Shirt', 'Soft cotton everyday tee', 29.99, 'https://images.unsplash.com/photo-1521572267360-ee0c2909d518?auto=format&fit=crop&w=600&q=80', 25, 'Fashion'),
        ('Wireless Headphones', 'Noise cancelling over-ear sound', 149.99, 'https://images.unsplash.com/photo-1546435770-a3e426bf472b?auto=format&fit=crop&w=600&q=80', 15, 'Electronics'),
        ('Coffee Grinder', 'Fresh grounds for every morning', 59.50, 'https://images.unsplash.com/photo-1447933601403-0c6688de566e?auto=format&fit=crop&w=600&q=80', 18, 'Home'),
        ('Running Shoes', 'Lightweight daily training pair', 89.99, 'https://images.unsplash.com/photo-1542291026-7eec264c27ff?auto=format&fit=crop&w=600&q=80', 22, 'Sports'),
        ('Smart Watch', 'Track fitness and notifications', 199.00, 'https://images.unsplash.com/photo-1523275335684-37898b6baf30?auto=format&fit=crop&w=600&q=80', 12, 'Electronics'),
    ]

    conn.executemany(
        'INSERT INTO products (name, description, price, image_url, stock, category) VALUES (?, ?, ?, ?, ?, ?)',
        default_products,
    )
    conn.commit()


def init_db() -> None:
    if not SCHEMA_PATH.exists():
        raise FileNotFoundError(f'Missing schema file: {SCHEMA_PATH}')

    conn = open_db()
    try:
        schema_sql = SCHEMA_PATH.read_text(encoding='utf-8')
        conn.executescript(schema_sql)
        seed_products(conn)
        conn.commit()
    finally:
        conn.close()


def register_user(full_name: str, email: str, password: str):
    full_name = (full_name or '').strip()
    email = (email or '').strip().lower()
    password = (password or '').strip()

    if not full_name or not email or not password:
        return {'ok': False, 'message': 'Full name, email, and password are required.'}, 400

    if '@' not in email or '.' not in email:
        return {'ok': False, 'message': 'Please enter a valid email address.'}, 400

    conn = open_db()
    try:
        existing = conn.execute('SELECT 1 FROM users WHERE email = ?', (email,)).fetchone()
        if existing:
            return {'ok': False, 'message': 'An account with this email already exists.'}, 409

        password_hash = hash_password(password)
        conn.execute(
            'INSERT INTO users (full_name, email, password_hash, role) VALUES (?, ?, ?, ?)',
            (full_name, email, password_hash, 'customer'),
        )
        conn.commit()
        return {'ok': True, 'message': 'User registered successfully.'}, 201
    finally:
        conn.close()


def login_user(email: str, password: str):
    email = (email or '').strip().lower()
    password = (password or '').strip()

    if not email or not password:
        return {'ok': False, 'message': 'Email and password are required.'}, 400

    conn = open_db()
    try:
        row = conn.execute(
            'SELECT id, full_name, email, password_hash, role FROM users WHERE email = ?',
            (email,),
        ).fetchone()

        if not row:
            return {'ok': False, 'message': 'Invalid email or password.'}, 401

        stored_hash = row['password_hash']
        if stored_hash != hash_password(password):
            return {'ok': False, 'message': 'Invalid email or password.'}, 401

        return {
            'ok': True,
            'message': 'Login successful.',
            'user': {
                'id': row['id'],
                'full_name': row['full_name'],
                'email': row['email'],
                'role': row['role'],
            },
        }, 200
    finally:
        conn.close()


def get_products():
    conn = open_db()
    try:
        rows = conn.execute(
            'SELECT id, name, description, price, image_url, stock, category FROM products ORDER BY id ASC'
        ).fetchall()
        products = [dict(row) for row in rows]
        return {'ok': True, 'products': products}, 200
    finally:
        conn.close()


def create_order(user_id: int, items):
    if not user_id or not isinstance(items, list) or not items:
        return {'ok': False, 'message': 'Please add at least one item to the cart.'}, 400

    conn = open_db()
    try:
        user_exists = conn.execute('SELECT 1 FROM users WHERE id = ?', (int(user_id),)).fetchone()
        if not user_exists:
            return {'ok': False, 'message': 'User not found.'}, 404

        total_amount = 0.0
        normalized_items = []

        for item in items:
            product_id = item.get('product_id')
            quantity = int(item.get('quantity', 0))
            if not product_id or quantity <= 0:
                return {'ok': False, 'message': 'Each cart item must include a valid product and quantity.'}, 400

            product = conn.execute('SELECT id, name, price, stock FROM products WHERE id = ?', (int(product_id),)).fetchone()
            if not product:
                return {'ok': False, 'message': f'Product {product_id} was not found.'}, 404
            if product['stock'] < quantity:
                return {'ok': False, 'message': f'Not enough stock for {product["name"]}.'}, 400

            unit_price = float(product['price'])
            total_amount += unit_price * quantity
            normalized_items.append({
                'product_id': int(product_id),
                'quantity': quantity,
                'price_at_order': unit_price,
            })

        cursor = conn.execute(
            'INSERT INTO orders (user_id, total_amount, status) VALUES (?, ?, ?)',
            (int(user_id), round(total_amount, 2), 'pending'),
        )
        order_id = cursor.lastrowid

        for item in normalized_items:
            conn.execute(
                'INSERT INTO order_items (order_id, product_id, quantity, price_at_order) VALUES (?, ?, ?, ?)',
                (order_id, item['product_id'], item['quantity'], item['price_at_order']),
            )
            conn.execute(
                'UPDATE products SET stock = stock - ? WHERE id = ?',
                (item['quantity'], item['product_id']),
            )

        conn.commit()
        return {
            'ok': True,
            'message': 'Order placed successfully.',
            'order': {
                'id': order_id,
                'user_id': int(user_id),
                'total_amount': round(total_amount, 2),
                'status': 'pending',
                'items': normalized_items,
            },
        }, 201
    finally:
        conn.close()


class AppHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        parsed = urlparse(self.path)
        path = parsed.path

        if path == '/api/health':
            self.send_json({'status': 'ok'})
            return

        if path == '/api/users':
            conn = open_db()
            try:
                rows = conn.execute(
                    'SELECT id, full_name, email, role FROM users ORDER BY id DESC'
                ).fetchall()
                users = [dict(row) for row in rows]
                self.send_json({'ok': True, 'users': users})
            finally:
                conn.close()
            return

        if path == '/api/products':
            result, status = get_products()
            self.send_json(result, status=status)
            return

        file_path = PUBLIC_DIR / path.lstrip('/')
        if path in ('/', ''):
            file_path = PUBLIC_DIR / 'login.html'

        if file_path.exists() and file_path.is_file():
            self.serve_file(file_path)
            return

        self.send_error(404, 'Not found')

    def do_POST(self):
        parsed = urlparse(self.path)
        path = parsed.path

        if path == '/api/register':
            content_length = int(self.headers.get('Content-Length', 0))
            body = self.rfile.read(content_length).decode('utf-8')
            payload = json.loads(body or '{}')
            result, status = register_user(payload.get('full_name', ''), payload.get('email', ''), payload.get('password', ''))
            self.send_json(result, status=status)
            return

        if path == '/api/login':
            content_length = int(self.headers.get('Content-Length', 0))
            body = self.rfile.read(content_length).decode('utf-8')
            payload = json.loads(body or '{}')
            result, status = login_user(payload.get('email', ''), payload.get('password', ''))
            self.send_json(result, status=status)
            return

        if path == '/api/orders':
            content_length = int(self.headers.get('Content-Length', 0))
            body = self.rfile.read(content_length).decode('utf-8')
            payload = json.loads(body or '{}')
            result, status = create_order(payload.get('user_id'), payload.get('items', []))
            self.send_json(result, status=status)
            return

        self.send_error(404, 'Endpoint not found')

    def serve_file(self, file_path: Path):
        content = file_path.read_bytes()
        mime_types = {
            '.html': 'text/html; charset=utf-8',
            '.css': 'text/css; charset=utf-8',
            '.js': 'application/javascript; charset=utf-8',
            '.json': 'application/json; charset=utf-8',
            '.png': 'image/png',
            '.jpg': 'image/jpeg',
            '.jpeg': 'image/jpeg',
            '.svg': 'image/svg+xml',
        }
        content_type = mime_types.get(file_path.suffix.lower(), 'application/octet-stream')

        self.send_response(200)
        self.send_header('Content-Type', content_type)
        self.send_header('Content-Length', str(len(content)))
        self.end_headers()
        self.wfile.write(content)

    def send_json(self, payload, status=200):
        body = json.dumps(payload).encode('utf-8')
        self.send_response(status)
        self.send_header('Content-Type', 'application/json; charset=utf-8')
        self.send_header('Content-Length', str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, format, *args):
        return


if __name__ == '__main__':
    init_db()
    server = ThreadingHTTPServer(('0.0.0.0', 8000), AppHandler)
    print('WaafiyyaMart backend running on http://localhost:8000')
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print('\nShutting down server...')
        server.server_close()
