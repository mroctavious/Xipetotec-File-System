#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <string>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <vector>
#include <stack>
#define PORT 9999
using namespace std;

/*
    Xipetotec File System is a class which will control
    the way a user can send and receive files, it will have many
    options available using mysql as main DB.
*/
class XipetotecFS
{
    private:
        /*
            Attributes for the correct functionality of the sockets
        */
        struct sockaddr_in address;
        int sock;
        int valread;
        struct sockaddr_in serv_addr;
        

    public:
        /*
            Constructor and destructor
        */
        XipetotecFS( const char *addrs, int port );
        ~XipetotecFS();

        /*
            Command methods
        */
        int login( string user, string pass );
        int upload( vector<string> commands );
        int mkdir( vector<string> commands );
        int cd( vector<string> commands );
        int rmdir( vector<string> commands );
        int rm( vector<string> commands );
        int undofile( vector<string> commands );
        int undofolder( vector<string> commands );
        int mvdir( vector<string> commands );
        int mv( vector<string> commands );
        int cp( vector<string> commands );
        int download( vector<string> commands );
        int ls( vector<string> commands );
        int useradd( vector<string> commands );
        int userdel( vector<string> commands );
        int userlist( vector<string> commands);
        int share( vector<string> commands, string cmdType );
        

        /*
            Parsing methods
        */
        string remove_extra_whitespaces( const char *input );
        vector<string> stringTokenizer( string input, const char *delimiter );
        string prepareArgs( vector<string>args );
        string getWorkingDirString();
        string createCommand( string command, string args );

        /*
            Buffer for socket data receiving and sending
        */
        char *buffer;
        int buffer_size;
        
        /*
            Infinite loop that will keep reading commands from user
            and execute them in the server
        */
        void run();
        
        /*
            Main attributes for correct functionality
        */
        string username;
        string passwd;
        string currentWD;
        string downloadFolder;

        /*
            Attributes for the prompt
        */
        int currentWD_ID;
        vector<int> folderIDs;
        vector<string> folderNames;

};

