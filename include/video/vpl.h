#ifndef __VIDEO_VPL_H
#define __VIDEO_VPL_H

#include <fb.h>

#define VPL_PREPARE		0x67660001
#define VPL_UNPREPARE		0x67660002
#define VPL_ENABLE		0x67660003
#define VPL_DISABLE		0x67660004
#define VPL_GET_VIDEOMODES	0x67660005
#define VPL_GET_BUS_FORMAT	0x67660006

struct vpl {
	int (*ioctl)(struct vpl *, unsigned int port,
			unsigned int cmd, void *ptr);
	struct device_node *node;
	struct list_head list;
};

int vpl_register(struct vpl *);
int vpl_ioctl(struct vpl *, unsigned int port,
		unsigned int cmd, void *ptr);

struct vpl *of_vpl_get(struct device_node *node, int port);

static inline int vpl_ioctl_prepare(struct vpl *vpl, unsigned int port,
		struct fb_videomode *mode)
{
	return vpl_ioctl(vpl, port, VPL_PREPARE, mode);
}

static inline int vpl_ioctl_unprepare(struct vpl *vpl, unsigned int port)
{
	return vpl_ioctl(vpl, port, VPL_UNPREPARE, NULL);
}

static inline int vpl_ioctl_enable(struct vpl *vpl, unsigned int port)
{
	return vpl_ioctl(vpl, port, VPL_ENABLE, NULL);
}

static inline int vpl_ioctl_disable(struct vpl *vpl, unsigned int port)
{
	return vpl_ioctl(vpl, port, VPL_DISABLE, NULL);
}

#endif /* __VIDEO_VPL_H */
