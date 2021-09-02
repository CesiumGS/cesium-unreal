echo "Downloading Linux toolchain"
wget https://cdn.unrealengine.com/CrossToolchain_Linux/v19_clang-11.0.1-centos7.exe
echo "Finished download"
chmod a+x v19_clang-11.0.1-centos7.exe
./v19_clang-11.0.1-centos7.exe "//S"
