FROM nginx:1.26.2-alpine

# Copy custom nginx configuration
COPY conf/nginx.conf /etc/nginx/nginx.conf

# Expose ports
# HTTP (GateServer LB)
EXPOSE 80
# TCP Stream (ChatServer LB)
EXPOSE 8090 8091 8092 8093

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD wget --no-verbose --tries=1 --spider http://localhost/health || exit 1

USER nginx

CMD ["nginx", "-g", "daemon off;"]
