"""
    This program will be the socket server for 
    file transfer, this project was an idea of my
    teache Olmos, it was very fun for me to make this
    real

    Made by Octavio Rodriguez
    Universidad Autonoma de Queretaro
    
"""

import socket
import sys
import pymysql
import threading
import string
import random
import os

"""
    Main class of the server, when a new connection is received
    the class will send a new thread to attend the user which is 
    currently connected.

    All this connection and files are managed by the database which
    has its own procedures and I just called them. The database is
    MySQL.

    Author: Eric Octavio Rodriguez Garcia
"""
class XipetotecFSServer:

    ##Constructor
    def __init__(self, host="localhost", user="xipetotec", password="Mexico123!", db="xipetotecFS", port=9999 ):
        self.db = pymysql.connect( host, user, password, db )
        self.cursor = self.db.cursor()
        self.server_socket = socket.socket()
        self.server_socket.bind( ( 'localhost', port ) )
        self.server_socket.listen( 16 )
        self.CHUNK_SIZE=512
        self.mainDirectory="/opt/Xipetotec/files"


    ##Destructor
    def __del__(self):
        self.db.close()


    ##This method will return if the user was able to login
    def login(self, user, passwd, clientSocket ):
        sqlQuery = "SELECT usr.id, usr.username, fol.id FROM folders fol, users usr WHERE usr.username='%s' AND usr.passwd=MD5('%s') AND folder_name='/' AND parent_folder is NULL;" % (user, passwd)
        try:
            ##Execute command
            self.cursor.execute(sqlQuery)

            # Fetch all the rows in a list of lists
            results = self.cursor.fetchall()
            if len(results) > 0:
                for row in results:
                    id = row[0]
                    username = row[1]
                    idPath = row[2]
                    
                    clientSocket.send( str(idPath).encode() )
                    return(int(id), username, int(idPath))


            else:
                clientSocket.send( "0".encode() )
                return None

        except:
            clientSocket.send( "0".encode() )


    """
        This method will parse the commands received from
        the socket and will return a dictionary with the 
        arguments
    """
    def parseCommand( self, strng ):
        cmds=strng.split("|")
        command=cmds[0]
        args=cmds[1:]

        parsedCommands = {
            'command':command,
            'args':args
        }
        return parsedCommands


    """
        This method will return a dictionary with the user data,
        I get the information by parsing the string received from
        the main socket
    """
    def parseLogin( self, strng ):
        cmds=strng.split("|")
        user=cmds[0]
        passwd=cmds[1]


        parsedData = {
            'username':user,
            'passwd':passwd
        }
        return parsedData


    """
        This method will try to execute the insert query
        in case of failure the db will rollback
    """
    def executeQuery(self, sqlQuery):
        try:
            # Execute the SQL command
            self.cursor.execute(sqlQuery)
    
            # Commit your changes in the database
            self.db.commit()
        except:
            # Rollback in case there is any error
            self.db.rollback()


    """
        Check if the file exists, this will help to avoid
        repetitions when we want to upload a file.
        This method returns None in case the file does not exists
        and will return an Integer which is the ID of the file.
    """
    def fileExists( self, workingDir=None, filename=None, status=1 ):

        sqlQuery="SELECT id FROM files WHERE filename='%s' AND folder_id=%d AND status=%d;" % (filename, workingDir, status)

        try:
            # Execute the SQL command
            self.cursor.execute(sqlQuery)

            # Fetch all the rows in a list of lists
            results = self.cursor.fetchall()

            if len(results) > 0:
                return int(results[0][0])
            else:
                return None
        except:
            print("Error with query: ", sqlQuery)

    
    """
        This method will get the data of the owner of the file
    """
    def getFileOwner( self, idFile=None, default="file" ):
        if idFile == None:
            raise ValueError("No file id was given")
        if default == "file":
            sqlQuery="SELECT id, username FROM users WHERE id=(SELECT fileOwner FROM files WHERE id=%d );" % idFile
        elif default == "folder":
            sqlQuery="SELECT id, username FROM users WHERE id=(SELECT folder_owner FROM folders WHERE id=%d );" % idFile
        else:
            return None
        try:
            # Execute the SQL command
            self.cursor.execute(sqlQuery)

            # Fetch all the rows in a list of lists
            results = self.cursor.fetchall()

            if len(results) > 0:
                return ( int(results[0][0]), results[0][1] )
            else:
                return None
        except:
            print("Error with query: ", sqlQuery)


    """
        This method will get the id of the username
    """
    def getUsernameID( self, username=None ):
        if username == None:
            raise ValueError("No username was given")

        sqlQuery="SELECT id FROM users WHERE username='%s';" % username
        try:
            # Execute the SQL command
            self.cursor.execute(sqlQuery)

            # Fetch all the rows in a list of lists
            results = self.cursor.fetchall()

            if len(results) > 0:
                return int(results[0][0])
            else:
                return None
        except:
            print("Error with query: ", sqlQuery)


    """
        This method will return a 128 characters long string, it will be used to
        store the file in the server and map it to the database
    """ 
    def generateString( self, size=128, chars=string.ascii_uppercase + string.digits ):
        return ''.join(random.choice(chars) for _ in range(size))

    """
        This method will start receiving a file from a client,
        after getting all the bytes of the file then will save and 
        map the user to the file in the database
    """
    def uploadFile(self, client_socket, addr, user, arguments ):
        ##Update db
        self.db.commit()
        ##Random filename to store in server
        randomFilename=self.generateString()
        filename=self.mainDirectory + "/" + randomFilename

        ##Parsing commands
        #The original filename will be saved in the database and link it to the
        #random filename
        originalFilename=arguments[0]

        ##Save this file in the current working directory
        workingDir=arguments[1]

        ##Folder id
        folderID=int(arguments[2])

        ##Size of the file, with this we will know when to stop
        #receiving bytes from the client
        byteSize=int(arguments[3])


        ##Check if the file already exists, in case it does then
        #check if its owner, if so, then ask user if he wants to
        #overwrite the file
        idFile=self.fileExists( workingDir=folderID,filename=originalFilename)
        overwrite=False
        if idFile != None:
            idOwner,userOwner=self.getFileOwner(idFile=idFile)

            if user[1] == userOwner:
                client_socket.send( "OVERWRITE".encode() )
                answer = client_socket.recv(self.CHUNK_SIZE)
                if answer.decode() == "N":
                    #print("Not overwriting")
                    client_socket.send( "OK".encode() )
                    return
                elif answer.decode() == "Y":
                    #print("Overwriting")
                    overwrite=True
                    client_socket.send( "OK".encode() )
            else:
                client_socket.send( "EXISTS".encode() )
                return
                #print("Can not overwrite, owner: ", userOwner)
        else:
            client_socket.send( "OK".encode() )

        ##Start receiving bytes from client
        currentBytes=0
        chunk = client_socket.recv(self.CHUNK_SIZE)
        if chunk:

            ##Write bytes in local file
            with open( filename, 'wb' ) as f:
                f.write(chunk)
                currentBytes+=len(chunk)
                if currentBytes != byteSize:

                    ##Keep receiving until we get the file
                    while chunk:
                        chunk = client_socket.recv(self.CHUNK_SIZE)
                        f.write(chunk)
                        currentBytes+=len(chunk)

                        if currentBytes == byteSize:
                            break

        ##Send message to the user, upload was completed
        client_socket.send( "Upload done".encode() )

        ##Start the database save process
        if overwrite == False:
            sqlQuery="CALL addFile( '%s', '%s', %d, %d);" % ( originalFilename, randomFilename, user[0], folderID)
            self.executeQuery(sqlQuery)
        else:
            sqlQuery="UPDATE files SET serverFile='%s' WHERE id=%d;" % ( randomFilename, idFile )
            self.executeQuery(sqlQuery)


    """
        This method will receive the id of the file and will return 
        the 128 bytes long random string, all the files are in the same
        folder
    """
    def getServerFile(self, fileID):
        sqlQuery="SELECT serverFile FROM files WHERE id=%d;" % int(fileID)
        try:
            # Execute the SQL command
            self.cursor.execute(sqlQuery)

            # Fetch all the rows in a list of lists
            results = self.cursor.fetchall()

            if len(results) > 0:
                return results[0][0]
            else:
                return None
        except:
            print("Error with query: ", sqlQuery)


    """
        Will return the full path of the system + the random string filename
    """
    def getFullPath(self, randomFilename):
        return self.mainDirectory + "/" + randomFilename


    """
        This method will return the exact byte size of the file
    """
    def getFileSize(self, filename):
        return os.path.getsize(filename)


    """
        This method will verify if the file can be downloaded, if the user
        is root or the owner it can be automatically downloaded. Also it will
        check if the canBeDownloaded flag is turned on
    """
    def canDownload( self, userObj=None, fileID=None ):
        if fileID == None:
            return False
        if fileID > 0:
            sqlQuery="SELECT canDownload, fileOwner FROM files WHERE id=%d;" % fileID
            try:
                # Execute the SQL command
                self.cursor.execute(sqlQuery)

                # Fetch all the rows in a list of lists
                results = self.cursor.fetchall()

                if len(results) > 0:
                    canDown=int(results[0][0])
                    owner=int(results[0][1])
                    if canDown == 1:
                        return True
                    else:
                        if owner == userObj[0]:
                            return True
                        #else:
                        #    if userObj[1] == "root":
                        #        return True
                return False
            except:
                print("Error with query: ", sqlQuery)



    """
        This function will open the file and send the data
        through sockets to the client
    """
    def download(self, client_socket, addr, user, arguments ):
        ##Update db
        self.db.commit()

        ##Parsing commands
        #The original filename will be saved in the database and link it to the
        #random filename
        originalFilename=arguments[0]

        ##Folder id
        folderID=int(arguments[2])

        fileID=self.iterativeFileExists( originalFilename, folderID )
        if fileID == None:
            client_socket.send( "FILE_NOT_FOUND".encode() )
            return
        
        if not self.canDownload(user, fileID):
            client_socket.send( "PERMISSION_DENIED".encode() )
            return
        fullPathFileServer=self.getFullPath(self.getServerFile(fileID))
        fileSize=self.getFileSize(fullPathFileServer)
        ##Send file size
        client_socket.send( str(fileSize).encode() )

        ##Receive flag of download start
        client_socket.recv(self.CHUNK_SIZE)

        ##Open file and keep sending the bytes until the end of the file
        with open( fullPathFileServer, 'rb' ) as f:
            client_socket.sendfile(f, 0)

        ##Receive flag of done receiving
        client_socket.recv(self.CHUNK_SIZE)

        ##Send Download done message to the client
        client_socket.send( "Download done.".encode() )


    """
        This method will return the string format
        of the result of the ls(list) command,
        can be used for folders and files. The client
        will receive the string and parse it for the print
    """
    def formatLS(self, folderList):
        stringFolder = ""
        for folder in folderList:
            
            tmpString = str(folder[0])
            stringFolder+=tmpString+"|"

        if stringFolder == "":
            stringFolder = "None"

        if stringFolder[-1] == "|":
            stringFolder = stringFolder[:-1]

        return stringFolder


    """
        This method checks if the folder exists, if it does then 
        it returns the ID of the folder otherwise it will return
        None(NULL)
    """
    def folderExists(self, idParentFolder, folderName, status=1 ):
        if folderName=="/" or idParentFolder==None:
            sqlQuery="SELECT id FROM folders WHERE status=%d AND parent_folder is NULL AND folder_name='/';" % status
        else:
            sqlQuery="SELECT id FROM folders WHERE status=%d AND parent_folder=%d AND folder_name='%s';" % ( status, idParentFolder, folderName )
        try:
            # Execute the SQL command
            self.cursor.execute(sqlQuery)

            # Fetch all the rows in a list of lists
            folder = self.cursor.fetchall()

            if len( folder ) > 0:
                return folder[0][0]
            else:
                return None

        except:
            print("Error with query: ", sqlQuery)


    """
        Given a full path, it will get the file id
        example input:/folder1/folder2/folder3/file.txt
        it will return the ID of file.txt
    """
    def iterativeFileExists(self, fullPath, idParentFolder ):
        paths=fullPath.split("/")
        if len(paths) == 0:
            return None
        if len(paths) == 1:
            return self.fileExists( workingDir=idParentFolder, filename=paths[0] )
        else:
            fileName=paths[-1]
            folderPath="/".join(paths[:-1])
            idFolder=self.iterativeFolderExists( fullPath=folderPath, idParentFolder=idParentFolder )
            
            if idFolder == None:
                return None
            
            return self.fileExists( workingDir=idFolder, filename=fileName )


    """
        Given a full path, it will get the last folder ID
        example input:/folder1/folder2/folder3
        it will return the ID of the folder 3
    """
    def iterativeFolderExists(self, fullPath, idParentFolder, status=1):
        paths=fullPath.split("/")

        if len(paths) == 0:
            return None

        if paths[-1] == '':
            if len(paths) != 1:
                paths.pop()

        if paths[0] != '':
            if len(paths) == 1:
                return self.folderExists(idParentFolder, paths[0], status=status )
            else:
                currentID=int(idParentFolder)
                for folder in paths:
                    print(folder)
                    tmp=self.folderExists(currentID, folder, status=status)
                    if tmp != None:
                        currentID=int(tmp)
                    else:
                        return None
                return currentID
        else:
            currentID=int(self.folderExists(None, "/", status=status))
            for folder in paths[1:]:
                tmp=self.folderExists(currentID, folder, status=status)
                if tmp != None:
                    currentID=int(tmp)
                else:
                    return None
            return currentID


    """
        This function will try to find the directory which
        the user want to change, if it does exists then it will return
        the ID of the selected folder, otherwise it will reply to the 
        client that the folder was not found(NOT_FOUND)
    """
    def cd(self, client_socket, addr, user, arguments):
        idFolderActual=int(arguments[0])
        folderToChange=arguments[1]

        folderID=self.iterativeFolderExists( folderToChange, idFolderActual)
        if folderID == None:
            client_socket.send( "NOT_FOUND".encode() )
        else:
            client_socket.send( str(folderID).encode() )


    """
        This method will move a folder inside another folder
    """
    def mvdir(self, client_socket, addr, user, arguments):
        idFolderActual=int(arguments[0])
        folderSrc=arguments[1]
        folderDest=arguments[2]
        folderIDSrc=self.iterativeFolderExists( folderSrc, idFolderActual)
        folderIDDest=self.iterativeFolderExists( folderDest, idFolderActual)
        if folderIDSrc == None or folderIDDest == None:
            client_socket.send( "NOT_FOUND".encode() )
        else:
            sqlQuery="CALL moveFolder( %d, %d, %d );" % ( folderIDSrc, folderIDDest, user[0] )
            self.executeQuery(sqlQuery)
            client_socket.send( "Moviendo".encode() )


    """
        This method will move a file inside another folder
    """
    def mv(self, client_socket, addr, user, arguments):
        idFolderActual=int(arguments[0])
        fileSrc=arguments[1]
        folderDest=arguments[2]
        folderIDDest=self.iterativeFolderExists( folderDest, idFolderActual )
        fileID=self.iterativeFileExists( fileSrc, idFolderActual )
        if fileID == None:
            client_socket.send( "FILE_NOT_FOUND".encode() )
            return

        if folderIDDest == None:
            client_socket.send( "FOLDER_NOT_FOUND".encode() )
            return

        sqlQuery="CALL moveFile( %d, %d, %d );" % (fileID, user[0], folderIDDest )
        self.executeQuery(sqlQuery)
        client_socket.send( "Moviendo".encode() )


    """
        This method will copy a file into another folder
    """
    def cp(self, client_socket, addr, user, arguments):
        idFolderActual=int(arguments[0])
        fileSrc=arguments[1]
        folderDest=arguments[2]
        folderIDDest=self.iterativeFolderExists( folderDest, idFolderActual )
        fileID=self.iterativeFileExists( fileSrc, idFolderActual )
        if fileID == None:
            client_socket.send( "FILE_NOT_FOUND".encode() )
            return

        if folderIDDest == None:
            client_socket.send( "FOLDER_NOT_FOUND".encode() )
            return

        sqlQuery="CALL copyFile( %d, %d, %d );" % (fileID, folderIDDest, user[0] )
        self.executeQuery(sqlQuery)
        client_socket.send( "Copiando".encode() )


    """
        This method will list all the files and folders in the current
        folder the user is at
    """
    def ls(self, client_socket, addr, user, arguments):
        currentDirID=int(arguments[0])

        folders=[]
        files=[]

        ##Getting folders first
        sqlQuery="SELECT folder_name FROM folders WHERE status=1 AND parent_folder=%d;" % currentDirID
        try:
            # Execute the SQL command
            self.cursor.execute(sqlQuery)

            # Fetch all the rows in a list of lists
            folders = self.cursor.fetchall()

        except:
            print("Error with query: ", sqlQuery)

        client_socket.send( self.formatLS(folders).encode() )
        ok=client_socket.recv( self.CHUNK_SIZE )
        sqlQuery="SELECT filename from files WHERE folder_id=%d AND status=1;" % currentDirID
        try:
            # Execute the SQL command
            self.cursor.execute(sqlQuery)

            # Fetch all the rows in a list of lists
            files = self.cursor.fetchall()

        except:
            print("Error with query: ", sqlQuery)
        client_socket.send( self.formatLS(files).encode() )

    def lsUsers(self, client_socket, addr, user, arguments):
        users=[]

        ##Getting folders first
        sqlQuery="SELECT username FROM users WHERE status=1;"
        try:
            # Execute the SQL command
            self.cursor.execute(sqlQuery)

            # Fetch all the rows in a list of lists
            users = self.cursor.fetchall()

            client_socket.send( self.formatLS(users).encode() )

        except:
            print("Error with query: ", sqlQuery)

    """
        This method will create a new folder inside the current folder
    """
    def mkdir(self, client_socket, addr, user, arguments):
        currentDirectoryName=arguments[0]
        currentDirID=int(arguments[1])
        foldersToCreate=arguments[2:]

        for folder in foldersToCreate:
            sqlQuery="CALL addFolder( '%s', %d, %d);" % ( folder, user[0], currentDirID )
            self.executeQuery(sqlQuery)
            
        if len(foldersToCreate) == 1:
            client_socket.send( "Folder created sucessfully".encode() )

        else:
            client_socket.send( "Folders created sucessfully".encode() )


    """
        This method will remove a folder inside the current directory
    """
    def rmdir(self, client_socket, addr, user, arguments):
        currentDirID=int(arguments[1])
        folderToRemove=arguments[2]
        folderID=self.iterativeFolderExists(folderToRemove, currentDirID)
        if folderID != None:
            sqlQuery="CALL delFolder( %d, %d );" % ( int(folderID), user[0] )
            self.executeQuery(sqlQuery)
            client_socket.send( "Folder removed".encode() )
        else:  
            client_socket.send( "NOT_FOUND".encode() )


    """
        This method will remove a file in the current folder
    """
    def rm(self, client_socket, addr, user, arguments):
        currentDirID=int(arguments[1])
        fileToRemove=arguments[2]
        fileID=self.fileExists( workingDir=currentDirID, filename=fileToRemove )
        if fileID != None:
            sqlQuery="CALL delFile( '%s', %d, %d );" % ( fileToRemove, currentDirID, user[0] )
            self.executeQuery(sqlQuery)
            client_socket.send( "File removed".encode() )
        else:
            client_socket.send( "NOT_FOUND".encode() )


    """
        This method will undo the last action on the file
    """
    def undoFile(self, client_socket, addr, user, arguments):
        currentDirID=int(arguments[1])
        fileToUndo=arguments[2]
        fileID=self.fileExists( workingDir=currentDirID, filename=fileToUndo )

        if fileID == None:
            fileID=self.fileExists( workingDir=currentDirID, filename=fileToUndo, status=0 )

        if fileID != None:
            sqlQuery="CALL doUndo( 'FILE', %d, %d );" % ( int(fileID), user[0] )
            self.executeQuery(sqlQuery)
            client_socket.send( "File Undone".encode() )
        else:
            client_socket.send( "NOT_FOUND".encode() )
    

    """
        This method will undo the last action on the folder
    """
    def undoFolder(self, client_socket, addr, user, arguments):
        currentDirID=int(arguments[1])
        folderToUndo=arguments[2]
        folderID=self.iterativeFolderExists(folderToUndo, currentDirID)
        if folderID == None:
            folderID=self.iterativeFolderExists(folderToUndo, currentDirID, status=0)

        if folderID != None:
            sqlQuery="CALL doUndo( 'FOLDER', %d, %d );" % ( int(folderID), user[0] )
            self.executeQuery(sqlQuery)
            client_socket.send( "Folder Undone".encode() )
        else:
            client_socket.send( "NOT_FOUND".encode() )


    """
        This method will create a new user in the database
    """
    def createUser(self, client_socket, addr, user, arguments):
        user=arguments[1]
        passwd=arguments[2]
        sqlQuery="INSERT INTO users( username, passwd ) VALUES( '%s', MD5('%s') );" % (user, passwd)
        self.executeQuery(sqlQuery)
        client_socket.send( "User created".encode() )


    """
        This method will remove an user from the database
    """
    def delUser(self, client_socket, addr, user, arguments):
        user=arguments[1]
        idUser=self.getUsernameID(user)
        if idUser == None:
            client_socket.send( "USER_NOT_FOUND".encode() )
            return
        
        sqlQuery="DELETE FROM users WHERE id=%d;" % idUser
        self.executeQuery(sqlQuery)
        client_socket.send( "User deleted.".encode() )


    """
        This method will share the file with everyone else
    """
    def share(self, client_socket, addr, user, arguments):
        currentDirID=int(arguments[1])
        fileName=arguments[2]
        fileID=self.fileExists( workingDir=currentDirID, filename=fileName )
        if fileID == None:
            client_socket.send( "NOT_FOUND".encode() )
            return
        
        fileOwner=self.getFileOwner( idFile=fileID )
        if fileOwner == None:
            client_socket.send( "ERROR".encode() )
            return

        if fileOwner[0] == user[0] or user[1] == "root":
            sqlQuery="UPDATE files SET canDownload=1 WHERE id=%d;" % fileID
            self.executeQuery(sqlQuery)
        else:
            client_socket.send( "PERMISSION_DENIED".encode() )
            return
        client_socket.send( "File Shared.".encode() )


    """
        This method will make download unavailable for everyone except the
        owner user
    """
    def rmShare(self, client_socket, addr, user, arguments):
        currentDirID=int(arguments[1])
        fileName=arguments[2]
        fileID=self.fileExists( workingDir=currentDirID, filename=fileName )
        if fileID == None:
            client_socket.send( "NOT_FOUND".encode() )
            return
        
        fileOwner=self.getFileOwner( idFile=fileID )
        if fileOwner == None:
            client_socket.send( "ERROR".encode() )
            return

        if fileOwner[0] == user[0] or user[1] == "root":
            sqlQuery="UPDATE files SET canDownload=0 WHERE id=%d;" % fileID
            self.executeQuery(sqlQuery)
        else:
            client_socket.send( "PERMISSION_DENIED".encode() )
            return
        client_socket.send( "File share removed.".encode() )

    """
        This is the main method that will keep always running,
        if a new client is connected, then try to authenticate,
        if the authentication was successfull then create a new
        thread and attend all the commands the user wants to execute.
    """
    def listen(self):
        while True:
            ##Wait for a connection
            client_socket, addr = self.server_socket.accept()

            ##Authentication data
            auth=client_socket.recv(self.CHUNK_SIZE)

            ##Parse the auth data
            authData=self.parseLogin( auth.decode() )

            ##Try login
            userData=self.login( authData['username'], authData['passwd'], client_socket )

            if userData == None:
                print("Wrong auth try from ", addr)
                continue
            print("Login success of the user: ", userData[1], " connected from ", addr )

            thread = threading.Thread(target=self.run, args=( client_socket, addr, userData ))
            thread.daemon = True
            thread.start()

    """
        This is the function the new thread will run,
        it contains the commands available and will keep
        listening from the client until he writes 'exit'
    """
    def run(self, client_socket, addr, user):
        option_types = {
            'U':self.uploadFile,
            'M':self.mkdir,
            'L':self.ls,
            'C':self.cd,
            'P':self.rmdir,
            'Z':self.mvdir,
            'R':self.rm,
            'K':self.mv,
            'O':self.cp,
            'D':self.download,
            'B':self.undoFile,
            'A':self.undoFolder,
            'G':self.createUser,
            'E':self.delUser,
            'V':self.share,
            'N':self.rmShare,
            'I':self.lsUsers,
        }

        while True:
            try:
                ##Receive command option and update db
                self.db.commit()
                command=client_socket.recv( self.CHUNK_SIZE )
                self.db.commit()

                ##Check if the user wants to exit
                if command.decode() == "EXIT":
                    return

                ##Parse the command to get the options to do
                commandDictionary = self.parseCommand( command.decode() )

                ##Executing command
                if commandDictionary['command'] in option_types:
                    option_types[commandDictionary['command']](client_socket, addr, user, commandDictionary['args'])

            except:
                return

server = XipetotecFSServer()
server.listen()