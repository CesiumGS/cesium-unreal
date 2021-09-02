echo "Downloading Linux toolchain"
AWS_ACCESS_KEY_ID=${GOOGLE_ACCESS_KEY_ID} AWS_SECRET_ACCESS_KEY=${GOOGLE_SECRET_ACCESS_KEY} aws s3 --endpoint-url https://storage.googleapis.com cp s3://cesium-unreal-engine/4.27.0/Windows/v19_clang-11.0.1-centos7.exe .
echo "Finished download"
chmod a+x v19_clang-11.0.1-centos7.exe
./v19_clang-11.0.1-centos7.exe "//S"
