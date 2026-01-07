import sys
import time
import urllib3
import concurrent.futures
import TestUtils
import TestKey


def load_url(url, http):
    print("Loading URL...")

    response = http.request("GET", url)

    global gBytesDownloaded
    gBytesDownloaded += len(response.data);

    print(response.status)


if __name__ == "__main__":
    global gBytesDownloaded
    gBytesDownloaded = 0

    sessionString = TestKey.SessionKey

    urlArray = TestUtils.buildUrlArray(sessionString)

    startMark = time.time()

    http = urllib3.PoolManager(headers=TestUtils.UnrealStyleHeaders)

    out = []
    CONNECTIONS = 100
    with concurrent.futures.ThreadPoolExecutor(max_workers=CONNECTIONS) as executor:
        future_to_url = (executor.submit(load_url, url, http) for url in urlArray)
        for future in concurrent.futures.as_completed(future_to_url):
            try:
                data = future.result()
            except Exception as exc:
                data = str(type(exc))
                print("Exception caught: " + data)
            finally:
                print("request completed")
                out.append(data)

    secsElapsed = time.time()-startMark

    TestUtils.printSummary(secsElapsed, gBytesDownloaded)
