# MemoChat VarifyServer Dockerfile
# Build: docker build -t memochat/varifyserver:latest -f server/VarifyServer/Dockerfile .

FROM node:20-alpine AS builder

WORKDIR /app

COPY server/VarifyServer/package*.json ./
RUN npm ci --only=production

COPY server/VarifyServer/ ./

FROM node:20-alpine

WORKDIR /app

RUN apk add --no-cache tini

COPY --from=builder /app/node_modules ./node_modules
COPY --from=builder /app/ ./

ENV NODE_ENV=production \
    MEMOCHAT_LOG_LEVEL=info \
    MEMOCHAT_SERVICE_NAME=VarifyServer

EXPOSE 50051 8081

HEALTHCHECK --interval=30s --timeout=10s --retries=3 \
    CMD node -e "require('http').get('http://localhost:8081/health', (r) => process.exit(r.statusCode === 200 ? 0 : 1)).on('error', () => process.exit(1))"

USER node

ENTRYPOINT ["/sbin/tini", "--", "node", "src/server.js"]
