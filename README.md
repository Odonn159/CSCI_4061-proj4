# CSCI_4061-proj4
Http Get Server

HTTP (and its encrypted counterpart, HTTPS) is the protocol powering the World Wide Web. Your browser acts as an HTTP client, while all websites and their contents are made accessible by processes acting as HTTP servers. You will gain firsthand experience with HTTP and the underlying TCP sockets over which HTTP text is communicated in this project.  

In Part 1 of the project, you will implement a simple HTTP server. You will need to perform the necessary socket setup, parse incoming client HTTP requests, and then reply with valid HTTP responses communicating the contents of files stored locally on disk. Your server code will be able to interact with any HTTP client, such as your HTTP client from Lab 11, the wget command-line utility, or a web browser.  

In Part 2 of the project, you will convert your single-threaded HTTP server (which can only maintain one client session at a time) into a multi-threaded server with a design much closer to those of the real servers that power the modern web. In particular, we will use the thread pool paradigm to create a fixed-size collection of threads each capable of interacting with one client.  

This project will focus on a few important systems programming topics:  

TCP server socket setup and initialization with socket(), bind(), and listen()  
Server-side TCP communication with accept() followed by read() and write()  
HTTP request parsing and response generation  
Clean server termination through signal handling  
Thread creation and management with pthread_create() and pthread_join()  
Implementation of a thread-safe queue data structure with the pthread_mutex_t and pthread_cond_t primitives  
