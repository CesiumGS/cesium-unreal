if [ ! -d "/c/Program Files/Epic Games/UE_4.26" ]
then
    aws s3 cp s3://cesium-unreal-engine/2021-03-16/UE_4.26.zip .
    7z x UE_4.26.zip "-oC:\Program Files\Epic Games"
    rm UE_4.26.zip
    aws s3 cp s3://cesium-unreal-engine/2021-03-16/Launcher.zip .
    7z x Launcher.zip "-oC:\Program Files (x86)\Epic Games"
    rm Launcher.zip
fi
