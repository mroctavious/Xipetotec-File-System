
DROP DATABASE IF EXISTS xipetotecFS;
CREATE DATABASE xipetotecFS;
USE xipetotecFS;

DROP USER IF EXISTS 'xipetotec'@'localhost';
CREATE USER 'xipetotec'@'localhost' IDENTIFIED BY 'Mexico123!';
GRANT ALL PRIVILEGES ON xipetotecFS.* TO 'xipetotec'@'localhost';

##This table will contain the user login information
CREATE TABLE users(
    id INT PRIMARY KEY AUTO_INCREMENT,
    username VARCHAR(64) NOT NULL UNIQUE,
    passwd VARCHAR(512) NOT NULL,
    creationDate TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    status TINYINT NOT NULL DEFAULT 1
) ENGINE=INNODB;

CREATE TABLE folders(
    id            INT PRIMARY KEY AUTO_INCREMENT,
	folder_name   VARCHAR(256) NOT NULL,
    folder_owner  INT NOT NULL,
    creation_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
	status TINYINT NOT NULL DEFAULT 1,
    parent_folder  INT NULL,
    FOREIGN KEY (folder_owner) REFERENCES users(id)
    ON DELETE CASCADE,    
    FOREIGN KEY(parent_folder) REFERENCES folders(id)
    ON DELETE CASCADE
);
ALTER TABLE folders ADD UNIQUE uniqueFolders(folder_name, parent_folder);

##This table will contain all the files in the system
CREATE TABLE files(
    id       INT PRIMARY KEY AUTO_INCREMENT,
    filename VARCHAR(256) NOT NULL,
    serverFile VARCHAR(256) DEFAULT NULL,
	creationDate TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    fileOwner INT NOT NULL,
    canDownload TINYINT NOT NULL DEFAULT 1,
    status TINYINT NOT NULL DEFAULT 1,
    folder_id INT NOT NULL,
    FOREIGN KEY (fileOwner) REFERENCES users(id)
    ON DELETE CASCADE,
    FOREIGN KEY (folder_id) REFERENCES folders(id)
    ON DELETE CASCADE    
);
ALTER TABLE files ADD UNIQUE uniqueFiles(filename, folder_id);

CREATE TABLE log_registry(
	id            INT PRIMARY KEY AUTO_INCREMENT,
    action_type   VARCHAR(20) NOT NULL,
    user_id       INT NOT NULL,
    creationDate  TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    file_id       INT,
    folder_id     INT,
    origin_folder INT,
    origin_file   INT,
    status        TINYINT NOT NULL DEFAULT 1,
    FOREIGN KEY (user_id) REFERENCES users(id)
    ON DELETE CASCADE
);

DELIMITER //
CREATE PROCEDURE addFolder(IN folderName VARCHAR(256), IN user_id INT,IN parent_folder INT)
BEGIN
    DECLARE fId INT;
    DECLARE newFId INT;
    SELECT getID("FOLD",folderName,parent_folder) INTO newFId;    
    IF newFId=0 THEN
        INSERT INTO folders(folder_name,folder_owner,parent_folder)
        VALUES(folderName,user_id,parent_folder);        
    ELSE
        UPDATE folders SET status=1 
        WHERE id=newFId;
    END IF;
    SELECT getID("FOLD",folderName,parent_folder) INTO fId;        
    CALL addLog("CREATE_FOLDER",user_id, NULL, fId ,NULL,NULL);
END //                              
DELIMITER ;

DELIMITER //
CREATE PROCEDURE undoAddFolder(IN pFolderId VARCHAR(256), IN pUserId INT)
BEGIN
	UPDATE folders SET
		status="0"
	WHERE id=pfolderId
    AND folder_owner=pUserId; 
    CALL addLog("UNDO_ADDFOLDER",pUserId, NULL, pFolderId ,NULL,NULL);
END //                              
DELIMITER ;


DELIMITER //
CREATE PROCEDURE delFolder(IN folderId INT, IN user_id INT)
BEGIN
    UPDATE folders SET
       status="0"
	WHERE id=folderId;
    CALL addLog("DELETE_FOLDER",user_id, NULL, folderId ,NULL,NULL);
END //                              
DELIMITER ;


DELIMITER //
CREATE PROCEDURE undoDelFolder(IN pFolderId INT, IN pUser_Id INT)
BEGIN
    UPDATE folders SET
       status="1"
	WHERE id=pFolderId
    AND folder_owner=pUser_Id;
    CALL addLog("UNDO_DELFOLDER",pUser_Id, NULL, pFolderId ,NULL,NULL);
