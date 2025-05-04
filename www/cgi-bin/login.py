#!/usr/bin/env python3

import os
from urllib.parse import parse_qs
import sys

def read_post_data():
    try:
        content_length = int(os.environ.get('CONTENT_LENGTH', 0))
    except ValueError:
        return None
    
    chunks = []
    bytes_read = 0
    
    while bytes_read < content_length:
        chunk = sys.stdin.buffer.read(min(8192, content_length - bytes_read))
        if not chunk:
            return None
        chunks.append(chunk)
        bytes_read += len(chunk)
    
    return b''.join(chunks).decode('utf-8')


def main():
    post_data = read_post_data()
    
    if post_data is None:
        print("Status: 400 Bad Request")
        print("Content-Type: text/plain")
        print()
        print("Invalid or incomplete POST data")
        return

    parsed_data = parse_qs(post_data)
    username = parsed_data.get('username', [''])[0]
    password = parsed_data.get('password', [''])[0]
    

    if username == 'admin' and password == 'password123':
        print("Status: 200 OK\r\n", end='')
        print("Content-Type: text/html\r\n", end='')
        print("\r\n", end='')
        print("<html>")
        print("<head>")
        print("<style>")
        print("body { font-family: Arial, sans-serif; margin: 40px auto; max-width: 800px; padding: 20px; }")
        print(".success-container { background-color: white; padding: 30px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); text-align: center; }")
        print(".back-button { display: inline-block; background-color: #4CAF50; color: white; padding: 12px 24px; text-decoration: none; border-radius: 4px; margin-top: 20px; }")
        print(".back-button:hover { background-color: #45a049; }")
        print("</style>")
        print("</head>")
        print("<body>")
        print("<div class='success-container'>")
        print("<h2>Login Successful!</h2>")
        print("<p>Welcome, " + username + "!</p>")
        print("<a href='/' class='back-button'>Back to Home</a>")
        print("</div>")
        print("</body></html>")
    else:
        print("Status: 401 Unauthorized\r\n", end="")
        print("Content-Type: text/html\r\n", end="")
        print("\r\n", end="")
        print("<html><body>")
        print("<h2>Login Failed</h2>")
        print("<p>Invalid username or password</p>")
        print("<a href='javascript:history.back()'>Go Back</a>")
        print("</body></html>")

if __name__ == "__main__":
   main()
   sys.exit(0)
