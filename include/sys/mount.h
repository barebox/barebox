#ifndef __SYS_MOUNT_H
#define __SYS_MOUNT_H

int mount(const char *device, const char *fsname, const char *path,
		const char *fsoptions);
int umount(const char *pathname);

#endif /* __SYS_MOUNT_H */
