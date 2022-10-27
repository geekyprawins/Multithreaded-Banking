# Multithreaded Banking Application
## Operating Systems Lab Mini Project 2022

### Team Details:-
* [Praveen Varma](https://github.com/geekyprawins/)
* [Gautham Prabhu](https://github.com/GauthamPrabhuM)
* [Sunag Kamath](https://github.com/sravikam)
* [Yashwanth](https://github.com/yashwanth008)

### Project Details:-

Our solution is an application for handling **multiple accounts** in a bank. In this project, we tried to show the working of a banking account system and cover the basic functionality of banking, particularly with a focus on achieving **multi-client concurrency**. This project is developed using **C** language.

Our project makes use of multiple important operating systems concepts which include but are not limited to  **multithreading**, **process 
synchronization**, **deadlock handling**, **mutex locks**, **concurrent client-server interactions**, etc.

Our prototype CLI application has the capacity to handle up to *20 concurrent clients* in one session, the capacity can be ramped up as and when required. The code makes heavy use of threads in various aspects ranging from accepting sessions, handling client inputs, opening, and closing connections, etc.

### Running the application

In the project directory, you can run: `make`

This will generate all the executable files required to run the application.

Start the server and client respectively by running `./server` and `./client`.

All the functions can be tested with the help of menu-driven program in the client side, have a look at what server logs in the terminal too!


