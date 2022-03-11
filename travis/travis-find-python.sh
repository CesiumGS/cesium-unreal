# On some platforms, it's python3, on others it's just python
export CESIUM_PYTHON=python3
if ! command -v python3
then
  export CESIUM_PYTHON=python
fi

export CESIUM_PIP=pip3
if ! command -v pip3
then
  export CESIUM_PIP=pip
fi

$CESIUM_PYTHON --version
$CESIUM_PIP --version