END //                              
DELIMITER ;


DELIMITER //
CREATE PROCEDURE moveFolder(IN pfolderId INT,IN pfolderIdDes INT, IN user_id INT)
BEGIN
    DECLARE IdFolderOri INT;
    SELECT parent_folder
    INTO   IdFolderOri
    FROM   folders
    WHERE  id=pfolderId;
    UPDATE folders SET
       parent_folder=pfolderIdDes
	WHERE id=pfolderId;
    CALL addLog("MOVE_FOLDER",user_id, NULL, pfolderId ,IdFolderOri,NULL);
END //                              
DELIMITER ;


DELIMITER //
CREATE PROCEDURE undoMoveFolder(IN pfolderId INT,IN pfolderIdDes INT, IN user_id INT)
BEGIN
    DECLARE IdFolderOri INT;
    SELECT parent_folder
    INTO   IdFolderOri
    FROM   folders
    WHERE  id=pfolderId
    AND folder_owner=user_id;
    UPDATE folders SET
       parent_folder=pfolderIdDes
	WHERE id=pfolderId
    AND folder_owner=user_id;
    CALL addLog("UNDO_MOVEFOLDER",user_id, NULL, pfolderId ,IdFolderOri,NULL);
END //                              
DELIMITER ;


DELIMITER //
CREATE PROCEDURE addFile(IN fileName VARCHAR(256),IN pserverFile VARCHAR(256), IN user_id INT,IN folder_id INT)
BEGIN
    DECLARE fId INT;
    DECLARE fIdExiste INT;
    SELECT getID("FILE",filename,folder_id) INTO fIdExiste;
    IF fIdExiste=0 THEN
		INSERT INTO files(filename,serverFile,fileOwner,folder_id)
		VALUES(filename,pserverFile,user_id,folder_id);
	ELSE
       UPDATE files SET status="1", serverFile=pserverFile WHERE id=fIdExiste;
    END IF;
	SELECT getID("FILE",filename,folder_id) INTO fId;
    CALL addLog("CREATE_FILE",user_id, fId, folder_id ,NULL,NULL);
END //                              
DELIMITER ;


DELIMITER //
CREATE PROCEDURE undoAddFile(IN pfileId INT, IN pUserId INT)
BEGIN
	UPDATE files SET 
       status="0" 
	WHERE id=pFileId
    AND fileOwner=pUserId;
    CALL addLog("UNDO_CREATEFILE",pUserId, pFileId, NULL ,NULL,NULL);
END //                              
DELIMITER ;


DELIMITER //
CREATE PROCEDURE delFile(IN fileName VARCHAR(256),IN folder_id INT, IN user_id INT)
BEGIN
    DECLARE fId INT;
    SELECT getID("FILE",fileName,folder_id) INTO fId;
    UPDATE files SET
       status="0"
	WHERE id=fId;    
    CALL addLog("DELETE_FILE",user_id, fId, folder_id ,NULL,NULL);
END //                              
DELIMITER ;


DELIMITER //
CREATE PROCEDURE undoDelFile(IN pFileID INT, IN pUserId INT)
BEGIN
    UPDATE files SET
       status="1"
	WHERE id=pFileID
    AND fileOwner=pUserId;    
    CALL addLog("UNDO_DELETEFILE",pUserId, pFileID, NULL ,NULL,NULL);
END //                              
DELIMITER ;


DELIMITER //
CREATE PROCEDURE moveFile(IN pfileID INT, IN puser_id INT,IN pfolder_id_des INT)
BEGIN
    DECLARE folIdOri INT;
    SELECT folder_id
    INTO   folIdOri
    FROM   files
    WHERE  id=pfileID;
    UPDATE files SET
       folder_id=pfolder_id_des
	WHERE id=pfileID;    
    CALL addLog("MOVE_FILE",puser_id, pfileID, pfolder_id_des ,folIdOri,pfileID);
END //                              
DELIMITER ;


DELIMITER //
CREATE PROCEDURE undoMoveFile(IN pFileId INT,IN pFolderIdDes INT, IN pUserId INT)
BEGIN
    UPDATE files SET
       folder_id=pFolderIdDes
	WHERE id=pFileId
    AND fileOwner=pUserId;    
    CALL addLog("UNDO_MOVEFILE",pUserId, pFileId, NULL ,NULL,NULL);
END //                              
DELIMITER ;


