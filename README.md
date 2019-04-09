# Xipetotec File System

Xipetotec File System is a school project that combines the power of C++, Python and MySQL. The program can transfer files between sockets and between users.

  - Login
  - Create and delete users
  - Upload File
  - Download a file
  - Move around folders
  - Make file only downloadable for you
  - Make directories
  - Move files between folders
  - Copy files between folders
  - List the files in the folder


# NOTE: The server uses the port 9999

### Authors

Xipetotec File System was created from a project idea, the idea is from the Master Carlos Alberto Olmos Trejo, teacher of the Universidad Autonoma de Queretaro, Facultad de Informatica who tought us the magic of sockets.

![N|Solid](http://posgradofif.uaq.mx/images/MIEVEA/nab/coordinador_mievea.JPG)


### Installation

Xipetotec File System requires Ubuntu or debian to install and run.

First download the repository
```sh
$ git clone https://github.com/mroctavious/Xipetotec-File-System.git
```

Now go to the folder and change the permission of the installation script and run the script
```sh
$ cd Xipetotec-File-System
$ chmod 755 InstallServer.sh
```

Now go log in as root user.
```sh
$ sudo su
$ ./InstallServer.sh
```

If you don't have installed mysql server, it will install it for you but it will prompt for a root password, please remember the password because you will use it later.

In case you already have installed mysql server, you must have the root password to continue.

Open your favorite Terminal and run the server with any of this commands.

Run and see whats happening:
```sh
$ XipetotecServer
```

Run and save the output in a file log:
```sh
$ XipetotecServer 2&>1 > log.txt
```

Run in background so you can log out later:
```sh
$ nohup XipetotecServer 2&>1 > log.txt &
```

### Client
To compile the client you must run the following command:
```sh
$ g++ XipetotecFS.cpp -o XipetotecClient
```

Now create the Downloads folder:
```sh
$ mkdir Downloads
```
Move all your files you want to transfer to the client folder and enjoy.

To start the client, you must run with the following arguments:
```sh
$ ./XipetotecClient <USER> <PASSWORD>
```

License
----

GNU General Public License v3.0


**Free Software**

