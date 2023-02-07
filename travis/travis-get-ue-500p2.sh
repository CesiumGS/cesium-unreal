if [[ $TRAVIS_OS_NAME == "windows" ]]
then
    # Enable compression because disk space is limited on Travis.
    mkdir "C:\Epic"
    mkdir "C:\Epic\UE_5.0"
    compact //c "//s:C:\Epic\UE_5.0"
    AWS_ACCESS_KEY_ID=${GOOGLE_ACCESS_KEY_ID} AWS_SECRET_ACCESS_KEY=${GOOGLE_SECRET_ACCESS_KEY} aws s3 --endpoint-url https://storage.googleapis.com cp s3://cesium-unreal-engine/5.0.0-preview2/UE_5.0p2.zip .
    7z x UE_5.0p2.zip "-oC:\Epic"
    rm UE_5.0p2.zip
# elif [[ $TRAVIS_OS_NAME == "osx" ]]
# then
#     aws s3 cp s3://cesium-unreal-engine/macos-2022-01-27/UE_4272_macOS.zip .

#     unzip -q UE_4272_macOS.zip -d $HOME
#     rm UE_4272_macOS.zip
# elif [[ $TRAVIS_OS_NAME == "linux" ]]
# then
#     AWS_ACCESS_KEY_ID=${GOOGLE_ACCESS_KEY_ID} AWS_SECRET_ACCESS_KEY=${GOOGLE_SECRET_ACCESS_KEY} aws s3 --endpoint-url https://storage.googleapis.com cp s3://cesium-unreal-engine/linux-2021-04-23/UnrealEngine-4.26.2.tar.gz .
#     mkdir $HOME/UE_4.26
#     tar --checkpoint=100000 -xzf UnrealEngine-4.26.2.tar.gz -C $HOME/UE_4.26
#     rm UnrealEngine-4.26.2.tar.gz
fi
