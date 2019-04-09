
#!/bin/bash

#Verify that is the root user
if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root" 
   exit 1
fi


##Get the installation script path
ABSPATH=$(readlink -f $0)
ABSDIR=$(dirname $ABSPATH)

#Load configuration file
source "$ABSDIR/config.conf"

echo "Installing dependencies..."
apt-get update
apt-get install mysql-server python-pymysql python3-pip
pip3 install pymysql
echo "Depenedencies done."


echo "Installng database..."
##Keep trying to login into mysql
while [ 1 -eq  1 ]; do
        mysql -u root -p < "$ABSDIR/XipetotecDB.sql"
        ok=$?

        if [ 0 -eq  $ok ]; then
                break
        fi
done
echo "Database installed."

echo "Installing main program..."
mkdir "$xipetotecMainPath" 2> /dev/null
mkdir "$xipetotecFilesFolder" 2> /dev/null
mkdir "$xipetotecProgramFolder" 2> /dev/null

cp "$ABSDIR/XipetotecServer.py" "$xipetotecProgramFolder"
chmod 755 "$xipetotecProgramFolder/XipetotecServer.py"

echo "python3 $xipetotecProgramFolder/XipetotecServer.py " > /bin/XipetotecServer
chmod 755 /bin/XipetotecServer
ln -s /bin/XipetotecServer /usr/bin

echo "Main program installed, you can run it like this:"
echo "XipetotecServer"