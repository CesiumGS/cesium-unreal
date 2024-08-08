# This test measures network performance when downloading Google P3DT tiles
#
# To use
# 1) Start any app that uses Google P3DT, like Cesium for Unreal sample level, or automation test
# 2) Find the session key that was started. 
#    - For Unreal, find you sqlite cache at C:\Users\<your username>\AppData\Local\UnrealEngine\5.4
#    - Use a sql browser to open cesium-request-cache.sqlite (I recommend DB Browser for SQLite)


import sys
import time
import os
import subprocess
import TestUtils
import TestKey

if __name__ == "__main__":
    sessionString = TestKey.SessionKey

    urlArray = TestUtils.buildUrlArray(sessionString)

    command = "curl.exe -Z ";

    for key, value in TestUtils.UnrealStyleHeaders.items():
        command += "--header \"" + key + ": " + value + "\" "

    command += "\"" + urlArray[0] + "\" "
    command += "\"" + urlArray[1] + "\" "
    command += "\"" + urlArray[2] + "\" "
    command += "\"" + urlArray[3] + "\""

    print ("\nExecuting....\n\n" + command + "\n\n")

    startMark = time.time()

    r = subprocess.run(command, capture_output=True)

    bytesDownloaded = len(r.stdout)

    secsElapsed = time.time()-startMark

    TestUtils.printSummary(secsElapsed, bytesDownloaded)
