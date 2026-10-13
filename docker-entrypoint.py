import json
import os
import sys
from urllib.parse import urlparse, unquote, parse_qs

database_url = os.environ.get("DATABASE_URL")
port = os.environ.get("PORT", "10000")

if not database_url:
    sys.exit("ERROR: DATABASE_URL environment variable is missing.")

try:
    parsed = urlparse(database_url)

    if parsed.scheme not in ("postgres", "postgresql"):
        raise ValueError("DATABASE_URL must be a PostgreSQL URL.")

    if not parsed.hostname or not parsed.path.lstrip("/"):
        raise ValueError("DATABASE_URL is missing its host or database name.")

    query = parse_qs(parsed.query)
    port_number = int(port)
    if not 1 <= port_number <= 65535:
        raise ValueError("PORT must be between 1 and 65535.")

    db_client = {
        "name": "default",
        "rdbms": "postgresql",
        "host": parsed.hostname,
        "port": parsed.port or 5432,
        "dbname": unquote(parsed.path.lstrip("/")),
        "user": unquote(parsed.username or ""),
        "passwd": unquote(parsed.password or ""),
        "connection_number": 5
    }

    if "sslmode" in query:
        db_client["connect_options"] = {
            "sslmode": query["sslmode"][0]
        }

    config = {
        "listeners": [
            {
                "address": "0.0.0.0",
                "port": port_number
            }
        ],
        "app": {
            "document_root": "./public"
        },
        "db_clients": [db_client]
    }

    os.makedirs("config", exist_ok=True)

    with open("config/config.json", "w", encoding="utf-8") as file:
        json.dump(config, file, indent=2)
    print(
        "Generated Drogon config for PostgreSQL and listener "
        "0.0.0.0:" + str(port_number),
        flush=True
    )

except (ValueError, TypeError) as error:
    sys.exit("ERROR: Invalid deployment configuration: " + str(error))

os.execv("/app/build/WaafiyyaMart", ["/app/build/WaafiyyaMart"])
