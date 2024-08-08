import sys
import time
import requests
import threading
import TestUtils
import TestKey

def thread_function(url):
    print("Loading URL...")

    response = requests.get(url, headers=TestUtils.UnrealStyleHeaders)

    global gBytesDownloaded
    gBytesDownloaded += len(response.text);

    print(response.status_code)


if __name__ == "__main__":
    global gBytesDownloaded
    gBytesDownloaded = 0

    sessionString = TestKey.SessionKey

    urlArray = TestUtils.buildUrlArray(sessionString)

    threadArray = [] 
    for url in urlArray:
        print("Creating thread for ulr request...")
        thread = threading.Thread(target=thread_function, args=(url,))
        threadArray.append(thread)

    startMark = time.time()

    for thread in threadArray:
        thread.start()

    for thread in threadArray:
        thread.join ()

    secsElapsed = time.time()-startMark

    TestUtils.printSummary(secsElapsed, gBytesDownloaded)
