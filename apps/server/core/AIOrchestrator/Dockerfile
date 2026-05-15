FROM python:3.12-slim

WORKDIR /app

ENV PYTHONDONTWRITEBYTECODE=1
ENV PYTHONUNBUFFERED=1
ENV PIP_DISABLE_PIP_VERSION_CHECK=1
ENV PIP_DEFAULT_TIMEOUT=120

ARG PIP_INDEX_URL=https://pypi.tuna.tsinghua.edu.cn/simple
ARG PIP_EXTRA_INDEX_URL=
ARG PIP_TRUSTED_HOST=pypi.tuna.tsinghua.edu.cn

COPY requirements.txt /app/requirements.txt
RUN --mount=type=cache,target=/root/.cache/pip \
    python -m pip install \
      --index-url "${PIP_INDEX_URL}" \
      ${PIP_EXTRA_INDEX_URL:+--extra-index-url "${PIP_EXTRA_INDEX_URL}"} \
      --trusted-host "${PIP_TRUSTED_HOST}" \
      -r /app/requirements.txt

COPY . /app

EXPOSE 8096

CMD ["python", "main.py"]
