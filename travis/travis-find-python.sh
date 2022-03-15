# On some platforms, it's python3, on others it's just python
export CESIUM_PYTHON=python3
if (! command -v python3) || (! python3 --version)
then
  export CESIUM_PYTHON=python
fi

export CESIUM_PIP=pip3
if (! command -v pip3) || (! pip3 --version)
then
  export CESIUM_PIP=pip
fi

echo Python executable: $CESIUM_PYTHON
$CESIUM_PYTHON --version
$CESIUM_PIP --version
