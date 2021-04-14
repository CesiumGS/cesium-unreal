if [[ $TRAVIS_OS_NAME == "windows" && ! -d "/c/Program Files/Epic Games/UE_4.26" ]]
then
    AWS_ACCESS_KEY_ID=${GOOGLE_ACCESS_KEY_ID} AWS_SECRET_ACCESS_KEY=${GOOGLE_SECRET_ACCESS_KEY} aws s3 --endpoint-url https://storage.googleapis.com cp s3://cesium-unreal-engine/2021-03-16/UE_4.26.zip .
    7z x UE_4.26.zip "-oC:\Program Files\Epic Games"
    rm UE_4.26.zip
    AWS_ACCESS_KEY_ID=${GOOGLE_ACCESS_KEY_ID} AWS_SECRET_ACCESS_KEY=${GOOGLE_SECRET_ACCESS_KEY} aws s3 --endpoint-url https://storage.googleapis.com cp s3://cesium-unreal-engine/2021-03-16/Launcher.zip .
    7z x Launcher.zip "-oC:\Program Files (x86)\Epic Games"
    rm Launcher.zip
elif [[ $TRAVIS_OS_NAME == "osx" && ! -d "$HOME/UE_4.26" ]]
then
    AWS_ACCESS_KEY_ID=${GOOGLE_ACCESS_KEY_ID} AWS_SECRET_ACCESS_KEY=${GOOGLE_SECRET_ACCESS_KEY} aws s3 --endpoint-url https://storage.googleapis.com cp s3://cesium-unreal-engine/macos-2021-04-13/UE_4.26.tar.gz .
    mkdir $HOME/UE_4.26
    tar xfz UE_4.26.tar.gz -C $HOME/UE_4.26
    rm UE_4.26.tar.gz
fi
