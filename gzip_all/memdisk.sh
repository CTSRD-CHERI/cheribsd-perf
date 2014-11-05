umount /mnt2
mdconfig -d -u 2
mdconfig -a -t swap -s 256m -u 2
newfs -U md2
mount /dev/md2 /mnt2
chmod 777 /mnt2
chmod 777 /mnt2/*