XipetotecFS::XipetotecFS( const char *addrs, int port )
{
    /* 
        Create the buffer which will receive all the bytes from the socket
        and clean it from memory trash
    */
    buffer_size = 1024*1024;
    buffer = (char*) malloc( sizeof(char) * buffer_size );
    memset( buffer, '\0', sizeof(char) * buffer_size );

    /*
        Setting the username and password that will be used in all commands
    */
    username="";
    passwd="";
    currentWD="/";
    downloadFolder="./Downloads";

    /*
        Prepare all the variables for the socket connection
    */
    if ( (sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    { 
        printf("\n Socket creation error \n"); 
        exit(5);
    }
    memset(&serv_addr, '0', sizeof(serv_addr)); 
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    /*
        Convert IPv4 and IPv6 addresses from text to binary form
    */
    if( inet_pton(AF_INET, addrs, &serv_addr.sin_addr) <= 0 )  
    { 
        printf("\nInvalid address/ Address not supported \n"); 
        exit(6); 
    }

    /*
        Start new connection socket to the port and address
        and check if it was able to create it
    */
    if ( connect( sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr) ) < 0 ) 
    { 
        printf("\nConnection Failed \n"); 
        exit(7); 
    } 
}

int XipetotecFS::login( string user, string pass )
{
    username=user;
    passwd=pass;

    string loginString="";
    loginString+=username;
    loginString+="|";
    loginString+=passwd;
    send( sock , loginString.c_str(), loginString.size(), 0 );
    valread = read( sock , buffer, buffer_size);
    string res(buffer);

    if( buffer[0] == '0' )
    {
        return 0;
    }
    else
    {
        currentWD_ID = std::stoi(res);
        folderIDs.push_back(currentWD_ID);
        folderNames.push_back(currentWD);
        return 1;
    }
    
}

/*
    This function will execute whenever the object is out of scope
*/
XipetotecFS::~XipetotecFS()
{
    free(buffer);
    //free(currentPath);
    close(sock);
}

/*
----------------------------------------------------
                COMMAND METHODS
----------------------------------------------------
*/
int XipetotecFS::cd( vector<string> commands )
{
    commands.insert( commands.begin()+1, to_string(currentWD_ID) );
    if( commands.size() < 3 )
        return 1;
    if( commands[2] == "..")
    {
        if( folderIDs.size() <= 1 )
            return 4;
        folderIDs.pop_back();
        folderNames.pop_back();
        currentWD=folderNames[ folderNames.size() - 1 ];
        currentWD_ID=folderIDs[ folderIDs.size() - 1 ];
        return 3;
    }
    string args=prepareArgs(commands);
    string formatedCommand=createCommand("C", args);
    send( sock , formatedCommand.c_str(), formatedCommand.size(), 0 );
    valread = read( sock , buffer, buffer_size);
    string res(buffer);

    if(res == "NOT_FOUND")
        return 2;
    else
    {
        currentWD_ID=stoi( res );
        currentWD=commands[2];
    }
    return 0;
}

int XipetotecFS::mvdir( vector<string> commands )
{
    commands.insert( commands.begin()+1, to_string(currentWD_ID) );
    if( commands.size() < 3 )
        return 1;
    if( commands.size() < 4 )
        return 2;

    string args=prepareArgs(commands);
    string formatedCommand=createCommand("Z", args);
    send( sock , formatedCommand.c_str(), formatedCommand.size(), 0 );
    valread = read( sock , buffer, buffer_size);
    string res(buffer);

    if(res == "NOT_FOUND")
        return 3;

    return 0;
}

int XipetotecFS::useradd( vector<string> commands )
{
    commands.insert( commands.begin()+1, to_string(currentWD_ID) );
    if( commands.size() < 3 )
        return 1;
    if( commands.size() < 4 )
        return 2;

    string args=prepareArgs(commands);
    string formatedCommand=createCommand("G", args);
    send( sock , formatedCommand.c_str(), formatedCommand.size(), 0 );
    valread = read( sock , buffer, buffer_size);
    string res(buffer);

    if(res == "NOT_FOUND")
        return 3;

    return 0;
}

int XipetotecFS::userdel( vector<string> commands )
{
    commands.insert( commands.begin()+1, to_string(currentWD_ID) );
    if( commands.size() < 3 )
        return 1;

    string args=prepareArgs(commands);
    string formatedCommand=createCommand("E", args);
    send( sock , formatedCommand.c_str(), formatedCommand.size(), 0 );
    valread = read( sock , buffer, buffer_size);
    string res(buffer);

    if(res == "USER_NOT_FOUND")
        return 3;

    return 0;
}
int XipetotecFS::mv( vector<string> commands )
{
    commands.insert( commands.begin()+1, to_string(currentWD_ID) );
    if( commands.size() < 3 )
        return 1;
    if( commands.size() < 4 )
        return 2;

    string args=prepareArgs(commands);
    string formatedCommand=createCommand("K", args);
    send( sock , formatedCommand.c_str(), formatedCommand.size(), 0 );
    valread = read( sock , buffer, buffer_size);
    string res(buffer);

    if(res == "FILE_NOT_FOUND")
        return 3;
    if(res == "FOLDER_NOT_FOUND")
        return 4;

    return 0;
}

int XipetotecFS::cp( vector<string> commands )
{
    commands.insert( commands.begin()+1, to_string(currentWD_ID) );
    if( commands.size() < 3 )
        return 1;
    if( commands.size() < 4 )
        return 2;

    string args=prepareArgs(commands);
    string formatedCommand=createCommand("O", args);
    send( sock , formatedCommand.c_str(), formatedCommand.size(), 0 );
    valread = read( sock , buffer, buffer_size);
    string res(buffer);

    if(res == "FILE_NOT_FOUND")
        return 3;
    if(res == "FOLDER_NOT_FOUND")
        return 4;

    return 0;
}

int XipetotecFS::mkdir( vector<string> commands )
{
    commands.insert( commands.begin()+1, to_string(currentWD_ID) );
    commands.insert( commands.begin()+1, currentWD );
    if( commands.size() < 3 )
        return 1;
    string args=prepareArgs(commands);
    string formatedCommand=createCommand("M", args);
    send( sock , formatedCommand.c_str(), formatedCommand.size(), 0 );
    valread = read( sock , buffer, buffer_size);
    string res(buffer);
    cout << res << endl;
    return 0;
}

int XipetotecFS::rmdir( vector<string> commands )
{
    commands.insert( commands.begin()+1, to_string(currentWD_ID) );
    commands.insert( commands.begin()+1, currentWD );
    if( commands.size() < 3 )
        return 1;
    string args=prepareArgs(commands);
    string formatedCommand=createCommand("P", args);
    send( sock , formatedCommand.c_str(), formatedCommand.size(), 0 );
    valread = read( sock , buffer, buffer_size);
    string res(buffer);
    if(res == "NOT_FOUND")
    {
        return 2;
    }
    else
    {
        cout << res << endl;
        return 0;
    }
    
    return 0;
}

int XipetotecFS::share( vector<string> commands, string cmdType )
{
    commands.insert( commands.begin()+1, to_string(currentWD_ID) );
    commands.insert( commands.begin()+1, currentWD );
    if( commands.size() < 3 )
        return 1;
    string args=prepareArgs(commands);
    string formatedCommand=createCommand( cmdType, args);
    send( sock , formatedCommand.c_str(), formatedCommand.size(), 0 );
    valread = read( sock , buffer, buffer_size);
    string res(buffer);
    if(res == "NOT_FOUND")
    {
        return 2;
    }
    
    return 0;
}

int XipetotecFS::rm( vector<string> commands )
{
    commands.insert( commands.begin()+1, to_string(currentWD_ID) );
    commands.insert( commands.begin()+1, currentWD );
    if( commands.size() < 3 )
        return 1;
    string args=prepareArgs(commands);
    string formatedCommand=createCommand("R", args);
    send( sock , formatedCommand.c_str(), formatedCommand.size(), 0 );
    valread = read( sock , buffer, buffer_size);
    string res(buffer);
    if(res == "NOT_FOUND")
    {
        return 2;
    }
    else
    {
        return 0;
    }
    
    return 0;
}

int XipetotecFS::undofile( vector<string> commands )
{
    commands.insert( commands.begin()+1, to_string(currentWD_ID) );
    commands.insert( commands.begin()+1, currentWD );
    if( commands.size() < 3 )
        return 1;
    string args=prepareArgs(commands);
    string formatedCommand=createCommand("B", args);
    send( sock , formatedCommand.c_str(), formatedCommand.size(), 0 );
    valread = read( sock , buffer, buffer_size);
    string res(buffer);
    if(res == "NOT_FOUND")
    {
        return 2;
    }
    else
    {
        return 0;
    }
    
    return 0;
}

int XipetotecFS::undofolder( vector<string> commands )
{
    commands.insert( commands.begin()+1, to_string(currentWD_ID) );
    commands.insert( commands.begin()+1, currentWD );
    if( commands.size() < 3 )
        return 1;
    string args=prepareArgs(commands);
    string formatedCommand=createCommand("A", args);
    send( sock , formatedCommand.c_str(), formatedCommand.size(), 0 );
    valread = read( sock , buffer, buffer_size);
    string res(buffer);
    if(res == "NOT_FOUND")
    {
        return 2;
    }
    else
    {
        return 0;
    }
    
    return 0;
}
/*
    This method will return all the folders and files in the current
    working directory at the server
*/
int XipetotecFS::ls( vector<string> commands )
{
    /*
        Format the string to send to the server
    */
    commands.insert( commands.begin()+1, to_string(currentWD_ID) );
    string args=prepareArgs(commands);
    string formatedCommand=createCommand("L", args);
    send( sock , formatedCommand.c_str(), formatedCommand.size(), 0 );

    /*
        Get the folders first and print them to the user
    */
    memset(buffer, '\0', sizeof(char) * buffer_size);
    valread = read( sock , buffer, buffer_size);
    string foldersString(buffer);

    vector<string> folders=stringTokenizer(foldersString, "|");
    cout << "Folders: " << endl;
    for(int i=0; i<folders.size(); ++i)
    {
        cout << folders[i] << "\t";
    }
    cout << endl;

    /*
        Send message to server to keep receiving now the files
        of the current directory
    */
    string okFolder("OK");
    send( sock , okFolder.c_str(), okFolder.size(), 0 );

    /*
        Get the files from the current folder and print them to the user
    */
    memset(buffer, '\0', sizeof(char) * buffer_size);
    valread = read( sock , buffer, buffer_size);
    string filesString(buffer);

    vector<string> files=stringTokenizer(filesString, "|");
    cout << "Files: " << endl;
    for(int i=0; i<files.size(); ++i)
    {
        cout << files[i] << "\t";
    }
    cout << endl;

    return 0;
}

int XipetotecFS::userlist( vector<string> commands )
{
    /*
        Format the string to send to the server
    */
    commands.insert( commands.begin()+1, to_string(currentWD_ID) );
    string args=prepareArgs(commands);
    string formatedCommand=createCommand("I", args);
    send( sock , formatedCommand.c_str(), formatedCommand.size(), 0 );

    /*
        Get the folders first and print them to the user
    */
    memset(buffer, '\0', sizeof(char) * buffer_size);
    valread = read( sock , buffer, buffer_size);
    string usersString(buffer);

    vector<string> usersList=stringTokenizer(usersString, "|");
    cout << "Users: " << endl;
    for(int i=0; i<usersList.size(); ++i)
    {
        cout << usersList[i] << "\t";
    }
    cout << endl;
    return 0;
}
/*
    This method will upload a file to the server and
    save all the information in the database, this is
    done using sockets
*/
int XipetotecFS::upload( vector<string> commands )
{
    /*
        Try to open the file, if it exists then start sending file
        to the server
    */
    if( commands.size() < 2 )
    {
        cout << "No file selected." << endl;
        return 1;
    }

    FILE *fp = fopen(commands[1].c_str(), "rb");
    if(fp == NULL)
    {
        cout << "File could not be opened." << endl;
        return 2;
    }

    /*
        Now that the file its open, we need to check how many 
        bytes the server will receive
    */
    unsigned int fullBytes=0;
    int bytes;
    while( ( bytes= fread( buffer, 1, sizeof(buffer), fp) ) > 0 )
        fullBytes+=bytes;

    /*
        Add to the arguments the size of the file and the current path
    */
    string fSize=to_string(fullBytes);
    commands.push_back(currentWD);
    commands.push_back(to_string(currentWD_ID));
    commands.push_back(fSize);
    string args=prepareArgs(commands);

    /*
        Seek to the beginning of the file
    */
    fseek(fp, 0, SEEK_SET);

    string formatedCommand=createCommand("U", args);
    send( sock , formatedCommand.c_str(), formatedCommand.size(), 0 );
    valread = read( sock , buffer, buffer_size);
    string res(buffer);
    if(res == "OVERWRITE")
    {
        while(1)
        {
            cout << "File already exists, do you want to overwrite it? [Y/N]" << endl << "overwrite? > ";
            string wantToOverwrite;
            getline(cin, wantToOverwrite );
            if( wantToOverwrite == "N" || wantToOverwrite == "NO" || wantToOverwrite == "n" || wantToOverwrite == "no" )
            {
                string answer("N");
                send( sock , answer.c_str(), answer.size(), 0 );
                valread = read( sock , buffer, buffer_size);
                memset( buffer, '\0', buffer_size );
                return 0;
            }
            else if( wantToOverwrite == "Y" || wantToOverwrite == "YES" || wantToOverwrite == "y" || wantToOverwrite == "yes" )
            {
                string answer("Y");
                send( sock , answer.c_str(), answer.size(), 0 );
                valread = read( sock , buffer, buffer_size);
                memset( buffer, '\0', buffer_size );
                break;
            }
            else
                cout << "Wrong option, please just write 'Y' o 'N'" << endl;
        }
    }
    else if( res == "EXISTS" )
    {
        cout << "File already exists, but you are not the owner, can not upload, sorry :(" << endl;
        return 1;
    }
    
    /*
        The total number of elements successfully read
    */
    while( (bytes = fread( buffer, 1, sizeof(buffer), fp)) > 0 )
    {
        send( sock, buffer, bytes, 0);
    }
    //Close file
    fclose(fp);

    valread = read( sock , buffer, buffer_size );
    string response(buffer);
    cout << response << endl;

    return 0;
}

/*
    This method will download file from the server and
    write a new file in the Downloads directory, this is
    done using sockets
*/
int XipetotecFS::download( vector<string> commands )
{
    /*
        Try to open the file, if it exists then start sending file
        to the server
    */
    if( commands.size() < 2 )
    {
        cout << "No file selected." << endl;
        return 1;
    }

    /*
        Add to the arguments the size of the file and the current path
    */
    commands.push_back(currentWD);
    commands.push_back(to_string(currentWD_ID));
    string args=prepareArgs(commands);
    string formatedCommand=createCommand("D", args);

    send( sock , formatedCommand.c_str(), formatedCommand.size(), 0 );

    /*
        Get the file size to receive first
    */
    valread = read( sock , buffer, buffer_size);
    string fileSizeStr(buffer);
    if( fileSizeStr == "FILE_NOT_FOUND" )
    {
        cout << "File not found!" << endl;
        return 3;
    }
    if( fileSizeStr == "PERMISSION_DENIED" )
    {
        cout << "You can not download, Permission Denied :/" << endl;
        return 4;
    }
    unsigned int fileSize=stoi(fileSizeStr);
    unsigned int currentBytesRead=0;

    /*
        Clear buffer
    */
    memset(buffer, '\0', sizeof(char)*buffer_size);

    /*
        Send flag to start receiving the file
    */
    string readyToDownload="OK";
    send( sock , readyToDownload.c_str(), readyToDownload.size(), 0 );

    /*
        Start downloading data from the server, open new file and
        write the data in there
    */
   
    string outFile=downloadFolder+"/"+commands[1];
    FILE *fp = fopen(outFile.c_str(), "wb");
    if(fp == NULL)
    {
        cout << "File could not be opened." << endl;
        return 2;
    }

    /*
        Keep reading bytes until the end of the file
        is reached
    */
    while( currentBytesRead != fileSize )
    {
        //Receive bytes from socket
        valread = read( sock , buffer, buffer_size);

        //Write in the file the bytes read by the socket
        fwrite (buffer , sizeof(char), valread, fp);
        currentBytesRead+=valread;

        //Clear buffer to avoid trash
        memset( buffer, '\0', sizeof(char)* buffer_size );
    }

    /*
        Send message of done receiving
    */
    send( sock , readyToDownload.c_str(), readyToDownload.size(), 0 );

    /*
        Closing connection
    */
    fclose(fp);
    valread = read( sock , buffer, buffer_size);
    string res(buffer);
    cout << res << endl;

    return 0;
}

/*
----------------------------------------------------
                PARSING METHODS
----------------------------------------------------
*/

/*
    This method will prepare the string to be sent to 
    the server...
    FORMAT OF STRING COMMAND
    COMMAND|ARGS
*/
string XipetotecFS::createCommand( string command, string args )
{
    string out="";
    out+=command;
    out+="|";
    out+=args;
    return out;
}

/*
    Given the command with the args, we are going to prepare
    the args with the correct format, so it can be compatible with the
    server
*/
string XipetotecFS::prepareArgs( vector<string>args )
{
    string out="";

    if( args.size() > 1 )
    {
        for( int i=1; i<args.size(); ++i )
        {
            out+=args[i];
            out+="|";
        }
        out.pop_back();
    }
    return out;
}

/*
    This function will clean the extra whitespaces found in the command
    entered by the user
*/
string XipetotecFS::remove_extra_whitespaces( const char *input )
{
    char output[2048];
    int inputIndex = 0;
    int outputIndex = 0;
    while(input[inputIndex] != '\0')
    {
        output[outputIndex] = input[inputIndex];

        if(input[inputIndex] == ' ')
        {
            while(input[inputIndex + 1] == ' ')
            {
                /* 
                    Skip over any extra spaces
                */
                inputIndex++;
            }
        }

        outputIndex++;
        inputIndex++;
    }

    /*
        Null-terminate output
    */
    output[outputIndex] = '\0';
    string returnString(output);
    return returnString;
}

/*
    This function will go through all the commands and will parse them
    afterwards it will return a vector with the main command and the args
*/
vector<string> XipetotecFS::stringTokenizer( string input, const char* delimiter )
{
    /*
        Prepare the tokenizer
    */
    int commandLen=input.length();
    vector<string> commands;
    
    char str[commandLen];
    strcpy(str, input.c_str() );


    /*
        Start parsing the commands
    */
    char* token = strtok(str, delimiter);
    while (token)
    {
        string tmp(token);
        commands.push_back(tmp);
        token = strtok(NULL, delimiter);

    }
    return commands;
}

string XipetotecFS::getWorkingDirString()
{
    string out="";
    for(int i=0; i<folderNames.size(); ++i)
    {
        if( 0 == i )
            out+=folderNames[i];
        else
            out+=folderNames[i]+"/";
    }
    return out;
}

/*
----------------------------------------------------
                MAIN PROGRAM METHOD
----------------------------------------------------
*/


/*
    Main program which can be executed by a thread,
    this function will analize the commands and do
    what the user wants to do
*/
void XipetotecFS::run()
{
    while(true)
    {
        /*
            Printing the prompt
        */
        cout << username << ":" << getWorkingDirString()  << "$ ";
        
        /*
            Cleaning and parsing the commands
        */
        string full_command;
        getline(cin, full_command );
        if( full_command.size() < 1 )
            continue;
        string cleaned = remove_extra_whitespaces( full_command.c_str() );

        /*
            Commands vector at index 0 will have the main command to execute,
            then all the others positions greater than 0, will be the arguments
        */
        vector<string> commands = stringTokenizer(cleaned, " ");
        string mainCommand = commands[0];
        memset(buffer, '\0', sizeof(char) * buffer_size);
        /*
            Depending of the command, the do...
        */
        if( mainCommand == "exit" )
        {
            cout << "Adios!" << endl;
            string adios("EXIT");
            send( sock , adios.c_str(), adios.size(), 0 );
            return;
        }
        else if ( mainCommand == "ls" )
        {
            ls(commands);
        }
        else if ( mainCommand == "mv" )
        {
            int result=mv(commands);
            if( result == 1 )
            {
                cout << "No directory was given." << endl;
            }
            else if( result == 2 )
            {
                cout << "No destiny directory was given." << endl;
            }
            else if( result == 3 )
            {
                cout << "File not found!" << endl;
            }
            else if( result == 4 )
            {
                cout << "Folder destiny not found!" << endl;
            }
            else if( result == 0 )
            {
                cout << "File moved." << endl;
            }
        }
        else if ( mainCommand == "upload" )
        {
            upload(commands);
        }
        else if ( mainCommand == "download" )
        {
            download(commands);
        }
        else if ( mainCommand == "rmdir" )
        {
            int result=rmdir(commands);
            if( result == 1 )
            {
                cout << "No directory was given" << endl;
            }
            else if( result == 2 )
            {
                cout << "Folder not found!" << endl;
            }
        }
        else if ( mainCommand == "useradd" )
        {
            if( username == "root" )
            {
                int result=useradd(commands);
                if( result == 1 )
                {
                    cout << "No username was given." << endl;
                }
                else if( result == 2 )
                {
                    cout << "No password was given." << endl;
                }
                else if( result == 0 )
                    cout << "User created." << endl;
            }
            else
                cout << "Access denied!" << endl;
        }
        else if ( mainCommand == "userdel" )
        {
            if( username == "root" )
            {
                int result=userdel(commands);
                if( result == 1 )
                {
                    cout << "No username was given." << endl;
                }
                else if( result == 3 )
                {
                    cout << "Username was not found." << endl;
                }
                else if( result == 0 )
                {
                    cout << "User deleted." << endl;
                }
                
            }
            else
                cout << "Access denied!" << endl;
        }
        else if ( mainCommand == "userlist" )
        {
            if( username == "root" )
            {
                int result=userlist(commands);
                
            }
            else
                cout << "Access denied!" << endl;
        }
        else if ( mainCommand == "rm" )
        {
            int result=rm(commands);
            if( result == 1 )
            {
                cout << "No directory was given" << endl;
            }
            else if( result == 2 )
            {
                cout << "Folder not found!" << endl;
            }
            else if( result == 0 )
                cout << "File deleted." << endl;
        }
        else if ( mainCommand == "undofile" )
        {
            int result=undofile(commands);
            if( result == 1 )
            {
                cout << "No File was given" << endl;
            }
            else if( result == 2 )
            {
                cout << "File not found!" << endl;
            }
            else if( result == 0 )
                cout << "File Undone." << endl;
        }
        else if ( mainCommand == "undofolder" )
        {
            int result=undofolder(commands);
            if( result == 1 )
            {
                cout << "No directory was given" << endl;
            }
            else if( result == 2 )
            {
                cout << "Folder not found!" << endl;
            }
            else if( result == 0 )
                cout << "Folder Undone." << endl;
        }
        else if ( mainCommand == "cp" )
        {
            int result=cp(commands);
            if( result == 1 )
            {
                cout << "No File was given." << endl;
            }
            else if( result == 2 )
            {
                cout << "No destiny directory was given." << endl;
            }
            else if( result == 3 )
            {
                cout << "File not found!" << endl;
            }
            else if( result == 4 )
            {
                cout << "Folder destiny not found!" << endl;
            }
            else if( result == 0 )
            {
                cout << "File copied." << endl;
            }
        }
        else if ( mainCommand == "pwd" )
        {
            cout << "Current directory: " << getWorkingDirString() << endl;
        }
        else if ( mainCommand == "cd" )
        {
            int result=cd(commands);
            if( result == 1 )
            {
                cout << "No directory was given" << endl;
            }
            else if( result == 2 )
            {
                cout << "Folder not found!" << endl;
            }
            else if( result == 0 )
            {
                folderIDs.push_back(currentWD_ID);
                folderNames.push_back(currentWD);
            }
        }
        else if ( mainCommand == "mvdir" )
        {
            int result=mvdir(commands);
            if( result == 1 )
            {
                cout << "No directory was given." << endl;
            }
            else if( result == 2 )
            {
                cout << "No destiny directory was given." << endl;
            }
            else if( result == 3 )
            {
                cout << "Folder not found!" << endl;
            }
            else if( result == 0 )
            {
                cout << "Folder moved." << endl;
            }
        }
        else if ( mainCommand == "mkdir" )
        {
            cout << "Make new directory" << endl;
            mkdir(commands);
        }
        
        else if ( mainCommand == "share" )
        {
            string commandType="V";
            int result=share(commands, commandType);
            if( result == 1 )
            {
                cout << "No file was given" << endl;
            }
            else if( result == 2 )
            {
                cout << "File not found!" << endl;
            }
            else if( result == 3 )
                cout << "Permission Denied!" << endl;
            else if( result == 0 )
                cout << "Others can download this file." << endl;
        }
        
        else if ( mainCommand == "rmshare" )
        {
            string commandType="N";
            int result=share(commands, commandType);
            if( result == 1 )
            {
                cout << "No file was given" << endl;
            }
            else if( result == 2 )
            {
                cout << "File not found!" << endl;
            }
            else if( result == 3 )
                cout << "Permission Denied!" << endl;
            else if( result == 0 )
                cout << "Others can NOT download this file." << endl;
        }
        else
        {
            cout << "Wrong command!" << endl;
        }
        
    }
}


int main(int argc, char **argv)
{
    if( argc < 3 )
    {
        cout << "Syntax error!" << endl << "\t" << argv[0] << "  <Username>  <Password>" << endl;
        exit(1);
    }

    XipetotecFS fs=XipetotecFS("127.0.0.1", 9999 );
    if( fs.login( argv[1], argv[2]) )
    {
        fs.run();
    }
    else
    {
        cout << "Wrong username or password, please try again!" << endl;
    }
        

    return 0;
}
