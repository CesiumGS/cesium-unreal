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
  choco install 7zip.portable
  choco install python --version 3.9.2
  choco install wget
  choco install ninja
  python -m pip install --upgrade pip
  pip3 install --upgrade pip
  pip3 install awscli
  pip3 install httpie
  pip3 install conan
fi

# Install our custom version of conan-ue4cli
pushd ..
git clone https://github.com/kring/conan-ue4cli.git
cd conan-ue4cli
python ./setup.py install
popd