DELIMITER //
CREATE PROCEDURE copyFile(IN pfileId INT,IN pfolder_id_des INT, IN puser_id INT)
BEGIN
    DECLARE fName VARCHAR(256);
    DECLARE sFileName VARCHAR(256);
    DECLARE fId INT;
    SELECT filename,serverFile
    INTO fName, sFileName
    FROM files
    WHERE id=pfileId;
    CALL addFile(fName,sFileName, puser_id,pfolder_id_des);
END //                              
DELIMITER ;


DELIMITER //
CREATE PROCEDURE undoCopyFile(IN pFileId INT, IN pUserId INT)
BEGIN
	UPDATE files SET
		status="0"
	WHERE id=pFileId
    AND fileOwner=pUserId;
    CALL addLog("UNDO_COPYFILE",pUserId, pFileId, NULL ,NULL,NULL);
END //                              
DELIMITER ;


DELIMITER //
CREATE PROCEDURE doUndo(IN pType VARCHAR(6),IN pFId INT, IN pUserId INT)
BEGIN
	DECLARE numRegFiles   INT;
    DECLARE maxIdReg      INT;
    DECLARE actionT       VARCHAR(20);
    DECLARE IdOrigin      INT;
    DECLARE numRegFolders INT;
    IF pType="FILE" THEN
		SELECT count(1) 
        INTO   numRegFiles
        FROM   log_registry
		WHERE  file_id=pFId
		AND    action_type NOT LIKE 'UNDO%'
		AND    status="0"
        AND    user_id=pUserId;
		IF numRegFiles<2 THEN
			SELECT max(id) 
			INTO   maxIdReg
			FROM   log_registry
			WHERE  file_id=pFId
			AND    action_type NOT LIKE 'UNDO%'
			AND    status="1"
			AND    user_id=pUserId;
            IF maxIdReg>0 THEN
				SELECT action_type,origin_folder
                INTO   actionT,IdOrigin
                FROM   log_registry
                WHERE id=maxIdReg;
                IF actionT="CREATE_FILE" THEN
					CALL undoAddFile(pFId, pUserId);
                ELSEIF actionT="DELETE_FILE" THEN
					CALL undoDelFile(pFId, pUserId);
                ELSEIF actionT="MOVE_FILE" THEN 
                    CALL undoMoveFile(pFId,IdOrigin,pUserId);
				END IF;
                UPDATE log_registry SET
					status="0"
				WHERE id=maxIdReg;
            END IF;
        END IF;
    ELSE
		SELECT count(1) 
        INTO   numRegFiles
        FROM   log_registry
		WHERE  folder_id=pFId
        AND    file_id IS NULL
		AND    action_type NOT LIKE 'UNDO%'
		AND    status="0"
        AND    user_id=pUserId;    
		IF numRegFiles<2 THEN
			SELECT max(id) 
			INTO   maxIdReg
			FROM   log_registry
			WHERE  folder_id=pFId
            AND    file_id IS NULL
			AND    action_type NOT LIKE 'UNDO%'
			AND    status="1"
			AND    user_id=pUserId;
            IF maxIdReg>0 THEN
				SELECT action_type,origin_folder
                INTO   actionT,IdOrigin
                FROM   log_registry
                WHERE id=maxIdReg;
                IF actionT="CREATE_FOLDER" THEN
					CALL undoAddFolder(pFId, pUserId);
                ELSEIF actionT="DELETE_FOLDER" THEN
					CALL undoDelFolder(pFId, pUserId);
                ELSEIF actionT="MOVE_FOLDER" THEN 
                    CALL undoMoveFolder(pFId,IdOrigin,pUserId);
				END IF;
                UPDATE log_registry SET
					status="0"
				WHERE id=maxIdReg;
            END IF;
        END IF;        
    END IF;
END //                              
DELIMITER ;


DELIMITER //
CREATE FUNCTION getId(getType VARCHAR(4),fname VARCHAR(256),fparent INT) 
RETURNS INT
BEGIN
    DECLARE fId INT DEFAULT 0;
	IF getType="FILE" THEN
		SELECT id
        INTO   fId
        FROM   files
        WHERE  filename=fname
        AND    folder_id=fparent;
    ELSE
		SELECT id
        INTO   fId
        FROM   folders
        WHERE  folder_name=fname
        AND    IFNULL(parent_folder,0)=IFNULL(fparent,0);
    END IF;
    RETURN fId;
END //
DELIMITER ;

