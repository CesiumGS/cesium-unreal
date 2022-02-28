# On some platforms, it's python3, on others it's just python
export CESIUM_PYTHON=python3
if ! command -v python3
then
  export CESIUM_PYTHON=python
fi
