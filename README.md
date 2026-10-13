# WaafiyyaMart

WaafiyyaMart is a C++ web-based shopping application built with Drogon and PostgreSQL.

## Features

- User registration and login
- Seller-owned product management with JPEG, PNG, and WebP image uploads
- Product listing with image previews, search, and category filtering
- Shopping cart
- Order placement
- Stock management
- Insufficient-stock validation
- Order history
- PostgreSQL database integration

## Technologies

- C++
- Drogon
- PostgreSQL
- HTML
- CSS
- JavaScript
- CMake
- vcpkg

## Database

The application uses PostgreSQL for storing users, products, orders, and order items.

The PostgreSQL connection settings are stored locally in config/config.json and are intentionally excluded from Git.
The Render entrypoint builds this file from the server-side `DATABASE_URL` environment variable.

## Product image storage on Render

The repository did not contain a configured Render disk or external object-storage integration. Product files are stored on disk by the C++ service, so **do not enable uploads on Render until a persistent disk is mounted**. Render's ordinary container filesystem is ephemeral.

1. In the Render dashboard, open the WaafiyyaMart web service and add a persistent disk. Set its mount path to `/var/data` and choose a disk size suitable for the catalog.
2. Add the service environment variable `PRODUCT_IMAGE_DIR=/var/data/product-images`.
3. Save the disk/environment settings and deploy the service.
4. Confirm that uploads are saved and served at `/uploads/<server-generated-name>`. The server rejects uploads in a Render environment when `PRODUCT_IMAGE_DIR` is missing instead of silently using ephemeral container storage.

For local development, the image directory defaults to `./uploads` when `PRODUCT_IMAGE_DIR` is unset. Set it to a durable local directory if desired.

Images are limited to 5 MiB and their JPEG, PNG, or WebP file signatures are checked on the server. The browser-supplied filename and content type are not used. Uploaded image URLs are stored in `products.image_url`.

## Seller sessions and existing products

Successful logins receive a random bearer token. Seller product endpoints require `Authorization: Bearer <token>` and verify the seller role and product ownership against PostgreSQL. Tokens are stored as SHA-256 hashes in `user_sessions` and expire after 30 days; logout revokes the token.

After Drogon starts, the service asynchronously adds the nullable `products.seller_id` column and creates the session table/indexes when absent. Requests receive HTTP 503 until the migrations finish; a database initialization failure is logged without credentials and shuts down the process cleanly. The listener is configured once by the deployment config generator, using Render's validated `PORT` on `0.0.0.0`. `db/schema.sql` includes the same schema for new installations. Existing products are deliberately left unassigned rather than being transferred to an arbitrary seller. To let the correct seller manage particular legacy products, first verify their IDs and the seller's user ID, then assign only those rows using a reviewed PostgreSQL statement, for example:

```sql
UPDATE products
SET seller_id = <verified_seller_user_id>
WHERE id IN (<verified_product_ids>)
  AND seller_id IS NULL;
```

Seller accounts must have `users.role = 'seller'`. Public registration continues to create customer accounts; seller-role provisioning remains an administrative database operation.

## Product management API

- `GET /api/admin/products` lists only products belonging to the authenticated seller.
- `POST /api/admin/products` accepts `multipart/form-data` fields `name`, `description`, `price`, `stock`, `category`, and required file field `image`.
- `PUT /api/admin/products/{id}` accepts the same fields; `image` is optional, and omitting it retains the existing image.
- `DELETE /api/admin/products/{id}` deletes only a product owned by the authenticated seller.
- `GET /api/products` remains the public catalog endpoint.

The seller dashboard obtains its bearer token from the login response. No image-storage or database credentials belong in frontend code.

## Build

The project uses CMake and vcpkg for dependency management.