DELIMITER //
CREATE  PROCEDURE addLog(IN action_type VARCHAR(20),IN user_id INT, IN file_id INT,IN folder_id INT,IN origin_folder INT,IN origin_file INT)
BEGIN
    INSERT INTO log_registry(action_type,user_id,file_id,folder_id,origin_folder,origin_file)
    VALUES(action_type,user_id,file_id,folder_id,origin_folder,origin_file);
END //                              
DELIMITER ;



DELIMITER //
CREATE PROCEDURE addRegistry( IN pathLocation VARCHAR(750), IN fileName VARCHAR(256), IN serverFile VARCHAR(256),
                              IN fileType VARCHAR(1), IN fileOwner INT, IN typeAction VARCHAR(20))
BEGIN
	##Verificatioon of the last slash character
	DECLARE lastChar VARCHAR(1);
	DECLARE fullPathFile VARCHAR(750) DEFAULT " ";
	DECLARE withSlash INT DEFAULT 0;
    
	SELECT Substring(pathLocation, -1) INTO lastChar;
	
    SELECT IF( lastChar="/", TRUE, FALSE) INTO  withSlash;
	
    IF withSlash=1 THEN
		SELECT CONCAT(pathLocation,filename) INTO fullPathFile;
	ELSE
		SELECT CONCAT(pathLocation,"/",filename) INTO fullPathFile;
	END IF;
	
    INSERT INTO files( fullPath, pathLocation,  filename, serverFile, fileType, fileOwner, canDownload )
	VALUES (  fullPathFile, pathLocation,  filename, serverFile, fileType, fileOwner, '0' );
    CALL addLog(fullPathFile,typeAction,fileOwner);
END //                              
DELIMITER ;

#=====================================================

DELIMITER //
CREATE PROCEDURE moveRegistry(IN fullPathOrigin VARCHAR(750), IN pathLocation VARCHAR(750), IN fileName VARCHAR(256), 
							  IN serverFile VARCHAR(256), IN fileType VARCHAR(1), IN fileOwner INT)
BEGIN
	UPDATE files SET
       status='0'
	WHERE fullPath=fullPathOrigin;
    CALL addRegistry(pathLocation,fileName,serverFile,fileType,fileOwner);
END //                              
DELIMITER ;


#=====================================================

##Create user procedure so it can be easily callable from python
DELIMITER //
	CREATE PROCEDURE addFolderByPath ( IN filename VARCHAR(256),  fullpath VARCHAR(750), idUser INT )
		BEGIN
			##Verificatioon of the last slash character
			DECLARE lastChar CHAR DEFAULT ' ';
			DECLARE fullPathFile VARCHAR(256) DEFAULT " ";
			DECLARE withSlash INT DEFAULT 0;
            SELECT Substring(fullpath, -1) INTO lastChar;
            SELECT IF( lastChar="/", TRUE, FALSE) INTO  withSlash;
            IF withSlash=1 THEN
			SELECT CONCAT(fullpath,filename) INTO fullPathFile;
            ELSE
				SELECT CONCAT(fullpath,"/",filename) INTO fullPathFile;
            END IF;
            
            
            INSERT INTO files( fullPath, pathLocation,  filename, serverFile, fileType, fileOwner, canDownload )
				VALUES (  fullPathFile, fullpath,  filename, NULL, "D", idUser, '1' );
		END //
DELIMITER ;


DELIMITER //
	CREATE PROCEDURE listFolders ( fullpath VARCHAR(750) )
		BEGIN
			##Verificatioon of the last slash character
            SELECT fullPath, filename FROM files WHERE pathLocation=fullpath AND fileType='D';
		END //
DELIMITER ;


DELIMITER //
	CREATE PROCEDURE listFiles ( fullpath VARCHAR(750) )
		BEGIN
			##Verificatioon of the last slash character
            SELECT fullPath, filename FROM files WHERE pathLocation=fullpath AND fileType='F';
		END //
DELIMITER ;


DELIMITER //
	CREATE PROCEDURE listAll ( fullpath VARCHAR(750) )
		BEGIN
			##Verificatioon of the last slash character
            SELECT fullPath, filename FROM files WHERE pathLocation=fullpath;
		END //
DELIMITER ;

##Initial inserts
INSERT INTO users( username, passwd ) VALUES( 'root', MD5('Mexico123') );
INSERT INTO folders( folder_name, parent_folder,folder_owner ) VALUES( "/", NULL, (SELECT id FROM users WHERE username='root' AND status=1) );
