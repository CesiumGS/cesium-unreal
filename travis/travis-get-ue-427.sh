if [[ $TRAVIS_OS_NAME == "windows" ]]
then
    # Enable compression because disk space is limited on Travis.
    mkdir "C:\Epic"
    mkdir "C:\Epic\UE_4.27"
    compact //c "//s:C:\Epic\UE_4.27"
    AWS_ACCESS_KEY_ID=${GOOGLE_ACCESS_KEY_ID} AWS_SECRET_ACCESS_KEY=${GOOGLE_SECRET_ACCESS_KEY} aws s3 --endpoint-url https://storage.googleapis.com cp s3://cesium-unreal-engine/4.27.0/Windows/UE_4.27-minimal.zip .
    7z x UE_4.27-minimal.zip "-oC:\Epic"
    rm UE_4.27-minimal.zip
# elif [[ $TRAVIS_OS_NAME == "osx" ]]
# then
#     aws s3 cp s3://cesium-unreal-engine/macos-2021-04-13/UE_4.26.tar.gz .
#     mkdir $HOME/UE_4.26
#     tar xfz UE_4.26.tar.gz -C $HOME/UE_4.26
#     rm UE_4.26.tar.gz
# elif [[ $TRAVIS_OS_NAME == "linux" ]]
# then
#     AWS_ACCESS_KEY_ID=${GOOGLE_ACCESS_KEY_ID} AWS_SECRET_ACCESS_KEY=${GOOGLE_SECRET_ACCESS_KEY} aws s3 --endpoint-url https://storage.googleapis.com cp s3://cesium-unreal-engine/linux-2021-04-23/UnrealEngine-4.26.2.tar.gz .
#     mkdir $HOME/UE_4.26
#     tar --checkpoint=100000 -xzf UnrealEngine-4.26.2.tar.gz -C $HOME/UE_4.26
#     rm UnrealEngine-4.26.2.tar.gz
fi
