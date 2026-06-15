FROM envoyproxy/envoy:v1.38.0

EXPOSE 80
EXPOSE 8090 8091

HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD envoy --mode validate -c /etc/envoy/envoy.yaml || exit 1

CMD ["envoy", "-c", "/etc/envoy/envoy.yaml"]
