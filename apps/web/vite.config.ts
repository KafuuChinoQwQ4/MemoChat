import { defineConfig } from "vite";
import react from "@vitejs/plugin-react-swc";
import path from "path";

const contentSecurityPolicy = [
  "default-src 'self'",
  "script-src 'self' 'unsafe-inline'",
  "style-src 'self' 'unsafe-inline'",
  "connect-src 'self' ws: wss: http: https:",
  "img-src 'self' data: blob: https:",
  "font-src 'self' data:",
  "worker-src 'self' blob:",
  "frame-ancestors 'none'",
].join("; ");

const securityHeaders = {
  "Content-Security-Policy": contentSecurityPolicy,
  "X-Content-Type-Options": "nosniff",
  "X-Frame-Options": "DENY",
  "Referrer-Policy": "strict-origin-when-cross-origin",
  "Permissions-Policy": "camera=(), microphone=(), geolocation=()",
};

// https://vitejs.dev/config/
export default defineConfig({
  plugins: [react()],
  resolve: {
    alias: {
      "@/app": path.resolve(__dirname, "src/app"),
      "@/core": path.resolve(__dirname, "src/core"),
      "@/shared": path.resolve(__dirname, "src/shared"),
      "@/features": path.resolve(__dirname, "src/features"),
      "@/routes": path.resolve(__dirname, "src/routes"),
      "@/theme": path.resolve(__dirname, "src/theme"),
      "@/test": path.resolve(__dirname, "src/test"),
    },
  },
  server: {
    headers: securityHeaders,
    proxy: {
      // Proxy HTTP API calls to Envoy gateway, avoiding CORS
      "/user_login": { target: "https://localhost:8443", secure: false, changeOrigin: true },
      "/user_register": { target: "https://localhost:8443", secure: false, changeOrigin: true },
      "/get_varifycode": { target: "https://localhost:8443", secure: false, changeOrigin: true },
      "/reset_pwd": { target: "https://localhost:8443", secure: false, changeOrigin: true },
      "/get_user_info": { target: "https://localhost:8443", secure: false, changeOrigin: true },
      "/user_update_profile": { target: "https://localhost:8443", secure: false, changeOrigin: true },
      "/api": { target: "https://localhost:8443", secure: false, changeOrigin: true },
      "/ai": { target: "https://localhost:8443", secure: false, changeOrigin: true },
      "/media": { target: "https://localhost:8443", secure: false, changeOrigin: true },
      // WebSocket chat ingress
      "/ws": {
        target: "ws://localhost:8444",
        ws: true,
        secure: false,
        changeOrigin: true,
      },
    },
  },
  preview: {
    headers: securityHeaders,
  },
  build: {
    target: "es2020",
    rollupOptions: {
      output: {
        // Split vendor chunks for better caching
        manualChunks: {
          react: ["react", "react-dom"],
          router: ["react-router-dom"],
          query: ["@tanstack/react-query"],
          zustand: ["zustand"],
          dexie: ["dexie"],
        },
      },
    },
  },
});
