if [ ! -d "/c/Program\ Files/Epic\ Games/UE_4.26" ]
then
    choco install 7zip.portable
    choco install python --version 3.8.0
    python -m pip install --upgrade pip
    pip3 install --upgrade pip
    pip3 install awscli
    aws s3 cp s3://cesium-unreal-engine/2021-03-16/UE_4.26.zip .
    7z x UE_4.26.zip "-oC:\Program Files\Epic Games"
    rm UE_4.26.zip
    aws s3 cp s3://cesium-unreal-engine/2021-03-16/Launcher.zip .
    7z x Launcher.zip "-oC:\Program Files (x86)\Epic Games"
    rm Launcher.zip
fi
