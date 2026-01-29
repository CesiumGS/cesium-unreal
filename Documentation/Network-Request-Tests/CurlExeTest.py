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
