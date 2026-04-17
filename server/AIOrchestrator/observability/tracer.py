"""
OpenTelemetry 追踪初始化
"""
from opentelemetry import trace
from opentelemetry.sdk.trace import TracerProvider
from opentelemetry.sdk.trace.export import BatchSpanProcessor
from opentelemetry.sdk.resources import Resource, SERVICE_NAME

from config import settings


def init_tracing():
    """初始化 OpenTelemetry 追踪"""
    if not settings.observability.enabled:
        return

    resource = Resource.create({
        SERVICE_NAME: settings.observability.otel.service_name,
    })

    provider = TracerProvider(resource=resource)

    try:
        from opentelemetry.exporter.otlp.proto.grpc.trace_exporter import OTLPSpanExporter
        exporter = OTLPSpanExporter(
            endpoint=settings.observability.otel.endpoint,
            insecure=True,
        )
        provider.add_span_processor(BatchSpanProcessor(exporter))
    except Exception as e:
        import structlog
        structlog.get_logger().warning("otel.exporter_failed", error=str(e))

    trace.set_tracer_provider(provider)


def get_tracer(name: str = "ai-orchestrator"):
    return trace.get_tracer(name)
