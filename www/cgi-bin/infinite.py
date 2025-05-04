#!/usr/bin/python3

import sys
import time
import os

log_file = "/tmp/cgi_timeout_test.log"

def log(message):
    print(message)
    sys.stdout.flush()
    
    with open(log_file, "a") as f:
        f.write(message + "\n")
        f.flush()

pid = os.getpid()

log("<html><body>")
log("<h1>CGI Script Running...</h1>")
log("<p>This script will run indefinitely until terminated by the server.</p>")
log("<p>Process ID: {}</p>".format(pid))

counter = 0
while True:
    log("<p>Still running... ({}s)</p>".format(counter))
    
    with open(log_file, "a") as f:
        f.write("Time elapsed: {} seconds\n".format(counter))
    
    counter += 5
    time.sleep(5)

log("</body></html>")