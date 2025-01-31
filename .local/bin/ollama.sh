#!/bin/sh

# https://github.com/intel-analytics/ipex-llm/blob/main/docs/mddocs/Quickstart/ollama_quickstart.md

conda activate llm-cpp
export OLLAMA_NUM_GPU=999
export no_proxy=localhost,127.0.0.1
export ZES_ENABLE_SYSMAN=1

source $HOME/intel/oneapi/setvars.sh
export SYCL_CACHE_PERSISTENT=1
# [optional] under most circumstances, the following environment variable may improve performance, but sometimes this may also cause performance degradation
# export SYCL_PI_LEVEL_ZERO_USE_IMMEDIATE_COMMANDLISTS=1
# [optional] if you want to run on single GPU, use below command to limit GPU may improve performance
export ONEAPI_DEVICE_SELECTOR=level_zero:0
init-ollama
# for NPU
export IPEX_LLM_NPU_ARL=1
pushd /home/rafael/github/intel-ipex/ && ./ollama serve
