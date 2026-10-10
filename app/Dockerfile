# Dockerfile for the hosted Shiny for Python MVP app.
#
# This is a local-first container: it builds the C++ core, installs the
# Python bindings, and serves the app on port 8000. It has not been pushed
# to or validated against a specific cloud host; see
# docs/architecture/hosted_mvp_design.md for what remains before a public
# deployment.
#
# Build and run from the repository root:
#   docker build -f app/Dockerfile -t socialchoicelab-mvp .
#   docker run --rm -p 8000:8000 socialchoicelab-mvp

FROM debian:bookworm-slim AS build

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential cmake git \
        libcgal-dev libgmp-dev libmpfr-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY CMakeLists.txt ./
COPY include/ include/
COPY src/ src/

RUN cmake -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build --target scs_api -j"$(nproc)"

FROM python:3.11-slim

RUN apt-get update && apt-get install -y --no-install-recommends \
        libcgal-dev libgmp10 libmpfr6 \
    && rm -rf /var/lib/apt/lists/*

# The C API shared library built above.
COPY --from=build /src/build/libscs_api.* /opt/socialchoicelab/lib/
ENV SCS_LIB_PATH=/opt/socialchoicelab/lib
ENV LD_LIBRARY_PATH=/opt/socialchoicelab/lib

WORKDIR /app
COPY python/ /opt/socialchoicelab/python/
RUN pip install --no-cache-dir /opt/socialchoicelab/python/

COPY app/requirements.txt ./
RUN pip install --no-cache-dir -r requirements.txt

COPY app/app.py ./

EXPOSE 8000
CMD ["python3", "-m", "shiny", "run", "--host", "0.0.0.0", "--port", "8000", "app.py"]
