FROM python:3.12-slim

ENV PYTHONDONTWRITEBYTECODE=1
ENV PYTHONUNBUFFERED=1

WORKDIR /app

COPY Memo_ops/requirements.txt /app/requirements.txt
RUN pip install --no-cache-dir -r /app/requirements.txt

COPY Memo_ops /app/Memo_ops

ENV PYTHONPATH=/app

CMD ["python", "-m", "Memo_ops.server.ops_server.main", "--config", "/app/config/opsserver.yaml"]
