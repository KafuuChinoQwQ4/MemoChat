FROM node:20-alpine

WORKDIR /app

COPY server/VarifyServer/package*.json ./
RUN npm ci --omit=dev

COPY server/VarifyServer ./

EXPOSE 50051 8081

CMD ["node", "server.js"]
