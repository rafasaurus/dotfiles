#!/bin/bash

# docker pull intelanalytics/ipex-llm-inference-cpp-xpu:latest
# https://dev.to/itlackey/run-ollama-on-intel-arc-gpu-ipex-4e4k
docker run -d --restart=always \
    --net=bridge \
    --device=/dev/dri \
    -p 11434:11434 \
    -v ~/.ollama/models:/root/.ollama/models \
    -e PATH=/llm/ollama:$PATH \
    -e OLLAMA_HOST=0.0.0.0 \
    -e no_proxy=localhost,127.0.0.1 \
    -e ZES_ENABLE_SYSMAN=1 \
    -e OLLAMA_INTEL_GPU=true \
    -e OLLAMA_NUM_GPU=999 \
    -e SYCL_CACHE_PERSISTENT=1 \
    -e ONEAPI_DEVICE_SELECTOR=level_zero:0 \
    -e DEVICE=Arc \
    --shm-size="16g" \
    --memory="32G" \
    --name=ipex-llm \
    intelanalytics/ipex-llm-inference-cpp-xpu:latest \
    bash -c "cd /llm/scripts/ && source ipex-llm-init --gpu --device Arc && bash start-ollama.sh && tail -f /llm/ollama/ollama.log"

