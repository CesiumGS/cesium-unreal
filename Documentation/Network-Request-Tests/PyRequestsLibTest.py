import sys
import time
import requests
import concurrent.futures
import TestUtils
import TestKey


def load_url(url, session):
    print("Loading URL...")

    response = session.get(url)

    global gBytesDownloaded
    gBytesDownloaded += len(response.text);

    print(response.status_code)


if __name__ == "__main__":
    global gBytesDownloaded
    gBytesDownloaded = 0

    sessionString = TestKey.SessionKey

    urlArray = TestUtils.buildUrlArray(sessionString)

    session = requests.Session()
    session.headers.update(TestUtils.UnrealStyleHeaders)

    startMark = time.time()

    out = []
    CONNECTIONS = 100
    with concurrent.futures.ThreadPoolExecutor(max_workers=CONNECTIONS) as executor:
        future_to_url = (executor.submit(load_url, url, session) for url in urlArray)
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
