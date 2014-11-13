umount /home/$USER/tmp
mdconfig -d -u 10
mdconfig -a -t swap -s 256m -u 10
newfs -U md10
mount /dev/md10 /home/$USER/tmp
chown $USER /home/$USER/tmp
chmod 777 /home/$USER/tmp
