if [[ $TRAVIS_OS_NAME == "windows" && ! -d "$HOME/android-ndk-r21e" ]]
then
    wget -q https://dl.google.com/android/repository/android-ndk-r21e-windows-x86_64.zip
    7z x android-ndk-r21e-windows-x86_64.zip "-o$HOME/"
    rm android-ndk-r21e-windows-x86_64.zip
fi

