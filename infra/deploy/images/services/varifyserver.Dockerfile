FROM node:20-alpine

WORKDIR /app

COPY apps/server/core/VarifyServer/package*.json ./
RUN npm ci --omit=dev

COPY apps/server/core/VarifyServer ./

EXPOSE 50051 8081

CMD ["node", "server.js"]
