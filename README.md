# CUTCRAFT — Where Creativity Meets Opportunity

A responsive creative freelancer marketplace built with a **compiled C11 + SQLite3 backend** and a **Liquid Glass React/TypeScript frontend** styled in the CUTCRAFT logo theme.

## Architecture

- **C Backend (`backend/`)**: Pure C11 multi-threaded POSIX HTTP/1.1 server (`server.c`, `main.c`) with modular business logic:
  - `authentication.c` / `authentication.h`: Pure C SHA-256 + 2048-round stretched password hashing, `/dev/urandom` salts & session tokens.
  - `user.c` / `user.h`: Account profile management.
  - `freelancer.c` / `freelancer.h`: Creator profiles, photo/video portfolio management, and reviews.
  - `search.c` / `search.h`: Parameterized SQLite search, category, price range, rating, and sorting filters.
  - `hiring.c` / `hiring.h`: Client project requests and freelancer accept/reject workflow.
  - `projects.c` / `projects.h`: Project progress tracking, verified client reviews, and client-freelancer chat messaging.
  - `commission.c` / `commission.h`: Configurable platform commission calculations (default 10%) and simulated escrow transactions.
  - `database.c` / `database.h`: SQLite3 relational schema (`users`, `sessions`, `freelancer_profiles`, `portfolio_items`, `project_requests`, `projects`, `messages`, `reviews`, `transactions`, `app_config`).
- **Gateway (`server.ts`)**: Serves Vite/dist on port `3000` and proxies all `/api/*` and `/uploads/*` requests directly to the compiled C server (`backend/cutcraft_server` on port `8080`).
- **Original Media Preservation**: Uploaded images and videos are stored uncompressed in `uploads/`, with schema hooks (`r2_object_key`, `mux_asset_id`, `mux_playback_id`) prepared for Cloudflare R2 and Mux adaptive streaming.

## Compilation & Running

```bash
# 1. Compile the C backend
make -C backend

# 2. Install frontend dependencies
npm install

# 3. Start full-stack app (launches C backend + Vite gateway on port 3000)
npm run dev
```

## Production Build & Hosting

```bash
make -C backend
npm run build
NODE_ENV=production npm start
```
