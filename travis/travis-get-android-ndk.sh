if [[ $TRAVIS_OS_NAME == "windows" ]]
then
    echo "Setting up Android NDK"
    if [[ ! -d "$HOME/android-ndk-r21e/platforms" ]]
    then
        echo "Downloading Android NDK"
        wget -q https://dl.google.com/android/repository/android-ndk-r21b-windows-x86_64.zip
        7z x android-ndk-r21e-windows-x86_64.zip "-o$HOME/"
        rm android-ndk-r21e-windows-x86_64.zip
    else
        echo "Using Android NDK from Travis Cache"
    fi
fi
