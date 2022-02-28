source travis/travis-find-python.sh

if [[ $TRAVIS_OS_NAME == "windows" ]]
then
  choco install 7zip.portable
  choco install python --version 3.9.2
  choco install wget
  choco install ninja
  python -m pip install --upgrade pip
  pip3 install --upgrade pip
  pip3 install awscli
  pip3 install httpie
  pip3 install conan
elif [[ $TRAVIS_OS_NAME == "osx" ]]
then
  python -m pip install --upgrade pip
  pip3 install --upgrade pip
  pip3 install awscli
  pip3 install httpie
  pip3 install conan
elif [[ $TRAVIS_OS_NAME == "linux" ]]
then
  # As of 2021-08-23, the server listed in the rabbitmq PPA has an expired certificate
  # and breaks our ability to update. We don't need it, so remove it.
  sudo rm /etc/apt/sources.list.d/rabbitmq.list
  sudo apt-get update
  sudo apt-get install unzip zip awscli httpie
  pip3 install conan
fi

# Install our custom version of conan-ue4cli
pushd ..
git clone -b cesium https://github.com/kring/conan-ue4cli.git
cd conan-ue4cli
$CESIUM_PYTHON ./setup.py install
popd
